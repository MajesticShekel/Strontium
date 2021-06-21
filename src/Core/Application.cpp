#include "Core/Application.h"

// Project includes.
#include "Core/Events.h"
#include "Core/Logs.h"

namespace SciRenderer
{
  Application* Application::appInstance = nullptr;

  // Singleton application class for everything that happens in SciRender.
  Application::Application(const std::string &name)
    : name(name)
    , running(true)
    , isMinimized(false)
    , lastTime(0.0f)
  {
    if (Application::appInstance != nullptr)
    {
      std::cout << "Already have an instance of the application. Aborting"
                << std::endl;
      assert(Application::appInstance != nullptr);
    }

    Application::appInstance = this;

    // Initialize the application logs.
    SciRenderer::Logger* logs = SciRenderer::Logger::getInstance();
    logs->init();

    // Initialize the application main window.
    this->appWindow = Window::getNewInstance(this->name);

    // Initialize the thread pool.
    workerGroup = Unique<ThreadPool>(ThreadPool::getInstance(4));

    // Initialize the 3D renderer.
    Renderer3D::init();

    // Initialize the asset managers.
    this->shaderCache.reset(AssetManager<Shader>::getManager());
    this->modelAssets.reset(AssetManager<Model>::getManager());
    this->texture2DAssets.reset(AssetManager<Texture2D>::getManager());

    // Load the shader into a cache and set the appropriate uniforms.
    Shader* program = new Shader ("./assets/shaders/mesh.vs",
                                  "./assets/shaders/pbr/pbrTex.fs");
    this->shaderCache->attachAsset("pbr_shader", program);

    Texture2D* defaultTex = Texture2D::createMonoColour(glm::vec4(1.0f, 0.0f, 1.0f, 1.0f), Texture2DParams(), false);
    this->texture2DAssets->setDefaultAsset(defaultTex);

    this->imLayer = new ImGuiLayer();
    this->pushOverlay(this->imLayer);
  }

  Application::~Application()
  {
    // Detach each layer and delete it.
    for (auto layer : this->layerStack)
		{
  		layer->onDetach();
  		delete layer;
		}

    // Shutdown the renderer.
    Renderer3D::shutdown();

    // Delete the application event dispatcher and logs.
    delete EventDispatcher::getInstance();
    delete Logger::getInstance();
  }

  void
  Application::pushLayer(Layer* layer)
  {
    this->layerStack.pushLayer(layer);
    layer->onAttach();
  }

  void
  Application::pushOverlay(Layer* overlay)
  {
    this->layerStack.pushOverlay(overlay);
    overlay->onAttach();
  }

  void
  Application::close()
  {
    this->running = false;
  }

  // The main application run function with the run loop.
  void Application::run()
  {
    while (this->running)
    {
      // Fetch delta time.
      float currentTime = glfwGetTime();
      float deltaTime = currentTime - this->lastTime;
      this->lastTime = currentTime;

      // Make sure the window isn't minimized to avoid losing performance.
      if (!this->isMinimized)
      {
        // Loop over each layer and call its update function.
        for (auto layer : this->layerStack)
          layer->onUpdate(deltaTime);

        // Setup ImGui for drawing, than loop over each layer and draw its GUI
        // elements.
        this->imLayer->beginImGui();
        for (auto layer : this->layerStack)
          layer->onImGuiRender();

        this->imLayer->endImGui();

        // Handle application events
        this->dispatchEvents();

        // Update the window.
        this->appWindow->onUpdate();

        // Clear the back buffer.
        RendererCommands::clear(true, false, false);
      }

      // Must be called at the end of every frame to create textures with loaded
      // images.
      Texture2D::bulkGenerateTextures();
    }
  }

  void
  Application::dispatchEvents()
  {
    // Fetch the dispatcher.
    EventDispatcher* appEvents = EventDispatcher::getInstance();

    while (!appEvents->isEmpty())
    {
      // Fetch the first event.
      Event* event = appEvents->dequeueEvent();

      // Call the application on event function first.
      this->onEvent(*event);

      // Call the on event functions for each layer.
      for (auto layer : this->layerStack)
        layer->onEvent(*event);

      // Delete the event when we're done with it.
      Event::deleteEvent(event);
    }
  }

  void
  Application::onEvent(Event &event)
  {
    if (event.getType() == EventType::WindowResizeEvent)
      this->onWindowResize();

    if (event.getType() == EventType::WindowCloseEvent)
      this->close();
  }

  void
  Application::onWindowResize()
  {
    glm::ivec2 windowSize = getWindow()->getSize();

    if (windowSize.x == 0 || windowSize.y == 0)
      this->isMinimized = true;
    else
      this->isMinimized = false;
  }
}
