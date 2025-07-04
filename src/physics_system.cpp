#include "physics_system.h"
#include "component_storage.h"
#include "rigidbody_component.h"
#include "transform_component.h"

#include <algorithm>
#include <iostream>

namespace PhysicsSystem {

void Update(float delta_time) {
  float coefficient_of_restitution = 0.5f;
  float coefficient_of_friction = 0.5f;
  // Collide with rigid bodies and static bodies
  // Iterate over every rigid body
  for (auto it1 = rigidbody_components.begin();
       it1 != rigidbody_components.end(); ++it1) {
    auto &entity1 = it1->first;
    auto &rigidbody1 = it1->second;
    // Check if entity has a transform component
    auto transform_it1 = transform_components.find(entity1);
    if (transform_it1 == transform_components.end()) {
      continue;
    }
    TransformComponent &transform1 = transform_it1->second;
    // Built in velocity modification
    rigidbody1.velocity = rigidbody1.velocity + rigidbody1.gravity * delta_time;
    // Physics integration
    transform1.position =
        transform1.position + rigidbody1.velocity * delta_time;

    AABB world_space_aabb1 =
        rigidbody1.collision_aabb.to_world_space(transform1.position);

    // process collisions with other rigid bodies
    for (auto it2 = std::next(it1); it2 != rigidbody_components.end(); ++it2) {
      auto &entity2 = it2->first;
      auto &rigidbody2 = it2->second;

      auto transform_it2 = transform_components.find(entity2);
      if (transform_it2 == transform_components.end()) {
        continue;
      }
      TransformComponent &transform2 = transform_it2->second;

      AABB world_space_aabb2 =
          rigidbody2.collision_aabb.to_world_space(transform2.position);

      float x_overlap =
          std::max(0.0f, std::min(world_space_aabb1.bottom_right.x,
                                  world_space_aabb2.bottom_right.x) -
                             std::max(world_space_aabb1.top_left.x,
                                      world_space_aabb2.top_left.x));
      float y_overlap =
          std::max(0.0f, std::min(world_space_aabb1.bottom_right.y,
                                  world_space_aabb2.bottom_right.y) -
                             std::max(world_space_aabb1.top_left.y,
                                      world_space_aabb2.top_left.y));

      bool is_colliding = x_overlap > 0.0f && y_overlap > 0.0f;

      // Resolve collision
      if (!is_colliding) {
        continue;
      }

      vec2 collision_normal;

      if (x_overlap < y_overlap) {
        collision_normal = vec2(1.0f, 0.0f);
        if (transform1.position.x > transform2.position.x)
          collision_normal = -collision_normal;
      } else {
        collision_normal = vec2(0.0f, 1.0f);
        if (transform1.position.y > transform2.position.y)
          collision_normal = -collision_normal;
      }

      vec2 relative_velocity = rigidbody1.velocity - rigidbody2.velocity;
      float velocity_along_normal = relative_velocity.dot(collision_normal);

      if (velocity_along_normal > 0.0f) {
        // 3. Calculate impulse scalar
        float e = coefficient_of_restitution;
        float j = (1.0f + e) * velocity_along_normal;
        j /= (1.0f / rigidbody1.mass) + (1.0f / rigidbody2.mass);

        // 4. Apply impulse
        vec2 impulse = collision_normal * -j;
        rigidbody1.velocity += impulse / rigidbody1.mass;
        rigidbody2.velocity -= impulse / rigidbody2.mass;
        vec2 velocity_along_surface1 =
            (rigidbody1.velocity -
             collision_normal * rigidbody1.velocity.dot(collision_normal));
        vec2 velocity_along_surface2 =
            (rigidbody2.velocity -
             collision_normal * rigidbody2.velocity.dot(collision_normal));
        rigidbody1.velocity -=
            velocity_along_surface1.normalized() * j * coefficient_of_friction;
        rigidbody2.velocity -=
            velocity_along_surface2.normalized() * j * coefficient_of_friction;
      }
      // Push back objects so they dont intersect with each other
      if (x_overlap < y_overlap) {
        // Apply impulse along the x-axis
        float push_back = x_overlap * 0.5f;
        // TODO: Apply friction along y axis
        if (transform1.position.x < transform2.position.x) {
          transform1.position.x -= push_back;
          transform2.position.x += push_back;
        } else {
          transform1.position.x += push_back;
          transform2.position.x -= push_back;
        }
      } else {
        // Apply impulse along the y-axis
        float push_back = y_overlap * 0.5f;
        // TODO: Apply friction along x axis
        if (transform1.position.y < transform2.position.y) {
          transform1.position.y -= push_back;
          transform2.position.y += push_back;
        } else {
          transform1.position.y += push_back;
          transform2.position.y -= push_back;
        }
      }
    }
    // Collide with static bodies
    for (auto &[entity2, staticbody2] : staticbody_components) {
      auto transform_it2 = transform_components.find(entity2);
      if (transform_it2 == transform_components.end()) {
        continue;
      }
      TransformComponent &transform2 = transform_it2->second;

      AABB world_space_aabb2 =
          staticbody2.collision_aabb.to_world_space(transform2.position);

      float x_overlap =
          std::max(0.0f, std::min(world_space_aabb1.bottom_right.x,
                                  world_space_aabb2.bottom_right.x) -
                             std::max(world_space_aabb1.top_left.x,
                                      world_space_aabb2.top_left.x));
      float y_overlap =
          std::max(0.0f, std::min(world_space_aabb1.bottom_right.y,
                                  world_space_aabb2.bottom_right.y) -
                             std::max(world_space_aabb1.top_left.y,
                                      world_space_aabb2.top_left.y));

      bool is_colliding = x_overlap > 0.0f && y_overlap > 0.0f;

      if (!is_colliding) {
        continue;
      }

      vec2 collision_normal;

      if (x_overlap < y_overlap) {
        collision_normal = vec2(1.0f, 0.0f);
        if (transform1.position.x > transform2.position.x)
          collision_normal = -collision_normal;
      } else {
        collision_normal = vec2(0.0f, 1.0f);
        if (transform1.position.y > transform2.position.y)
          collision_normal = -collision_normal;
      }

      // Push back objects so they dont intersect with each other
      if (x_overlap < y_overlap) {
        // Apply impulse along the x-axis
        float push_back = x_overlap;
        // TODO: Apply friction along y axis
        if (transform1.position.x < transform2.position.x) {
          transform1.position.x -= push_back;
        } else {
          transform1.position.x += push_back;
        }
      } else {
        // Apply impulse along the y-axis
        float push_back = y_overlap;
        // TODO: Apply friction along x axis
        if (transform1.position.y < transform2.position.y) {
          transform1.position.y -= push_back;
        } else {
          transform1.position.y += push_back;
        }
      }
      float impulse = rigidbody1.velocity.dot(collision_normal.normalized()) *
                      (1.0f + coefficient_of_restitution);

      rigidbody1.velocity -= collision_normal * impulse;
      vec2 velocity_along_surface =
          (rigidbody1.velocity -
           collision_normal * rigidbody1.velocity.dot(collision_normal));
      rigidbody1.velocity -= velocity_along_surface.normalized() *
                             std::min(impulse * coefficient_of_friction,
                                      velocity_along_surface.length());
    }
  }
}

} // namespace PhysicsSystem
