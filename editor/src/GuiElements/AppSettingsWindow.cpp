#include "GuiElements/AppSettingsWindow.h"

#include "Serialization/YamlSerialization.h"

#define ENV_APP 0
#define ENV_TEST 1

namespace Strontium
{
	AppSettingsWindow::AppSettingsWindow(EditorLayer* parentLayer, bool isOpen)
		: GuiWindow(parentLayer)
	{
		this->isOpen = isOpen;
		name = "AppSettingsWindow";
	}

	AppSettingsWindow::~AppSettingsWindow()
	{

	}

	void AppSettingsWindow::onImGuiRender(bool& isOpen, Shared<Scene> activeScene)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("App Settings", &isOpen, ImGuiWindowFlags_MenuBar);

		ImGui::PopStyleVar();
		
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Load..", "Ctrl+O"))
				{
					YAMLSerialization::deserializeAppStatus(editorStatus, CONFIG_FILEPATH);
				}
				if (ImGui::MenuItem("Save", "Ctrl+S"))
				{
					//TODO: how to save windows/cam here?
					YAMLSerialization::serializeAppStatus(editorStatus, CONFIG_FILEPATH);
				}
				if (ImGui::MenuItem("Close"))
					isOpen = false;
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		ImGui::Text("Environments:");

		ImGui::SameLine();
		if (ImGui::Button("Application"))
			environment = ENV_APP;
		ImGui::SameLine();
		if (ImGui::Button("Test"))
			environment = ENV_TEST;

		ImGui::Text("Keys:");
		ImGui::BeginChild("Scrolling");
		if (environment == ENV_APP)
		{
			for (const auto& pair : editorStatus.keyCodes)
				if (ImGui::Button(pair.first.c_str()))
					std::cout << "Resetting that key\n";
		}
		ImGui::EndChild();

		ImGui::End();
	}

	void AppSettingsWindow::onUpdate(float dt, Shared<Scene> activeScene)
	{

	}

	void AppSettingsWindow::onEvent(Event& event)
	{
		switch (event.getType())
		{
		default: break;
		}
	}

}