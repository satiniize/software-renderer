#pragma once

#include "aabb.h"
#include "bitmap.h"
#include "vec2.h"

// Pure ECS data-only component for sprite rendering
struct SpriteComponent {
  Bitmap bitmap;
  vec2 size;
  AABB rendering_aabb;
};
