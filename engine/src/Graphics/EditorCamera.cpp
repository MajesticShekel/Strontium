#include "Graphics/EditorCamera.h"

// Project includes.
#include "Core/Logs.h"
#include "Core/Application.h"
#include "Core/Window.h"
#include "Core/AppStatus.h"

namespace Strontium
{
  EditorCamera::EditorCamera(float xCenter, float yCenter, EditorCameraType type)
    : position(glm::vec3 { 0.0f, 0.0f, 0.0f })
    , camFront(glm::vec3 { 0.0f, 0.0f, -1.0f })
    , camTop(glm::vec3 { 0.0f, 1.0f, 0.0f })
    , proj(glm::mat4(1.0f))
    , lastTime(0.0f)
    , lastMouseX(xCenter)
    , lastMouseY(yCenter)
    , yaw(-90.0f)
    , pitch(0.0f)
    , currentType(type)
    , firstClick(true)
  {
    this->view = glm::lookAt(this->position, this->position + this->camFront,
                             this->camTop);
  }

  EditorCamera::EditorCamera(float xCenter, float yCenter, const glm::vec3 &initPosition,
                             EditorCameraType type)
    : position(initPosition)
    , camFront(glm::vec3 { 0.0f, 0.0f, -1.0f })
    , camTop(glm::vec3 { 0.0f, 1.0f, 0.0f })
    , proj(glm::mat4(1.0f))
    , lastTime(0.0f)
    , lastMouseX(xCenter)
    , lastMouseY(yCenter)
    , yaw(-90.0f)
    , pitch(0.0f)
    , currentType(type)
    , firstClick(true)
  {
    this->view = glm::lookAt(this->position, this->position + this->camFront,
                             this->camTop);
  }

  void
  EditorCamera::init(const float &fov, const float &aspect,
                     const float &near, const float &far)
  {
    this->proj = glm::perspective(glm::radians(fov), aspect, near, far);
    switch (this->currentType)
    {
      case EditorCameraType::Free:
        Application::getInstance()->getWindow()->setCursorCapture(true);
        break;
      case EditorCameraType::Stationary:
        Application::getInstance()->getWindow()->setCursorCapture(false);
        break;
    }

    // Fetch the application window for input polling.
    Shared<Window> appWindow = Application::getInstance()->getWindow();
    glm::vec2 mousePos = appWindow->getCursorPos();
    this->lastMouseX = mousePos.x;
    this->lastMouseY = mousePos.y;

    this->horFOV = fov;
    this->near = near;
    this->far = far;
    this->aspect = aspect;
  }

  // On update function for the camera.
  void
  EditorCamera::onUpdate(float dt)
  {
    // Fetch the application window for input polling.
    Shared<Window> appWindow = Application::getInstance()->getWindow();

    glm::vec2 mousePos = appWindow->getCursorPos();

    switch (this->currentType)
    {
      case EditorCameraType::Stationary:
      {
        break;
      }
      case EditorCameraType::Free:
      {
        //----------------------------------------------------------------------
        // Handles the mouse input component.
        //----------------------------------------------------------------------
        float dx = mousePos.x - this->lastMouseX;
        float dy = this->lastMouseY - mousePos.y;

        // Compute the yaw and pitch from mouse position.
        this->yaw   += (this->sensitivity * dx);
        this->pitch += (this->sensitivity * dy);

        if (this->pitch > 89.0f)
          this-> pitch = 89.0f;
        if (this->pitch < -89.0f)
          this->pitch = -89.0f;

        // Compute the new front normal vector.
        glm::vec3 temp;
        temp.x         = cos(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));
        temp.y         = sin(glm::radians(this->pitch));
        temp.z         = sin(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));
        this->camFront = glm::normalize(temp);

        //----------------------------------------------------------------------
        // Handles the keyboard input component.
        //----------------------------------------------------------------------
        float cameraSpeed = this->scalarSpeed * dt;

        // Move the camera position (Space Engineers styled camera).
        if (appWindow->isKeyPressed(editorStatus.keyCodes["KEY_W"]))
          this->position += this->camFront * cameraSpeed;

        if (appWindow->isKeyPressed(editorStatus.keyCodes["KEY_S"]))
          this->position -= this->camFront * cameraSpeed;

        if (appWindow->isKeyPressed(editorStatus.keyCodes["KEY_A"]))
          this->position -= glm::normalize(glm::cross(this->camFront, this->camTop))
                            * cameraSpeed;

        if (appWindow->isKeyPressed(editorStatus.keyCodes["KEY_D"]))
          this->position += glm::normalize(glm::cross(this->camFront, this->camTop))
                            * cameraSpeed;

        if (appWindow->isKeyPressed(editorStatus.keyCodes["KEY_SPACE"]))
          this->position += this->camTop * cameraSpeed;

        if (appWindow->isKeyPressed(editorStatus.keyCodes["KEY_LEFT_CONTROL"]))
          this->position -= this->camTop * cameraSpeed;
        break;
      }
    }

    // Recompute the view matrix.
    this->view = glm::lookAt(this->position, this->position + this->camFront,
                             this->camTop);

    this->lastMouseX = mousePos.x;
    this->lastMouseY = mousePos.y;
  }

  // Camera zoom function.
  void
  EditorCamera::cameraZoom(glm::vec2 offsets)
  {
    float cameraSpeed = 0.02 * (offsets.y) * this->scalarSpeed;

    this->position += this->camFront * cameraSpeed;

    this->view = glm::lookAt(this->position, this->position + this->camFront,
                             this->camTop);
  }

  // The event handling function.
  void
  EditorCamera::onEvent(Event &event)
  {
    switch (event.getType())
    {
      case EventType::MouseScrolledEvent:
        this->onMouseScroll(*(static_cast<MouseScrolledEvent*>(&event)));
        break;
      case EventType::WindowResizeEvent:
        this->onWindowResize(*(static_cast<WindowResizeEvent*>(&event)));
        break;
      case EventType::KeyPressedEvent:
        this->onKeyPress(*(static_cast<KeyPressedEvent*>(&event)));
        break;
      case EventType::KeyReleasedEvent:
          this->onKeyRelease(*(static_cast<KeyReleasedEvent*>(&event)));
          break;
      case EventType::KeyHeldEvent:
          this->onKeyHeld(*(static_cast<KeyHeldEvent*>(&event)));
          break;
      default:
        break;
    }
  }

  void
  EditorCamera::onMouseScroll(MouseScrolledEvent &mouseEvent)
  {
    // Fetch the application window for input polling.
    Shared<Window>appWindow = Application::getInstance()->getWindow();

    glm::vec2 offsets = mouseEvent.getOffset();

    switch (this->currentType)
    {
      case EditorCameraType::Stationary:
        if (offsets.y != 0.0 && appWindow->isKeyPressed(editorStatus.keyCodes["KEY_LEFT_ALT"]))
          this->cameraZoom(offsets);
        break;
      default:
        break;
    }
  }

  void
  EditorCamera::onWindowResize(WindowResizeEvent &windowEvent)
  {

  }

  void
  EditorCamera::onKeyPress(KeyPressedEvent &keyEvent)
  {
    // Fetch the application window for input polling.
    Shared<Window> appWindow = Application::getInstance()->getWindow();

    int keyCode = keyEvent.getKeyCode();

    if (keyCode == editorStatus.keyCodes["KEY_P"] && appWindow->isKeyPressed(editorStatus.keyCodes["KEY_LEFT_ALT"]))
      this->swap();

    if (keyCode == editorStatus.keyCodes["KEY_LEFT_SHIFT"] && this->currentType == EditorCameraType::Free)
        this->scalarSpeed *= 2.0;
  }

  void
  EditorCamera::onKeyRelease(KeyReleasedEvent& keyEvent)
  {
      // Fetch the applocation window for input polling.
      Shared<Window> appWindow = Application::getInstance()->getWindow();

      int keyCode = keyEvent.getKeyCode();

      if (keyCode == editorStatus.keyCodes["KEY_LEFT_SHIFT"] && this->currentType == EditorCameraType::Free)
          this->scalarSpeed /= 2.0;
  }

  void
  EditorCamera::onKeyHeld(KeyHeldEvent& keyEvent)
  {
      // Fetch the applocation window for input polling.
      Shared<Window> appWindow = Application::getInstance()->getWindow();

      int keyCode = keyEvent.getKeyCode();

      //none
  }

  // Swap the camera types.
  void
  EditorCamera::swap()
  {
    Logger* logs = Logger::getInstance();
    Shared<Window> appWindow = Application::getInstance()->getWindow();

    if (this->currentType == EditorCameraType::Stationary)
    {
      this->currentType = EditorCameraType::Free;
      appWindow->setCursorCapture(true);
      logs->logMessage(LogMessage("Swapped camera to free-form.", true, false));
    }
    else
    {
      this->currentType = EditorCameraType::Stationary;
      appWindow->setCursorCapture(false);
      logs->logMessage(LogMessage("Swapped camera to stationary.", true, false));
    }

    glm::vec2 cursorPos = appWindow->getCursorPos();
    this->lastMouseX = cursorPos.x;
    this->lastMouseY = cursorPos.y;
  }

  // Update the projection matrix.
  void
  EditorCamera::updateProj(float fov, float aspect, float near, float far)
  {
    this->proj = glm::perspective(glm::radians(fov), aspect, near, far);

    this->horFOV = fov;
    this->near = near;
    this->far = far;
    this->aspect = aspect;
  }

  // Fetch the view/projection matrix of the camera.
  glm::mat4&
  EditorCamera::getViewMatrix()
  {
    return this->view;
  }

  glm::mat4&
  EditorCamera::getProjMatrix()
  {
    return this->proj;
  }

  // Fetch the camera position (for shading).
  glm::vec3
  EditorCamera::getCamPos()
  {
    return this->position;
  }

  // Fetch the camera front vector (for shading).
  glm::vec3
  EditorCamera::getCamFront()
  {
    return this->camFront;
  }
  void EditorCamera::saveSettings()
  {
      editorStatus.camera.position = position;
      editorStatus.camera.front = camFront;
      editorStatus.camera.fov = horFOV;
      editorStatus.camera.near = near;
      editorStatus.camera.far = far;
      editorStatus.camera.speed = scalarSpeed;
      editorStatus.camera.sens = sensitivity;
  }
}
