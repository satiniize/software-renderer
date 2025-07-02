#pragma once

#include "component_storage.h"
#include <cstdint>

// SpriteSystem: ECS system for sprite-related logic (AABB update, drawing)
namespace SpriteSystem {

// Update the axis-aligned bounding box for all entities with SpriteComponent
// and TransformComponent
void update_aabbs();

// Draw all sprites with SpriteComponent and TransformComponent to the
// framebuffer
void draw_all(uint32_t *framebuffer, int framebuffer_width,
              int framebuffer_height);

} // namespace SpriteSystem
