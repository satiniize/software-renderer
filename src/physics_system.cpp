#include "physics_system.h"
#include "component_storage.h"
#include "rigidbody_component.h"
#include "transform_component.h"

#include <algorithm>

namespace PhysicsSystem {

// TODO: Do coefficient of restitution center of mass calculation
void Update(float delta_time) {
  // Physics code will go here

  float WIDTH = 320.0f;
  float HEIGHT = 180.0f;
  float coefficient_of_restitution = 0.75f;

  for (auto &[entity, rigidbody] : rigidbody_components) {
    auto transform_it = transform_components.find(entity);
    if (transform_it != transform_components.end()) {
      TransformComponent &transform = transform_it->second;

      // Physics system
      // Modify velocities
      rigidbody.velocity = rigidbody.velocity + rigidbody.gravity * delta_time;

      // Physics integration
      transform.position = transform.position + rigidbody.velocity * delta_time;

      // Collision detection
      vec2 transformed_physics_aabb_top_left =
          rigidbody.aabb_top_left + transform.position;
      vec2 transformed_physics_aabb_bottom_right =
          rigidbody.aabb_bottom_right + transform.position;

      if (transformed_physics_aabb_top_left.x < 0) {
        transform.position.x += -transformed_physics_aabb_top_left.x;
        rigidbody.velocity.x *= -coefficient_of_restitution;
      }
      if (transformed_physics_aabb_bottom_right.x > WIDTH) {
        transform.position.x -=
            (transformed_physics_aabb_bottom_right.x - WIDTH);
        rigidbody.velocity.x *= -coefficient_of_restitution;
      }
      if (transformed_physics_aabb_top_left.y < 0) {
        transform.position.y += -transformed_physics_aabb_top_left.y;
        rigidbody.velocity.y *= -coefficient_of_restitution;
      }
      if (transformed_physics_aabb_bottom_right.y > HEIGHT) {
        transform.position.y -=
            (transformed_physics_aabb_bottom_right.y - HEIGHT);
        rigidbody.velocity.y *= -coefficient_of_restitution;
      }
    }
  }

  for (auto it1 = rigidbody_components.begin();
       it1 != rigidbody_components.end(); ++it1) {
    auto &entity1 = it1->first;
    auto &rigidbody1 = it1->second;

    auto transform_it1 = transform_components.find(entity1);
    if (transform_it1 != transform_components.end()) {
      TransformComponent &transform1 = transform_it1->second;

      vec2 transformed_physics_aabb_top_left1 =
          rigidbody1.aabb_top_left + transform1.position;
      vec2 transformed_physics_aabb_bottom_right1 =
          rigidbody1.aabb_bottom_right + transform1.position;

      for (auto it2 = std::next(it1); it2 != rigidbody_components.end();
           ++it2) {
        auto &entity2 = it2->first;
        auto &rigidbody2 = it2->second;

        auto transform_it2 = transform_components.find(entity2);
        if (transform_it2 != transform_components.end()) {
          TransformComponent &transform2 = transform_it2->second;

          vec2 transformed_physics_aabb_top_left2 =
              rigidbody2.aabb_top_left + transform2.position;
          vec2 transformed_physics_aabb_bottom_right2 =
              rigidbody2.aabb_bottom_right + transform2.position;

          // Calculate overlap on each axis
          float x_overlap = std::max(
              0.0f, std::min(transformed_physics_aabb_bottom_right1.x,
                             transformed_physics_aabb_bottom_right2.x) -
                        std::max(transformed_physics_aabb_top_left1.x,
                                 transformed_physics_aabb_top_left2.x));
          float y_overlap = std::max(
              0.0f, std::min(transformed_physics_aabb_bottom_right1.y,
                             transformed_physics_aabb_bottom_right2.y) -
                        std::max(transformed_physics_aabb_top_left1.y,
                                 transformed_physics_aabb_top_left2.y));

          bool is_colliding = x_overlap > 0.0f && y_overlap > 0.0f;

          // Collision check
          if (is_colliding) {
            // Determine the axis of longest overlap
            if (x_overlap < y_overlap) {
              // Apply impulse along the x-axis
              float push_back = x_overlap * 0.5f;
              if (transform1.position.x < transform2.position.x) {
                transform1.position.x -= push_back;
                transform2.position.x += push_back;

                rigidbody1.velocity.x *= -coefficient_of_restitution;
                rigidbody2.velocity.x *= -coefficient_of_restitution;
              } else {
                transform1.position.x += push_back;
                transform2.position.x -= push_back;

                float temp_velocity_x1 = rigidbody1.velocity.x;
                rigidbody1.velocity.x =
                    rigidbody2.velocity.x * coefficient_of_restitution;
                rigidbody2.velocity.x =
                    temp_velocity_x1 * coefficient_of_restitution;
              }
            } else {

              // Apply impulse along the y - axis
              float push_back = y_overlap * 0.5f;
              if (transform1.position.y < transform2.position.y) {
                transform1.position.y -= push_back;
                transform2.position.y += push_back;

                float temp_velocity_y1 = rigidbody1.velocity.y;
                rigidbody1.velocity.y =
                    rigidbody2.velocity.y * coefficient_of_restitution;
                rigidbody2.velocity.y =
                    temp_velocity_y1 * coefficient_of_restitution;
              } else {
                transform1.position.y += push_back;
                transform2.position.y -= push_back;

                float temp_velocity_y1 = rigidbody1.velocity.y;
                rigidbody1.velocity.y =
                    rigidbody2.velocity.y * coefficient_of_restitution;
                rigidbody2.velocity.y =
                    temp_velocity_y1 * coefficient_of_restitution;
              }
            }
          }
        }
      }
    }
  }
}

} // namespace PhysicsSystem
