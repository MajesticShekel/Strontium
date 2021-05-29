#include "Layers/EditorLayer.h"

// Project includes.
#include "Core/Application.h"
#include "Core/Logs.h"
#include "GuiElements/Styles.h"
#include "GuiElements/SceneGraphWindow.h"
#include "GuiElements/CameraWindow.h"

namespace SciRenderer
{
  EditorLayer::EditorLayer()
    : Layer("Editor Layer")
    , logBuffer("")
  {
    // TODO: Serialize these into application settings. YAML.cpp?
    this->showPerf = true;
    this->editorSize = ImVec2(0, 0);
  }

  EditorLayer::~EditorLayer()
  {
    for (auto& pair : this->windows)
      delete pair.second;
  }

  void
  EditorLayer::onAttach()
  {
    // Fetch the asset caches and managers.
    auto shaderCache = AssetManager<Shader>::getManager();

    Styles::setDefaultTheme();

    // Fetch the width and height of the window and create a floating point
    // framebuffer.
    glm::ivec2 wDims = Application::getInstance()->getWindow()->getSize();
    this->drawBuffer = createShared<FrameBuffer>((GLuint) wDims.x, (GLuint) wDims.y);

    // Fetch a default floating point FBO spec and attach it.
    auto cSpec = FBOCommands::getFloatColourSpec(FBOTargetParam::Colour0);
    this->drawBuffer->attachTexture2D(cSpec);
  	this->drawBuffer->attachRenderBuffer();

    // Load the shader into a cache and set the appropriate uniforms.
    Shader* program = new Shader ("./res/shaders/mesh.vs",
                                  "./res/shaders/pbr/pbr.fs");
    shaderCache->attachAsset("pbr_shader", program);
  	program->addUniformSampler2D("irradianceMap", 0);
  	program->addUniformSampler2D("reflectanceMap", 1);
  	program->addUniformSampler2D("brdfLookUp", 2);

    // Setup stuff for the scene.
    this->currentScene = createShared<Scene>();
    auto ambient = this->currentScene->createEntity("Ambient Light");
    ambient.addComponent<AmbientComponent>("./res/textures/hdr_environments/pink_sunrise_4k.hdr");

    // Finally, the editor camera.
    this->editorCam = createShared<Camera>(1920 / 2, 1080 / 2, glm::vec3 { 0.0f, 1.0f, 4.0f },
                                           EditorCameraType::Stationary);
    this->editorCam->init(90.0f, 1.0f, 0.1f, 200.0f);

    // All the windows!
    this->windows.push_back(std::make_pair(true, new SceneGraphWindow()));
    this->windows.push_back(std::make_pair(true, new CameraWindow(this->editorCam)));
  }

  void
  EditorLayer::onDetach()
  {

  }

  // On event for the layer.
  void
  EditorLayer::onEvent(Event &event)
  {
    this->editorCam->onEvent(event);

    for (auto& pair : this->windows)
      pair.second->onEvent(event);
  }

  // On update for the layer.
  void
  EditorLayer::onUpdate(float dt)
  {
    for (auto& pair : this->windows)
      pair.second->onUpdate(dt);

    // Get the renderer.
    Renderer3D* renderer = Renderer3D::getInstance();

    // Update the size of the framebuffer to fit the editor window.
    glm::vec2 size = this->drawBuffer->getSize();
    if (this->editorSize.x != size.x || this->editorSize.y != size.y)
    {
      if (this->editorSize.x >= 1.0f && this->editorSize.y >= 1.0f)
        this->editorCam->updateProj(90.0f, editorSize.x / editorSize.y, 0.1f, 30.0f);
      this->drawBuffer->resize(this->editorSize.x, this->editorSize.y);
    }

    //--------------------------------------------------------------------------
    // The drawing phase. TODO: Move the application to a deferred renderer.
    //--------------------------------------------------------------------------
    // Draw the scene.
    this->drawBuffer->clear();
    this->drawBuffer->bind();
    this->drawBuffer->setViewport();

    // Update the scene.
    currentScene->onUpdate(dt, this->editorCam);

    this->drawBuffer->unbind();

    //--------------------------------------------------------------------------
    // Update the editor camera.
    //--------------------------------------------------------------------------
    this->editorCam->onUpdate(dt);
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
    for (auto& pair : this->windows)
      if (pair.first == true)
        pair.second->onImGuiRender(pair.first, this->currentScene);

  	if (ImGui::BeginMainMenuBar())
  	{
    	if (ImGui::BeginMenu("File"))
    	{
       	if (ImGui::MenuItem("New Scene"))
       	{

       	}
        if (ImGui::MenuItem("Load Scene"))
       	{

       	}
        if (ImGui::MenuItem("Save Scene"))
       	{

       	}
        if (ImGui::MenuItem("Exit"))
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
          if (ImGui::MenuItem("Show Performance Stats Menu"))
          {
            this->showPerf = true;
          }
          if (ImGui::MenuItem("Show Scene Graph Menu"))
          {
            this->windows[0].first = true;
          }
          if (ImGui::MenuItem("Show Camera Menu"))
          {
            this->windows[1].first = true;
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

    // The editor viewport.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Editor Viewport", nullptr, ImGuiWindowFlags_NoCollapse);
    {
      ImGui::BeginChild("EditorRender");
        {
          this->editorSize = ImGui::GetWindowSize();
          ImGui::Image((ImTextureID) this->drawBuffer->getAttachID(FBOTargetParam::Colour0),
                       this->editorSize, ImVec2(0, 1), ImVec2(1, 0));
        }
      ImGui::EndChild();
    }
    ImGui::PopStyleVar(3);
    ImGui::End();

    // The log menu.
    this->logBuffer += logs->getLastMessages();
    ImGui::Begin("Application Logs", nullptr);
    {
      if (ImGui::Button("Clear Logs"))
      {
        this->logBuffer = "";
      }
      ImGui::BeginChild("LogText");
      {
        ImGui::Text(this->logBuffer.c_str());
      }
      ImGui::EndChild();
    }
    ImGui::End();

    // The performance window.
    if (this->showPerf)
    {
      ImGui::Begin("Performance Window", &this->showPerf);
      ImGui::Text(Application::getInstance()->getWindow()->getContextInfo().c_str());
      ImGui::Text("Application averaging %.3f ms/frame (%.1f FPS)",
                  1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      ImGui::End();
    }

    // MUST KEEP THIS. Docking window end.
    ImGui::End();
  }
}
