#pragma once

#include "vec2.h"

struct RigidbodyComponent {
  vec2 aabb_top_left = vec2(-4.0f, -4.0f);
  vec2 aabb_bottom_right = vec2(4.0f, 4.0f);
  vec2 velocity = vec2(0.0f, 0.0f);
  vec2 gravity = vec2(0.0f, 256.0f);
};
