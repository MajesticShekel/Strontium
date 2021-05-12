#include "Graphics/GuiHandler.h"

// Project includes.
#include "Core/Logs.h"

namespace SciRenderer
{
  GuiHandler::GuiHandler(LightController* lights)
    : logBuffer("")
    , usePBR(true)
    , drawIrrad(false)
    , drawFilter(false)
    , mapRes(512)
    , currentLights(lights)
  {
    this->currentULName = "----";
    this->currentPLName = "----";
    this->currentSLName = "----";
    this->selectedULight = nullptr;
    this->selectedPLight = nullptr;
    this->selectedSLight = nullptr;
  }

  void
  GuiHandler::init(GLFWwindow *window)
  {
    // Dear Imgui init.
  	IMGUI_CHECKVERSION();
  	ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
  	ImGui::StyleColorsDark();
  	ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 440");
  }

  void
  GuiHandler::shutDown()
  {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
  	ImGui::DestroyContext();
  }

  void
  GuiHandler::drawGUI(FrameBuffer* frontBuffer, Camera* editorCamera,
                      EnvironmentMap* environment, GLFWwindow* window)
  {
    // Fetch the singleton logger.
    Logger* logs = Logger::getInstance();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Get the window size.
    ImVec2 wSize = ImGui::GetIO().DisplaySize;

    bool openObjMenu = false;
    bool openEnvironment = false;

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
          glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
       	ImGui::EndMenu();
     	}
     	ImGui::EndMainMenuBar();
  	}

    // The editor viewport.
    ImGui::SetNextWindowPos(ImVec2(wSize.x * 16 / 64, 19));
    ImGui::SetNextWindowSize(ImVec2(wSize.x - wSize.x * 32 / 64,
                                    wSize.y - wSize.x * 16 / 128),
                                    ImGuiCond_Always);
    ImGui::Begin("Editor Viewport", nullptr, this->editorFlags);
    {
      ImGui::BeginChild("EditorRender");
        {
          ImVec2 editorSize = ImGui::GetWindowSize();
          ImGui::Image((ImTextureID) frontBuffer->getAttachID(FBOTargetParam::Colour0), editorSize,
                       ImVec2(0, 1), ImVec2(1, 0));
        }
      ImGui::EndChild();
    }
    ImGui::End();

  	// Left sidebar.
  	ImGui::SetNextWindowPos(ImVec2(0, 19));
  	ImGui::SetNextWindowSize(ImVec2(wSize.x * 16 / 64, wSize.y), ImGuiCond_Always);
  	ImGui::Begin("Left Sidebar", nullptr, this->sidebarFlags);
    {
      // Left sidebar contents.
      if (ImGui::CollapsingHeader("Performance", ImGuiTreeNodeFlags_DefaultOpen))
      {
    	  ImGui::Text("Application averaging %.3f ms/frame (%.1f FPS)",
    							  1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      }

      if (ImGui::CollapsingHeader("Lighting"))
      {
        this->lightingMenu();
      }

      if (ImGui::CollapsingHeader("Models"))
      {

      }

      if (ImGui::CollapsingHeader("2D Textures"))
      {

      }

      if (ImGui::CollapsingHeader("Environment Maps"))
      {
        if (ImGui::Button("Load Equirectangular Map"))
        {
          openEnvironment = true;
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear Environment Map"))
        {
          environment->unloadEnvironment();
          this->drawIrrad = false;
          this->drawFilter = false;
          environment->setDrawingType(MapType::Skybox);
        }

        if (environment->hasEqrMap() && !environment->hasSkybox())
        {
          ImGui::Text("Preview:");
          ImGui::Image((ImTextureID) environment->getTexID(MapType::Equirectangular),
                       ImVec2(360, 180), ImVec2(0, 1), ImVec2(1, 0));
          ImGui::SliderInt("Environment Resolution", &this->mapRes, 512, 2048);
          if (ImGui::Button("Generate Skybox"))
          {
            environment->equiToCubeMap(true, this->mapRes, this->mapRes);
          }
        }

        if (environment->hasSkybox())
        {
          ImGui::SliderFloat("Gamma", &environment->getGamma(), 1.0f, 5.0f);
          ImGui::SliderFloat("Exposure", &environment->getExposure(), 1.0f, 5.0f);

          if (!environment->hasIrradiance())
          {
            if (ImGui::Button("Generate Irradiance Map"))
            {
              environment->precomputeIrradiance(256, 256, true);
            }
          }
          else
          {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            ImGui::Button("Generate Irradiance Map");
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();

            ImGui::Checkbox("Draw Irradiance Map", &this->drawIrrad);
            if (this->drawIrrad && !environment->drawingFilter())
            {
              environment->setDrawingType(MapType::Irradiance);
            }
            else if (!this->drawIrrad && !environment->drawingFilter())
            {
              environment->setDrawingType(MapType::Skybox);
            }
            else
            {
              this->drawIrrad = false;
            }
          }

          if (!environment->hasPrefilter())
          {
            if (ImGui::Button("Generate BRDF Specular Map"))
            {
              environment->precomputeSpecular(this->mapRes, this->mapRes);
            }
          }
          else
          {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            ImGui::Button("Generate BRDF Specular Map");
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();

            ImGui::Checkbox("Draw Pre-Filter Map", &this->drawFilter);
            if (this->drawFilter && !environment->drawingIrrad())
            {
              environment->setDrawingType(MapType::Prefilter);
              ImGui::SliderFloat("Roughness", &environment->getRoughness(), 0.0f, 1.0f);
            }
            else if (!this->drawFilter && !environment->drawingIrrad())
            {
              environment->setDrawingType(MapType::Skybox);
            }
            else
            {
              this->drawFilter = false;
            }
          }

          if (environment->hasIntegration())
          {
            ImGui::Text("BRDF Lookup Texture:");
            ImGui::Image((ImTextureID) environment->getTexID(MapType::Integration),
                         ImVec2(128, 128), ImVec2(0, 1), ImVec2(1, 0));
          }
        }
      }
    }
  	ImGui::End();

    // The right sidebar.
    ImGui::SetNextWindowPos(ImVec2(wSize.x - wSize.x * 16 / 64, 19));
  	ImGui::SetNextWindowSize(ImVec2(wSize.x * 16 / 64, wSize.y), ImGuiCond_Always);
  	ImGui::Begin("Right Sidebar", nullptr, this->sidebarFlags);
    {
      // Right sidebar contents.
    }
    ImGui::End();

    // The log menu.
    this->logBuffer += logs->getLastMessages();
    ImGui::SetNextWindowPos(ImVec2(wSize.x * 16 / 64, wSize.y - wSize.x * 15 / 128 + 2));
  	ImGui::SetNextWindowSize(ImVec2(wSize.x - wSize.x * 16 / 64 - wSize.x * 16 / 64,
                                    wSize.x * 16 / 128), ImGuiCond_Always);
    ImGui::Begin("Application Logs", nullptr, this->logFlags);
    {
      if (ImGui::Button("Clear Logs"))
        this->logBuffer = "";
      ImGui::BeginChild("LogText");
      {
        ImGui::Text(this->logBuffer.c_str());
      }
      ImGui::EndChild();
    }
    ImGui::End();

    if (openEnvironment)
      ImGui::OpenPopup("Load Equirectangular Map");

    if (this->fileHandler.showFileDialog("Load Equirectangular Map",
        imgui_addons::ImGuiFileBrowser::DialogMode::OPEN, ImVec2(700, 310), ".hdr"))
    {
      environment->loadEquirectangularMap(this->fileHandler.selected_path);
    }

    if (openObjMenu)
      ImGui::OpenPopup("Load Obj File");

    if (this->fileHandler.showFileDialog("Load Obj File",
        imgui_addons::ImGuiFileBrowser::DialogMode::OPEN, ImVec2(700, 310), ".obj"))
    {
      std::cout << this->fileHandler.selected_path << std::endl;
    }

  	ImGui::Render();
  	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    auto bSize = frontBuffer->getSize();
    if (bSize.x != wSize.x - wSize.x * 32 / 64 || bSize.y != wSize.y - wSize.x * 16 / 128)
    {
      frontBuffer->resize(wSize.x - wSize.x * 32 / 64, wSize.y - wSize.x * 16 / 128);
      GLfloat ratio = 1.0f * (wSize.x - wSize.x * 32 / 64) / (wSize.y - wSize.x * 16 / 128);
      editorCamera->updateProj(90.0f, ratio, 0.1f, 50.0f);
    }
  }

  void
  GuiHandler::lightingMenu()
  {
    // Get the gui labels for the lights.
    this->uLightNames = this->currentLights->getGuiLabel(UNIFORM);
    this->pLightNames = this->currentLights->getGuiLabel(POINT);
    this->sLightNames = this->currentLights->getGuiLabel(SPOT);

    // The lighting window.
  	ImGui::Text("Total of %d lightcaster(s)", this->currentLights->getNumLights(ALL));
    ImGui::Checkbox("Use PBR Pipeline", &this->usePBR);
  	ImGui::ColorEdit3("Ambient colour", &this->currentLights->getAmbient()->x);

  	// Dropdown box for uniform lights.
  	if (ImGui::BeginCombo("Uniform lights", this->currentULName))
  	{
  		for (unsigned i = 0; i < this->uLightNames.size(); i++)
  		{
  			bool isLightSelected = (this->currentULName == this->uLightNames[i].c_str());
  			if (ImGui::Selectable(this->uLightNames[i].c_str(), isLightSelected))
  			{
  					this->currentULName = this->uLightNames[i].c_str();
  					if (i - 1 < 0)
  						this->selectedULight = nullptr;
  					else
  						this->selectedULight = this->currentLights->getULight(i - 1);
  			}
  			if (isLightSelected)
  					ImGui::SetItemDefaultFocus();
  		}
  		ImGui::EndCombo();
  	}
  	// Menu to modify the uniform light properties.
  	if (this->selectedULight != nullptr)
  	{
  		ImGui::SliderFloat3((this->selectedULight->name + std::string(" direction")).c_str(),
                          &(this->selectedULight->direction.x), -1.0f, 1.0f);
  		ImGui::ColorEdit3((this->selectedULight->name + std::string(" colour")).c_str(),
                        &(this->selectedULight->colour.x));
  		ImGui::SliderFloat((this->selectedULight->name + std::string(" intensity")).c_str(),
                        &(this->selectedULight->intensity), 0.0f, 1.0f);
      if (!this->usePBR)
      {
        ImGui::Text("%s properties:", this->selectedULight->name.c_str());
        ImGui::SliderFloat3((this->selectedULight->name + std::string(" diffuse")).c_str(),
                            &(this->selectedULight->mat.diffuse.x), 0.0f, 1.0f);
        ImGui::SliderFloat3((this->selectedULight->name + std::string(" specular")).c_str(),
                            &(this->selectedULight->mat.specular.x), 0.0f, 1.0f);
        ImGui::SliderFloat((this->selectedULight->name + std::string(" shininess")).c_str(),
                            &(this->selectedULight->mat.shininess), 1.0f, 128.0f);
      }

  		if (ImGui::Button((std::string("Delete ") + this->selectedULight->name).c_str()))
  		{
  			this->currentLights->deleteLight(UNIFORM, this->selectedULight->lightID);
  			this->selectedULight = nullptr;
  			this->currentULName = "----";
  			this->currentLights->setGuiLabel(UNIFORM);
  		}
  	}
  	// Dropdown box for point lights.
  	if (ImGui::BeginCombo("Point lights", this->currentPLName))
  	{
  		for (unsigned i = 0; i < this->pLightNames.size(); i++)
  		{
  			bool isLightSelected = (this->currentPLName == this->pLightNames[i].c_str());
  			if (ImGui::Selectable(this->pLightNames[i].c_str(), isLightSelected))
  			{
  					this->currentPLName = this->pLightNames[i].c_str();
  					if (i - 1 < 0)
  						this->selectedPLight = nullptr;
  					else
  						this->selectedPLight = this->currentLights->getPLight(i - 1);
  			}
  			if (isLightSelected)
  					ImGui::SetItemDefaultFocus();
  		}
  		ImGui::EndCombo();
  	}
  	// Menu to modify point light properties.
  	if (this->selectedPLight != nullptr)
  	{
  		ImGui::SliderFloat3((this->selectedPLight->name + std::string(" position")).c_str(),
                          &(this->selectedPLight->position.x), -10.0f, 10.0f);
  		ImGui::ColorEdit3((this->selectedPLight->name + std::string(" colour")).c_str(),
                        &(this->selectedPLight->colour.x));
  		ImGui::SliderFloat((this->selectedPLight->name + std::string(" intensity")).c_str(),
                         &(this->selectedPLight->intensity), 0.0f, 1.0f);
  		ImGui::SliderFloat((this->selectedPLight->name + std::string(" mesh scale")).c_str(),
                         &(this->selectedPLight->meshScale), 0.0f, 1.0f);
      ImGui::Text("%s properties:", this->selectedPLight->name.c_str());
      if (!this->usePBR)
      {
        ImGui::SliderFloat3((this->selectedPLight->name + std::string(" diffuse")).c_str(),
                            &(this->selectedPLight->mat.diffuse.x), 0.0f, 1.0f);
        ImGui::SliderFloat3((this->selectedPLight->name + std::string(" specular")).c_str(),
                            &(this->selectedPLight->mat.specular.x), 0.0f, 1.0f);
        ImGui::SliderFloat((this->selectedPLight->name + std::string(" shininess")).c_str(),
                           &(this->selectedPLight->mat.shininess), 1.0f, 128.0f);
      }
      ImGui::SliderFloat2((this->selectedPLight->name + std::string(" attenuation")).c_str(),
                          &(this->selectedPLight->mat.attenuation.x), 0.0f, 1.0f);

  		if (ImGui::Button((std::string("Delete ") + this->selectedPLight->name).c_str()))
  		{
  			this->currentLights->deleteLight(POINT, this->selectedPLight->lightID);
  			this->selectedPLight = nullptr;
  			this->currentPLName = "----";
  			this->currentLights->setGuiLabel(POINT);
  		}
  	}
  	// Dropdown box for spot lights.
  	if (ImGui::BeginCombo("Spotlights", this->currentSLName))
  	{
  		for (unsigned i = 0; i < this->sLightNames.size(); i++)
  		{
  			bool isLightSelected = (this->currentSLName == this->sLightNames[i].c_str());
  			if (ImGui::Selectable(this->sLightNames[i].c_str(), isLightSelected))
  			{
  					this->currentSLName = this->sLightNames[i].c_str();
  					if (i - 1 < 0)
  						this->selectedSLight = nullptr;
  					else
  						this->selectedSLight = this->currentLights->getSLight(i - 1);
  			}
  			if (isLightSelected)
  					ImGui::SetItemDefaultFocus();
  		}
  		ImGui::EndCombo();
  	}
  	// Menu to modify spot light properties.
  	if (selectedSLight != nullptr)
  	{
  		ImGui::SliderFloat3((this->selectedSLight->name + std::string(" position")).c_str(),
                          &(this->selectedSLight->position.x), -10.0f, 10.0f);
  		ImGui::SliderFloat3((this->selectedSLight->name + std::string(" direction")).c_str(),
                          &(this->selectedSLight->direction.x), -1.0f, 1.0f);
  		ImGui::ColorEdit3((this->selectedSLight->name + std::string(" colour")).c_str(),
                        &(this->selectedSLight->colour.x));
  		ImGui::SliderFloat((this->selectedSLight->name + std::string(" intensity")).c_str(),
                         &(this->selectedSLight->intensity), 0.0f, 1.0f);
  		ImGui::SliderFloat((this->selectedSLight->name + std::string(" inner cutoff")).c_str(),
                         &(this->selectedSLight->innerCutOff), 0.0f, 1.0f);
  		ImGui::SliderFloat((this->selectedSLight->name + std::string(" outer cutoff")).c_str(),
                         &(this->selectedSLight->outerCutOff), 0.0f, 1.0f);
  		ImGui::SliderFloat((this->selectedSLight->name + std::string(" mesh scale")).c_str(),
                         &(this->selectedSLight->meshScale), 0.0f, 1.0f);
      ImGui::Text("%s properties:", this->selectedSLight->name.c_str());
      if (!this->usePBR)
      {
        ImGui::SliderFloat3((this->selectedSLight->name + std::string(" diffuse")).c_str(),
                            &(this->selectedSLight->mat.diffuse.x), 0.0f, 1.0f);
        ImGui::SliderFloat3((this->selectedSLight->name + std::string(" specular")).c_str(),
                            &(this->selectedSLight->mat.specular.x), 0.0f, 1.0f);
        ImGui::SliderFloat((this->selectedSLight->name + std::string(" shininess")).c_str(),
                           &(this->selectedSLight->mat.shininess), 1.0f, 128.0f);
      }
      ImGui::SliderFloat2((this->selectedSLight->name + std::string(" attenuation")).c_str(),
                          &(this->selectedSLight->mat.attenuation.x), 0.0f, 1.0f);

  		if (ImGui::Button((std::string("Delete ") + this->selectedSLight->name).c_str()))
  		{
  			this->currentLights->deleteLight(SPOT, this->selectedSLight->lightID);
  			this->selectedSLight = nullptr;
  			this->currentSLName = "----";
  			this->currentLights->setGuiLabel(SPOT);
  		}
  	}
  	// Buttons to create lights.
  	if (ImGui::Button("New uniform light"))
  	{
  		UniformLight temp = UniformLight();
  		this->currentLights->addLight(temp);
  	}
  	ImGui::SameLine();
  	if (ImGui::Button("New point light"))
  	{
  		PointLight temp = PointLight();
  		this->currentLights->addLight(temp, 0.1f);
  	}
  	ImGui::SameLine();
  	if (ImGui::Button("New spotlight"))
  	{
  		SpotLight temp = SpotLight();
  		this->currentLights->addLight(temp, 0.1f);
  	}
  }

  void
  GuiHandler::loadObjMenu()
  {

  }

  void
  GuiHandler::modelMenu()
  {

  }
}