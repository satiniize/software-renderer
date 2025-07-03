#pragma once

#include "vec2.h"

struct StaticBodyComponent {
public:
  vec2 aabb_top_left = vec2(-4.0f, -4.0f);
  vec2 aabb_bottom_right = vec2(4.0f, 4.0f);
  float coefficient_of_restitution = 0.5f;
  float coefficient_of_friction = 0.5f;

private:
};
