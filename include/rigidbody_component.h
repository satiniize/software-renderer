#pragma once

#include "aabb.h"
#include "vec2.h"

struct RigidbodyComponent {
public:
  AABB collision_aabb = AABB(vec2(-4.0f, -4.0f), vec2(4.0f, 4.0f));
  vec2 velocity = vec2(0.0f, 0.0f);
  vec2 gravity = vec2(0.0f, 256.0f);
  float mass = 1.0f;
  float coefficient_of_restitution = 0.5f;
  float coefficient_of_friction = 0.5f;

  vec2 forces = vec2(0.0f, 0.0f);

private:
};
