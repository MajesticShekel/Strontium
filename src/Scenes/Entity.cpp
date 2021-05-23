#include "Scenes/Entity.h"

namespace SciRenderer
{
  Entity::Entity()
    : entityID(entt::null)
    , parentScene(nullptr)
  { }

  Entity::Entity(entt::entity entHandle, Scene* scene)
    : entityID(entHandle)
    , parentScene(scene)
  { }
}
