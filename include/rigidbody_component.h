#pragma once

#include "vec2.h"

struct RigidbodyComponent {
public:
  vec2 aabb_top_left = vec2(-4.0f, -4.0f);
  vec2 aabb_bottom_right = vec2(4.0f, 4.0f);
  vec2 velocity = vec2(0.0f, 0.0f);
  vec2 gravity = vec2(0.0f, 256.0f);
  float mass = 1.0f;
  float coefficient_of_restitution = 0.5f;
  float coefficient_of_friction = 0.5f;

  vec2 forces = vec2(0.0f, 0.0f);

private:
};
