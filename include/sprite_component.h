#pragma once

#include "bitmap.h"
#include "vec2.h"

// Pure ECS data-only component for sprite rendering
struct SpriteComponent {
  Bitmap bitmap;
  vec2 size;
  vec2 aabb_top_left;
  vec2 aabb_bottom_right;
};
