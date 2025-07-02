#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdint.h>
#include <string>
#include <vector>

#define WIDTH 320
#define HEIGHT 180

#include "bitmap.h"
#include "vec2.h"
#include "vec2i.h"

vec2 transform(vec2 x_axis, vec2 y_axis, vec2 origin, vec2 point) {
  return vec2(x_axis.x * point.x + y_axis.x * point.y + origin.x,
              x_axis.y * point.x + y_axis.y * point.y + origin.y);
}

vec2 inverse_transform(vec2 x_axis, vec2 y_axis, vec2 origin,
                       vec2 transformed_point) {
  // Step 1: Subtract the origin
  float px = transformed_point.x - origin.x;
  float py = transformed_point.y - origin.y;

  // Step 2: Compute the determinant
  float det = x_axis.x * y_axis.y - x_axis.y * y_axis.x;
  if (fabs(det) < 1e-8) {
    // Handle singular matrix (no inverse)
    return vec2(0, 0); // Or handle error as needed
  }
  float inv_det = 1.0f / det;

  // Step 3: Apply the inverse matrix
  float x = (y_axis.y * px - y_axis.x * py) * inv_det;
  float y = (-x_axis.y * px + x_axis.x * py) * inv_det;

  return vec2(x, y);
}

int main(int argc, char *argv[]) {
  std::string image_file_name_write = "test_image_write.bmp";
  std::string image_file_name_read = "test_image_read.bmp";

  int image_width = 16;
  int image_height = 16;

  // Create a test image (gradient)
  Bitmap bmp_write(image_width, image_height);
  for (int y = 0; y < image_height; ++y) {
    for (int x = 0; x < image_width; ++x) {
      uint8_t r =
          static_cast<int>(static_cast<float>(x) / image_width * 255.0f);
      uint8_t g =
          static_cast<int>(static_cast<float>(y) / image_height * 255.0f);
      uint8_t b = 0;
      bmp_write.set_pixel(x, y, (r << 24) | (g << 16) | (b << 8) | 0xFF);
    }
  }

  // Dump BMP file
  if (!bmp_write.dump(image_file_name_write)) {
    std::cerr << "Failed to write BMP file.\n";
    return 1;
  }

  // Read BMP file using constructor
  Bitmap bmp_read(image_file_name_read);
  if (bmp_read.width() == 0 || bmp_read.height() == 0) {
    std::cerr << "Failed to open image file.\n";
    return 1;
  }

  std::cout << "Width: " << bmp_read.width() << std::endl;
  std::cout << "Height: " << bmp_read.height() << std::endl;
  std::cout << "Pixels: " << bmp_read.pixels().size() << std::endl;

  int bmp_width = bmp_read.width();
  int bmp_height = bmp_read.height();
  std::vector<uint32_t> texture_pixels = bmp_read.pixels();

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return 1;
  }
  SDL_Log("SDL video driver: %s", SDL_GetCurrentVideoDriver());

  // FPS timer variables
  uint32_t fps_last_time = SDL_GetTicks();
  int fps_frames = 0;

  // Scale up for visibility
  const int scale = 8;
  SDL_Window *window =
      SDL_CreateWindow("Checkerboard", WIDTH * scale, HEIGHT * scale, 0);
  if (!window) {
    SDL_Log("Couldn't create window: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
  if (!renderer) {
    SDL_Log("Couldn't create renderer: %s", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  SDL_Texture *texture =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                        SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
  SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
  if (!texture) {
    SDL_Log("Couldn't create texture: %s", SDL_GetError());
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  uint32_t pixels[WIDTH * HEIGHT];

  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
    }

    // --- Software rendering: fill the pixel buffer (checkerboard) ---
    int checker_size = 1;
    for (int y = 0; y < HEIGHT; ++y) {
      for (int x = 0; x < WIDTH; ++x) {
        int checker = ((x / checker_size) + (y / checker_size)) % 2;
        uint8_t v = checker ? 192 : 128;
        pixels[y * WIDTH + x] = (v << 24) | (v << 16) | (v << 8) | 0xFF;
      }
    }

    float rotation = 0.0f;

    int ticks = SDL_GetTicks();

    float sine = std::sin(float(ticks) / 256.0f);
    float cosine = std::cos(float(ticks) / 256.0f);

    // No rotation
    // float sine = 0.0f;
    // float cosine = 1.0f;

    // Transform matrix
    vec2 x_axis = vec2(cosine, -sine); // Initially faces right, rotates CCW
    vec2 y_axis = vec2(sine, cosine);  // Initially faces down, rotates CCW
    vec2 origin = vec2((WIDTH / 2), (HEIGHT / 2));

    // Transformed sprite corners
    vec2 top_left = transform(x_axis, y_axis, origin,
                              vec2(-bmp_width / 2.0f, -bmp_height / 2.0f));
    vec2 top_right = transform(x_axis, y_axis, origin,
                               vec2(bmp_width / 2.0f, -bmp_height / 2.0f));
    vec2 bottom_left = transform(x_axis, y_axis, origin,
                                 vec2(-bmp_width / 2.0f, bmp_height / 2.0f));
    vec2 bottom_right = transform(x_axis, y_axis, origin,
                                  vec2(bmp_width / 2.0f, bmp_height / 2.0f));

    vec2 aabb_top_left = vec2(
        std::min({top_left.x, top_right.x, bottom_left.x, bottom_right.x}),
        std::min({top_left.y, top_right.y, bottom_left.y, bottom_right.y}));
    vec2 aabb_bottom_right = vec2(
        std::max({top_left.x, top_right.x, bottom_left.x, bottom_right.x}),
        std::max({top_left.y, top_right.y, bottom_left.y, bottom_right.y}));

    for (int y = static_cast<int>(std::floor(aabb_top_left.y));
         y < static_cast<int>(std::ceil(aabb_bottom_right.y)); ++y) {
      for (int x = static_cast<int>(std::floor(aabb_top_left.x));
           x < static_cast<int>(std::ceil(aabb_bottom_right.x)); ++x) {
        // vec2 point = vec2(x - bmp_width / 2, y - bmp_height / 2);
        vec2 tex_coords = inverse_transform(x_axis, y_axis, origin, vec2(x, y));

        // Plus half sprite size to move origin from center to top left
        int u = static_cast<int>(std::round(tex_coords.x + bmp_width / 2.0f));
        int v = static_cast<int>(std::round(tex_coords.y + bmp_height / 2.0f));

        bool within_bounds =
            (u >= 0) && (u < bmp_width) && (v >= 0) && (v < bmp_height);
        if (within_bounds) {
          uint32_t src = texture_pixels[v * bmp_width + u];
          uint32_t dst = pixels[y * WIDTH + x];

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
          uint8_t out_a = 0xFF; // Output alpha can be set as needed

          pixels[y * WIDTH + x] =
              (out_r << 24) | (out_g << 16) | (out_b << 8) | out_a;
        }
      }
    }

    // --- Upload pixel buffer to texture ---
    void *tex_pixels;
    int pitch;
    SDL_LockTexture(texture, NULL, &tex_pixels, &pitch);
    memcpy(tex_pixels, pixels, WIDTH * HEIGHT * sizeof(uint32_t));
    SDL_UnlockTexture(texture);

    // --- Render the texture to the window ---
    SDL_RenderClear(renderer);
    SDL_FRect dst = {0, 0, WIDTH * scale, HEIGHT * scale};
    SDL_RenderTexture(renderer, texture, NULL, &dst);
    SDL_RenderPresent(renderer);

    // FPS timer logic
    fps_frames++;
    uint32_t now = SDL_GetTicks();
    if (now - fps_last_time >= 1000) {
      SDL_Log("FPS: %d", fps_frames);
      fps_frames = 0;
      fps_last_time = now;
    }
  }

  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
