#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/Events.h"
#include "Graphics/EditorCamera.h"
#include "GuiElements/GuiWindow.h"
#include "Scenes/Scene.h"

// ImGui includes.
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

// STL includes.
#include <filesystem>

namespace Strontium
{
    class AppSettingsWindow : public GuiWindow
    {
    public:
        AppSettingsWindow(EditorLayer* parentLayer, bool isOpen = false);
        ~AppSettingsWindow();

        void onImGuiRender(bool& isOpen, Shared<Scene> activeScene);
        void onUpdate(float dt, Shared<Scene> activeScene);
        void onEvent(Event& event);

    private:
        short environment = 0;
    };
}
