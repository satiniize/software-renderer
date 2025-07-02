#pragma once

#include "entity_manager.h"
#include "sprite_component.h"
#include "transform_component.h"
#include <unordered_map>

// ECS component storage for each component type.
// Each map stores components by EntityID.

inline std::unordered_map<EntityID, SpriteComponent> spriteComponents;
inline std::unordered_map<EntityID, TransformComponent> transformComponents;
