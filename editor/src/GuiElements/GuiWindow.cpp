#include "GuiElements/GuiWindow.h"

namespace Strontium
{
  GuiWindow::GuiWindow(EditorLayer* parentLayer, bool isOpen)
    : parentLayer(parentLayer)
    , isOpen(isOpen)
  { 
      name = "GuiWindow";
  }

  GuiWindow::~GuiWindow()
  { }

  void
  GuiWindow::onImGuiRender(bool &isOpen, Shared<Scene> activeScene)
  { }

  void
  GuiWindow::onUpdate(float dt, Shared<Scene> activeScene)
  { }

  void
  GuiWindow::onEvent(Event &event)
  { }
}
