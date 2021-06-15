#pragma once

// Macro include file.
#include "SciRenderPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/Events.h"
#include "Core/AssetManager.h"
#include "Layers/Layers.h"
#include "Graphics/GraphicsSystem.h"
#include "Scenes/Scene.h"
#include "Scenes/Entity.h"
#include "GuiElements/GuiWindow.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imguibrowser/FileBrowser/ImGuiFileBrowser.h"

namespace SciRenderer
{
  class EditorLayer : public Layer
  {
  public:
    EditorLayer();
    virtual ~EditorLayer();

    virtual void onAttach() override;
    virtual void onDetach() override;
    virtual void onImGuiRender() override;
    virtual void onEvent(Event &event) override;
    virtual void onUpdate(float dt) override;

  protected:
    void DNDTarget();
    void loadDNDAsset(const std::string &filepath);

    // The current scene.
    Shared<Scene> currentScene;
    // The framebuffer for the scene.
    Shared<FrameBuffer> drawBuffer;
    // Editor camera.
    Shared<Camera> editorCam;

    // The various external windows. Kinda janky.
    // TODO: use an unordered map?
    std::vector<std::pair<bool, GuiWindow*>> windows;

    // Stuff for ImGui and the GUI.
    bool showPerf;
    bool showSceneGraph;
    std::string logBuffer;
    ImVec2 editorSize;
  };
}
