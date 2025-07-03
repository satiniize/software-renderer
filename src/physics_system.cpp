#include "physics_system.h"
#include "component_storage.h"
#include "rigidbody_component.h"
#include "transform_component.h"

namespace PhysicsSystem {

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
}

} // namespace PhysicsSystem
