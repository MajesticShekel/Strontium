#pragma once

#include <Core/ApplicationBase.h>

struct camData {
	glm::vec3 position, front;
	float fov, near, far, speed, sens;
};

struct appStatus {
	std::map<std::string, uint> keyCodes;
	std::map<std::string, bool> windows;
	camData camera;
};

// Keycodes. A 1-1 remapping of GLFW keycodes.
//extern std::map<std::string, uint> defaultKeyCodes;
extern appStatus defaultEditorStatus;

extern appStatus editorStatus;