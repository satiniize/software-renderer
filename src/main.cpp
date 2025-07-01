#include "bitmap.h"
#include <SDL3/SDL.h>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdint.h>
#include <string>
#include <vector>

#define WIDTH 160
#define HEIGHT 90

#include "vec2i.h"

// (BMP read/write helpers moved to bitmap.cpp)

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
      if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_KEY_DOWN) {
        running = false;
      }
    }

    // --- Software rendering: fill the pixel buffer (checkerboard) ---
    int checker_size = 1;
    for (int y = 0; y < HEIGHT; ++y) {
      for (int x = 0; x < WIDTH; ++x) {
        int checker = ((x / checker_size) + (y / checker_size)) % 2;
        uint8_t v = checker ? 255 : 0;
        pixels[y * WIDTH + x] = (v << 24) | (v << 16) | (v << 8) | 0xFF;
      }
    }

    for (int y = 0; y < bmp_height; ++y) {
      for (int x = 0; x < bmp_width; ++x) {
        int ticks = SDL_GetTicks();
        vec2i offset = vec2i(-(bmp_width / 2) + (WIDTH / 2) +
                                 32.0 * std::sin(float(ticks) / 1000.0f),
                             -(bmp_height / 2) + (HEIGHT / 2) +
                                 32.0 * std::cos(float(ticks) / 1000.0f));
        int transformed_x = x + offset.x;
        int transformed_y = y + offset.y;
        bool within_bounds = (transformed_x >= 0) && (transformed_x < WIDTH) &&
                             (transformed_y >= 0) && (transformed_y < HEIGHT);
        if (within_bounds) {
          uint32_t src = texture_pixels[y * bmp_width + x];
          uint32_t dst = pixels[transformed_y * WIDTH + transformed_x];

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

          pixels[transformed_y * WIDTH + transformed_x] =
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
