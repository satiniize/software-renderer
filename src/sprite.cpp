#include "sprite.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring> // for memcpy
#include <vector>

// Update the transform and recalculate corners
void Sprite::set_transform(const Transform &t) { transform = t; }

// Update the axis-aligned bounding box based on current transform and size
void Sprite::update_aabb() {
  float x_ext = size.x / 2.0f;
  float y_ext = size.y / 2.0f;

  vec2 top_left = transform * vec2(-x_ext, -y_ext);
  vec2 top_right = transform * vec2(x_ext, -y_ext);
  vec2 bottom_left = transform * vec2(-x_ext, y_ext);
  vec2 bottom_right = transform * vec2(x_ext, y_ext);

  aabb_top_left =
      vec2(std::min({top_left.x, top_right.x, bottom_left.x, bottom_right.x}),
           std::min({top_left.y, top_right.y, bottom_left.y, bottom_right.y}));
  aabb_bottom_right =
      vec2(std::max({top_left.x, top_right.x, bottom_left.x, bottom_right.x}),
           std::max({top_left.y, top_right.y, bottom_left.y, bottom_right.y}));
}

// Draw the sprite to a framebuffer (RGBA8888), given framebuffer dimensions
void Sprite::draw(uint32_t *framebuffer, int framebuffer_width,
                  int framebuffer_height) const {
  int bmp_width = bitmap.get_width();
  int bmp_height = bitmap.get_height();
  const std::vector<uint32_t> &pixels = bitmap.get_pixels();

  // Draw sprite within calculated AABB
  for (int y = static_cast<int>(std::floor(aabb_top_left.y));
       y < static_cast<int>(std::ceil(aabb_bottom_right.y)); ++y) {
    for (int x = static_cast<int>(std::floor(aabb_top_left.x));
         x < static_cast<int>(std::ceil(aabb_bottom_right.x)); ++x) {
      vec2 tex_coords = transform.inverse_transform(vec2(x, y));
      int u = static_cast<int>(std::round(tex_coords.x + bmp_width / 2.0f));
      int v = static_cast<int>(std::round(tex_coords.y + bmp_height / 2.0f));

      bool within_bounds =
          (u >= 0) && (u < bmp_width) && (v >= 0) && (v < bmp_height);
      bool within_screen = (x >= 0) && (x < framebuffer_width) && (y >= 0) &&
                           (y < framebuffer_height);
      if (within_bounds && within_screen) {
        uint32_t src = pixels[v * bmp_width + u];
        uint32_t &dst = framebuffer[y * framebuffer_width + x];

        uint8_t src_r = (src >> 24) & 0xFF;
        uint8_t src_g = (src >> 16) & 0xFF;
        uint8_t src_b = (src >> 8) & 0xFF;
        uint8_t src_a = src & 0xFF;

        uint8_t dst_r = (dst >> 24) & 0xFF;
        uint8_t dst_g = (dst >> 16) & 0xFF;
        uint8_t dst_b = (dst >> 8) & 0xFF;
        uint8_t dst_a = dst & 0xFF;

        float alpha = src_a / 255.0f;

        uint8_t out_r =
            static_cast<uint8_t>(src_r * alpha + dst_r * (1.0f - alpha));
        uint8_t out_g =
            static_cast<uint8_t>(src_g * alpha + dst_g * (1.0f - alpha));
        uint8_t out_b =
            static_cast<uint8_t>(src_b * alpha + dst_b * (1.0f - alpha));
        uint8_t out_a = 0xFF;

        dst = (out_r << 24) | (out_g << 16) | (out_b << 8) | out_a;
      }
    }
  }
}
