#include "EditorLayer.h"

// Project includes.
#include "Core/Application.h"
#include "Core/Logs.h"
#include "Core/AppStatus.h"
#include "GuiElements/Styles.h"
#include "GuiElements/Panels.h"
#include "Serialization/YamlSerialization.h"
#include "Scenes/Components.h"

// Some math for decomposing matrix transformations.
#include "glm/gtx/matrix_decompose.hpp"

// ImGizmo goodies.
#include "imguizmo/ImGuizmo.h"

#define SCENE_GRAPH  0
#define CAMERA_MENU  1
#define SHADER_MENU  2
#define FILE_BROWSER  3
#define MODEL_INFO  4
#define CONTENT_BROWSER  5
#define RENDERER_SETTINGS  6
#define VIEWPORT_MENU  7
#define APP_SETTINGS_WINDOW 8

namespace Strontium
{
  EditorLayer::EditorLayer()
    : Layer("Editor Layer")
    , loadTarget(FileLoadTargets::TargetNone)
    , saveTarget(FileSaveTargets::TargetNone)
    , dndScenePath("")
    , showPerf(true)
    , editorSize(ImVec2(0, 0))
    , sceneState(SceneState::Edit)
  { }

  EditorLayer::~EditorLayer()
  {
    for (auto& window : this->windows)
      delete window;
  }

  void
  EditorLayer::onAttach()
  {
    Styles::setDefaultTheme();
    YAMLSerialization::deserializeAppStatus(editorStatus, CONFIG_FILEPATH);

    // Fetch the width and height of the window and create a floating point
    // framebuffer.
    glm::ivec2 wDims = Application::getInstance()->getWindow()->getSize();
    this->drawBuffer = createShared<FrameBuffer>((uint) wDims.x, (uint) wDims.y);

    // Fetch a default floating point FBO spec and attach it. Also attach a single
    // float spec for entity IDs.
    auto cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour0);
    this->drawBuffer->attachTexture2D(cSpec);
    cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour1); // The ID texture.
    cSpec.internal = TextureInternalFormats::R32f;
    cSpec.format = TextureFormats::Red;
    cSpec.sWrap = TextureWrapParams::ClampEdges;
    cSpec.tWrap = TextureWrapParams::ClampEdges;
    this->drawBuffer->attachTexture2D(cSpec);
    this->drawBuffer->setDrawBuffers();
  	this->drawBuffer->attachTexture2D(FBOCommands::getDefaultDepthSpec());

    // Setup stuff for the scene.
    this->currentScene = createShared<Scene>();

    // Finally, the editor camera.
    this->editorCam = createShared<EditorCamera>(1920 / 2, 1080 / 2, glm::vec3 { 0.0f, 1.0f, 4.0f },
                                                 EditorCameraType::Stationary);
    this->editorCam->init(90.0f, 1.0f, 0.1f, 200.0f);

    // All the windows!
    this->windows.push_back(new SceneGraphWindow(this));
    this->windows.push_back(new CameraWindow(this, this->editorCam));
    this->windows.push_back(new ShaderWindow(this));
    this->windows.push_back(new FileBrowserWindow(this));
    this->windows.push_back(new ModelWindow(this, false));
    this->windows.push_back(new AssetBrowserWindow(this));
    this->windows.push_back(new RendererWindow(this)); // 6
    this->windows.push_back(new ViewportWindow(this));
    this->windows.push_back(new AppSettingsWindow(this));
  }

  void
  EditorLayer::onDetach()
  {
      saveWindows();
      editorCam->saveSettings();
      YAMLSerialization::serializeAppStatus(editorStatus, CONFIG_FILEPATH);
  }

  // On event for the layer.
  void
  EditorLayer::onEvent(Event &event)
  {
    // Push the events through to all the gui elements.
    for (auto& window : this->windows)
      window->onEvent(event);

    // Push the event through to the editor camera.
    this->editorCam->onEvent(event);

    // Handle events for the layer.
    switch(event.getType())
    {
      case EventType::KeyPressedEvent:
      {
        auto keyEvent = *(static_cast<KeyPressedEvent*>(&event));
        this->onKeyPressEvent(keyEvent);
        break;
      }

      case EventType::MouseClickEvent:
      {
        auto mouseEvent = *(static_cast<MouseClickEvent*>(&event));
        this->onMouseEvent(mouseEvent);
        break;
      }

      case EventType::EntityDeleteEvent:
      {
        auto entDeleteEvent = *(static_cast<EntityDeleteEvent*>(&event));
        auto entityID = entDeleteEvent.getStoredEntity();
        auto entityParentScene = entDeleteEvent.getStoredScene();

        this->currentScene->recurseDeleteEntity(Entity((entt::entity) entityID, entityParentScene));

        break;
      }

      case EventType::LoadFileEvent:
      {
        auto loadEvent = *(static_cast<LoadFileEvent*>(&event));

        if (this->loadTarget == FileLoadTargets::TargetScene)
        {
          Shared<Scene> tempScene = createShared<Scene>();
          bool success = YAMLSerialization::deserializeScene(tempScene, loadEvent.getAbsPath());

          if (success)
          {
            this->currentScene = tempScene;
            this->currentScene->getSaveFilepath() = loadEvent.getAbsPath();
            static_cast<SceneGraphWindow*>(this->windows[0])->setSelectedEntity(Entity());
            static_cast<ModelWindow*>(this->windows[4])->setSelectedEntity(Entity());
          }
        }

        this->loadTarget = FileLoadTargets::TargetNone;
        break;
      }

      case EventType::SaveFileEvent:
      {
        auto saveEvent = *(static_cast<SaveFileEvent*>(&event));

        if (this->saveTarget == FileSaveTargets::TargetScene)
        {
          std::string name = saveEvent.getFileName().substr(0, saveEvent.getFileName().find_last_of('.'));
          YAMLSerialization::serializeScene(this->currentScene, saveEvent.getAbsPath(), name);
          this->currentScene->getSaveFilepath() = saveEvent.getAbsPath();

          if (this->dndScenePath != "")
          {
            static_cast<SceneGraphWindow*>(this->windows[0])->setSelectedEntity(Entity());
            static_cast<ModelWindow*>(this->windows[4])->setSelectedEntity(Entity());

            Shared<Scene> tempScene = createShared<Scene>();
            if (YAMLSerialization::deserializeScene(tempScene, this->dndScenePath))
            {
              this->currentScene = tempScene;
              this->currentScene->getSaveFilepath() = this->dndScenePath;
            }
            this->dndScenePath = "";
          }
        }

        this->saveTarget = FileSaveTargets::TargetNone;
        break;
      }

      default:
      {
        break;
      }
    }
  }

  // On update for the layer.
  void
  EditorLayer::onUpdate(float dt)
  {
    // Update each of the windows.
    for (auto& window : this->windows)
      window->onUpdate(dt, this->currentScene);

    // Update the size of the framebuffer to fit the editor window.
    glm::vec2 size = this->drawBuffer->getSize();
    if (this->editorSize.x != size.x || this->editorSize.y != size.y)
    {
      if (this->editorSize.x >= 1.0f && this->editorSize.y >= 1.0f)
        this->editorCam->updateProj(this->editorCam->getHorFOV(),
                                    this->editorSize.x / this->editorSize.y,
                                    this->editorCam->getNear(),
                                    this->editorCam->getFar());
      this->drawBuffer->resize(this->editorSize.x, this->editorSize.y);
    }

    // Update the scene.
    switch (this->sceneState)
    {
      case SceneState::Edit:
      {
        // Update the scene.
        this->currentScene->onUpdateEditor(dt);

        // Draw the scene.
        this->drawBuffer->clear();
        Renderer3D::begin(this->editorSize.x, this->editorSize.y, (Camera) (*this->editorCam.get()));
        this->currentScene->onRenderEditor(this->getSelectedEntity());
        Renderer3D::end(this->drawBuffer);

        // Update the editor camera.
        this->editorCam->onUpdate(dt);
        break;
      }

      case SceneState::Play:
      {
        // Update the scene.
        this->currentScene->onUpdateRuntime(dt);

        // Fetch the primary camera entity.
        auto primaryCameraEntity = this->currentScene->getPrimaryCameraEntity();
        Camera primaryCamera;
        if (primaryCameraEntity)
        {
          primaryCamera = primaryCameraEntity.getComponent<CameraComponent>().entCamera;

          float ratio = this->editorSize.x / this->editorSize.y;
          primaryCamera.projection = glm::perspective(primaryCamera.fov,
                                                      ratio, primaryCamera.near,
                                                      primaryCamera.far);
          primaryCamera.invViewProj = glm::inverse(primaryCamera.projection * primaryCamera.view);
        }
        else
        {
          primaryCamera = (Camera)(*this->editorCam.get());
          this->editorCam->onUpdate(dt);
        }

        this->drawBuffer->clear();
        Renderer3D::begin(this->editorSize.x, this->editorSize.y, primaryCamera);
        this->currentScene->onRenderRuntime();
        Renderer3D::end(this->drawBuffer);
        break;
      }
    }
  }

  void
  EditorLayer::onImGuiRender()
  {
    Logger* logs = Logger::getInstance();

    // Get the window size.
    ImVec2 wSize = ImGui::GetIO().DisplaySize;

    static bool dockspaceOpen = true;

    // Seting up the dockspace.
    // Set the size of the full screen ImGui window to the size of the viewport.
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    // Window flags to prevent the docking context from taking up visible space.
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
    windowFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    // Dockspace flags.
    ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;

    // Remove window sizes and create the window.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Demo", &dockspaceOpen, windowFlags);
		ImGui::PopStyleVar(3);

    // Prepare the dockspace context.
    ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		float minWinSizeX = style.WindowMinSize.x;
		style.WindowMinSize.x = 370.0f;
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
			ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);
		}

		style.WindowMinSize.x = minWinSizeX;

    // On ImGui render methods for all the GUI elements.
    for (auto& window : this->windows)
      if (window->isOpen)
        window->onImGuiRender(window->isOpen, this->currentScene);

  	if (ImGui::BeginMainMenuBar())
  	{
    	if (ImGui::BeginMenu("File"))
    	{
       	if (ImGui::MenuItem(ICON_FA_FILE_O" New", "Ctrl+N"))
       	{
          auto storage = Renderer3D::getStorage();
          storage->currentEnvironment->unloadEnvironment();

          this->currentScene = createShared<Scene>();
       	}
        if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN_O" Open...", "Ctrl+O"))
       	{
          EventDispatcher* dispatcher = EventDispatcher::getInstance();
          dispatcher->queueEvent(new OpenDialogueEvent(DialogueEventType::FileOpen,
                                                       ".srn"));
          this->loadTarget = FileLoadTargets::TargetScene;
       	}
        if (ImGui::MenuItem(ICON_FA_FLOPPY_O" Save", "Ctrl+S"))
        {
          if (this->currentScene->getSaveFilepath() != "")
          {
            std::string path =  this->currentScene->getSaveFilepath();
            std::string name = path.substr(path.find_last_of('/') + 1, path.find_last_of('.'));

            YAMLSerialization::serializeScene(this->currentScene, path, name);
          }
          else
          {
            EventDispatcher* dispatcher = EventDispatcher::getInstance();
            dispatcher->queueEvent(new OpenDialogueEvent(DialogueEventType::FileSave,
                                                         ".srn"));
            this->saveTarget = FileSaveTargets::TargetScene;
          }
        }
        if (ImGui::MenuItem(ICON_FA_FLOPPY_O" Save As", "Ctrl+Shift+S"))
       	{
          EventDispatcher* dispatcher = EventDispatcher::getInstance();
          dispatcher->queueEvent(new OpenDialogueEvent(DialogueEventType::FileSave,
                                                       ".srn"));
          this->saveTarget = FileSaveTargets::TargetScene;
       	}
        if (ImGui::MenuItem(ICON_FA_POWER_OFF" Exit"))
        {
          EventDispatcher* appEvents = EventDispatcher::getInstance();
          appEvents->queueEvent(new WindowCloseEvent());
        }
       	ImGui::EndMenu();
     	}

      if (ImGui::BeginMenu("Edit"))
      {
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Add"))
      {
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Scripts"))
      {
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Settings"))
      {
        if (ImGui::BeginMenu("Menus"))
        {
          if (ImGui::BeginMenu("Scene Menu Settings"))
          {
            if (ImGui::MenuItem("Show Scene Graph"))
            {
              this->windows[SCENE_GRAPH]->isOpen = true;
            }

            if (ImGui::MenuItem("Show Model Information"))
            {
              this->windows[MODEL_INFO]->isOpen = true;
            }

            ImGui::EndMenu();
          }

          if (ImGui::BeginMenu("Editor Menu Settings"))
          {
            if (ImGui::MenuItem("Show Content Browser"))
            {
              this->windows[CONTENT_BROWSER]->isOpen = true;
            }

            if (ImGui::MenuItem("Show Performance Stats Menu"))
            {
              this->showPerf = true;
            }

            if (ImGui::MenuItem("Show Camera Menu"))
            {
              this->windows[CAMERA_MENU]->isOpen = true;
            }

            if (ImGui::MenuItem("Show Shader Menu"))
            {
              this->windows[SHADER_MENU]->isOpen = true;
            }

            ImGui::EndMenu();
          }

          if (ImGui::MenuItem("Show Renderer Settings"))
          {
            this->windows[RENDERER_SETTINGS]->isOpen = true;
          }

          ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("System Settings"))
        {
            if (ImGui::MenuItem("Keybindings"))
            {
                this->windows[APP_SETTINGS_WINDOW]->isOpen = true;
            }
            if (ImGui::MenuItem("Save Settings"))
            {
                saveWindows();
                editorCam->saveSettings();
                YAMLSerialization::serializeAppStatus(editorStatus, CONFIG_FILEPATH);
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Help"))
      {
        ImGui::EndMenu();
      }
     	ImGui::EndMainMenuBar();
  	}

    // The log menu.
    ImGui::Begin("Application Logs", nullptr);
    {
      if (ImGui::Button("Clear Logs"))
        Logger::getInstance()->getLogs() = "";

      ImGui::BeginChild("LogText");
      {
        auto size = ImGui::GetWindowSize();
        ImGui::PushTextWrapPos(size.x);
        ImGui::Text(Logger::getInstance()->getLogs().c_str());
        ImGui::PopTextWrapPos();
      }
      ImGui::EndChild();
    }
    ImGui::End();

    // The performance window.
    if (this->showPerf)
    {
      ImGui::Begin("Performance Window", &this->showPerf);
      auto size = ImGui::GetWindowSize();
      ImGui::PushTextWrapPos(size.x);
      ImGui::Text(Application::getInstance()->getWindow()->getContextInfo().c_str());
      ImGui::Text("Application averaging %.3f ms/frame (%.1f FPS)",
                  1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      ImGui::PopTextWrapPos();
      ImGui::End();
    }

    // Toolbar window.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    auto& colors = ImGui::GetStyle().Colors;
    const auto& buttonHovered = colors[ImGuiCol_ButtonHovered];
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonHovered.x, buttonHovered.y, buttonHovered.z, 0.5f));
    const auto& buttonActive = colors[ImGuiCol_ButtonActive];
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(buttonActive.x, buttonActive.y, buttonActive.z, 0.5f));

    auto icon = this->sceneState == SceneState::Edit ? ICON_FA_PLAY : ICON_FA_STOP;
    ImGui::Begin("##buttonBar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    float size = ImGui::GetWindowHeight() - 4.0f;
    ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - (size * 0.5f));
    if (ImGui::Button(icon, ImVec2(size, size)))
    {
      if (this->sceneState == SceneState::Edit)
        this->onScenePlay();
      else if (this->sceneState == SceneState::Play)
        this->onSceneStop();
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
    ImGui::End();

    // Show a warning when a new scene is to be loaded.
    if (this->dndScenePath != "" && this->currentScene->getRegistry().size() > 0)
    {
      auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

      ImGui::Begin("Warning", nullptr, flags);

      ImGui::Text("Loading a new scene will overwrite the current scene, do you wish to continue?");
      ImGui::Text(" ");

      auto cursor = ImGui::GetCursorPos();
      ImGui::SetCursorPos(ImVec2(cursor.x + 90.0f, cursor.y));
      if (ImGui::Button("Save and Continue"))
      {
        if (this->currentScene->getSaveFilepath() != "")
        {
          std::string path =  this->currentScene->getSaveFilepath();
          std::string name = path.substr(path.find_last_of('/') + 1, path.find_last_of('.'));

          YAMLSerialization::serializeScene(this->currentScene, path, name);

          static_cast<SceneGraphWindow*>(this->windows[0])->setSelectedEntity(Entity());
          static_cast<ModelWindow*>(this->windows[4])->setSelectedEntity(Entity());

          Shared<Scene> tempScene = createShared<Scene>();
          if (YAMLSerialization::deserializeScene(tempScene, this->dndScenePath))
          {
            this->currentScene = tempScene;
            this->currentScene->getSaveFilepath() = this->dndScenePath;
          }
          this->dndScenePath = "";
        }
        else
        {
          EventDispatcher* dispatcher = EventDispatcher::getInstance();
          dispatcher->queueEvent(new OpenDialogueEvent(DialogueEventType::FileSave,
                                                       ".srn"));
          this->saveTarget = FileSaveTargets::TargetScene;
        }
      }

      ImGui::SameLine();
      if (ImGui::Button("Continue"))
      {
        static_cast<SceneGraphWindow*>(this->windows[0])->setSelectedEntity(Entity());
        static_cast<ModelWindow*>(this->windows[4])->setSelectedEntity(Entity());

        Shared<Scene> tempScene = createShared<Scene>();
        if (YAMLSerialization::deserializeScene(tempScene, this->dndScenePath))
        {
          this->currentScene = tempScene;
          this->currentScene->getSaveFilepath() = this->dndScenePath;
        }
        this->dndScenePath = "";
      }

      ImGui::SameLine();
      if (ImGui::Button("Cancel"))
        this->dndScenePath = "";

      ImGui::End();
    }
    else if (this->dndScenePath != "")
    {
      static_cast<SceneGraphWindow*>(this->windows[0])->setSelectedEntity(Entity());
      static_cast<ModelWindow*>(this->windows[4])->setSelectedEntity(Entity());

      Shared<Scene> tempScene = createShared<Scene>();
      if (YAMLSerialization::deserializeScene(tempScene, this->dndScenePath))
      {
        this->currentScene = tempScene;
        this->currentScene->getSaveFilepath() = this->dndScenePath;
      }
      this->dndScenePath = "";
    }

    // MUST KEEP THIS. Docking window end.
    ImGui::End();
  }

  void EditorLayer::saveWindows()
  {
      for (GuiWindow* window : windows) {
          if (editorStatus.windows.find(window->name) != editorStatus.windows.end())
              editorStatus.windows[window->name] = window->isOpen;
          else
              editorStatus.windows.insert(std::pair{ window->name, window->isOpen });
      }
  }

  void
  EditorLayer::onKeyPressEvent(KeyPressedEvent &keyEvent)
  {
    // Fetch the application window for input polling.
    Shared<Window> appWindow = Application::getInstance()->getWindow();

    int keyCode = keyEvent.getKeyCode();

    bool camStationary = this->editorCam->isStationary();
    bool lControlHeld = appWindow->isKeyPressed(editorStatus.keyCodes["KEY_LEFT_CONTROL"]);
    bool lShiftHeld = appWindow->isKeyPressed(editorStatus.keyCodes["KEY_LEFT_SHIFT"]);

    if (keyCode == editorStatus.keyCodes["KEY_N"]) 
    {
        if (lControlHeld && camStationary)
        {
            auto storage = Renderer3D::getStorage();
            storage->currentEnvironment->unloadEnvironment();

            this->currentScene = createShared<Scene>();
            static_cast<SceneGraphWindow*>(this->windows[0])->setSelectedEntity(Entity());
            static_cast<ModelWindow*>(this->windows[4])->setSelectedEntity(Entity());
        }
    }
    else if (keyCode == editorStatus.keyCodes["KEY_O"])
    {
        if (lControlHeld && camStationary)
        {
            EventDispatcher* dispatcher = EventDispatcher::getInstance();
            dispatcher->queueEvent(new OpenDialogueEvent(DialogueEventType::FileOpen,
                ".srn"));
            this->loadTarget = FileLoadTargets::TargetScene;
        }
    }
    else if (keyCode == editorStatus.keyCodes["KEY_S"])
    {
        if (lControlHeld && lShiftHeld && camStationary)
        {
            EventDispatcher* dispatcher = EventDispatcher::getInstance();
            dispatcher->queueEvent(new OpenDialogueEvent(DialogueEventType::FileSave,
                ".srn"));
            this->saveTarget = FileSaveTargets::TargetScene;
        }
        else if (lControlHeld && camStationary)
        {
            if (this->currentScene->getSaveFilepath() != "")
            {
                std::string path = this->currentScene->getSaveFilepath();
                std::string name = path.substr(path.find_last_of('/') + 1, path.find_last_of('.'));

                YAMLSerialization::serializeScene(this->currentScene, path, name);
            }
            else
            {
                EventDispatcher* dispatcher = EventDispatcher::getInstance();
                dispatcher->queueEvent(new OpenDialogueEvent(DialogueEventType::FileSave,
                    ".srn"));
                this->saveTarget = FileSaveTargets::TargetScene;
            }
        }
    }
  }

  void EditorLayer::onMouseEvent(MouseClickEvent &mouseEvent)
  {
    // Fetch the application window for input polling.
    Shared<Window> appWindow = Application::getInstance()->getWindow();

    int mouseCode = mouseEvent.getButton();

    bool camStationary = this->editorCam->isStationary();
    bool lControlHeld = appWindow->isKeyPressed(editorStatus.keyCodes["KEY_LEFT_CONTROL"]);
    bool lShiftHeld = appWindow->isKeyPressed(editorStatus.keyCodes["KEY_LEFT_SHIFT"]);

    switch (mouseCode)
    {
      default:
      {
        break;
      }
    }
  }

  void
  EditorLayer::onScenePlay()
  {
    this->sceneState = SceneState::Play;
  }

  void
  EditorLayer::onSceneStop()
  {
    this->sceneState = SceneState::Edit;
  }

  Entity
  EditorLayer::getSelectedEntity()
  {
    return static_cast<SceneGraphWindow*>(this->windows[0])->getSelectedEntity();
  }
}
