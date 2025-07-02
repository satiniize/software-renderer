#ifndef SPRITE_H
#define SPRITE_H

#include "bitmap.h"
#include "transform.h"
#include "vec2.h"

class Sprite {
public:
  Bitmap bitmap;
  vec2 position;
  Transform transform;
  vec2 size;
  vec2 aabb_top_left;
  vec2 aabb_bottom_right;

  // Update the transform and recalculate corners
  void set_transform(const Transform &t);

  // Update the axis-aligned bounding box based on current transform and size
  void update_aabb();

  // Draw the sprite to a framebuffer (RGBA8888), given framebuffer dimensions
  void draw(uint32_t *framebuffer, int framebuffer_width,
            int framebuffer_height) const;
};

#endif // SPRITE_H
