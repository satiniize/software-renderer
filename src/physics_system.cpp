#include "physics_system.h"
#include "component_storage.h"
#include "rigidbody_component.h"
#include "transform_component.h"

#include <algorithm>

namespace PhysicsSystem {

void Update(float delta_time) {
  // Physics code will go here

  float WIDTH = 320.0f;
  float HEIGHT = 180.0f;
  float coefficient_of_restitution = 0.6f;

  // Collide with edge
  for (auto &[entity, rigidbody] : rigidbody_components) {
    auto transform_it = transform_components.find(entity);
    if (transform_it != transform_components.end()) {
      TransformComponent &transform = transform_it->second;

      // Modify velocities
      rigidbody.velocity = rigidbody.velocity + rigidbody.gravity * delta_time;

      // Physics integration
      transform.position = transform.position + rigidbody.velocity * delta_time;

      // Collision detection
      vec2 transformed_physics_aabb_top_left =
          rigidbody.aabb_top_left + transform.position;
      vec2 transformed_physics_aabb_bottom_right =
          rigidbody.aabb_bottom_right + transform.position;

      vec2 collision_normal = vec2(0.0f, 0.0f);
      if (transformed_physics_aabb_top_left.x < 0) {
        transform.position.x += -transformed_physics_aabb_top_left.x;
        collision_normal += vec2(1.0f, 0.0f);
      }
      if (transformed_physics_aabb_bottom_right.x > WIDTH) {
        transform.position.x -=
            (transformed_physics_aabb_bottom_right.x - WIDTH);
        collision_normal += vec2(-1.0f, 0.0f);
      }
      if (transformed_physics_aabb_top_left.y < 0) {
        transform.position.y += -transformed_physics_aabb_top_left.y;
        collision_normal += vec2(0.0f, 1.0f);
      }
      if (transformed_physics_aabb_bottom_right.y > HEIGHT) {
        transform.position.y -=
            (transformed_physics_aabb_bottom_right.y - HEIGHT);
        collision_normal += vec2(0.0f, -1.0f);
      }
      rigidbody.velocity +=
          collision_normal *
          rigidbody.velocity.dot(collision_normal.normalized()) *
          -(1.0f + coefficient_of_restitution);
    }
  }

  // Collide with rigid bodies and static bodies
  // Iterate over every rigid body
  for (auto it1 = rigidbody_components.begin();
       it1 != rigidbody_components.end(); ++it1) {
    auto &entity1 = it1->first;
    auto &rigidbody1 = it1->second;

    auto transform_it1 = transform_components.find(entity1);
    if (transform_it1 == transform_components.end()) {
      continue;
    }
    TransformComponent &transform1 = transform_it1->second;

    vec2 transformed_physics_aabb_top_left1 =
        rigidbody1.aabb_top_left + transform1.position;
    vec2 transformed_physics_aabb_bottom_right1 =
        rigidbody1.aabb_bottom_right + transform1.position;

    // process collisions with other rigid bodies
    for (auto it2 = std::next(it1); it2 != rigidbody_components.end(); ++it2) {
      auto &entity2 = it2->first;
      auto &rigidbody2 = it2->second;

      auto transform_it2 = transform_components.find(entity2);
      if (transform_it2 == transform_components.end()) {
        continue;
      }
      TransformComponent &transform2 = transform_it2->second;

      vec2 transformed_physics_aabb_top_left2 =
          rigidbody2.aabb_top_left + transform2.position;
      vec2 transformed_physics_aabb_bottom_right2 =
          rigidbody2.aabb_bottom_right + transform2.position;

      // Calculate overlap on each axis
      float x_overlap =
          std::max(0.0f, std::min(transformed_physics_aabb_bottom_right1.x,
                                  transformed_physics_aabb_bottom_right2.x) -
                             std::max(transformed_physics_aabb_top_left1.x,
                                      transformed_physics_aabb_top_left2.x));
      float y_overlap =
          std::max(0.0f, std::min(transformed_physics_aabb_bottom_right1.y,
                                  transformed_physics_aabb_bottom_right2.y) -
                             std::max(transformed_physics_aabb_top_left1.y,
                                      transformed_physics_aabb_top_left2.y));

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
        float j = -(1.0f + e) * velocity_along_normal;
        j /= (1.0f / rigidbody1.mass) + (1.0f / rigidbody2.mass);

        // 4. Apply impulse
        vec2 impulse = collision_normal * j;
        rigidbody1.velocity += impulse / rigidbody1.mass;
        rigidbody2.velocity -= impulse / rigidbody2.mass;
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
  }
}

} // namespace PhysicsSystem
