#pragma once

#include "vec2.h"

class AABB {
public:
  AABB() = default;
  AABB(const vec2 &top_left, const vec2 &bottom_right)
      : top_left(top_left), bottom_right(bottom_right) {}

  vec2 top_left;
  vec2 bottom_right;
};
