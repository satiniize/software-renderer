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
#include "sprite.h"
#include "transform.h"
#include "vec2.h"
#include "vec2i.h"

vec2 calculate_rendering_aabb_top_left(vec2 top_left, vec2 top_right,
                                       vec2 bottom_left, vec2 bottom_right) {
  return vec2(
      std::min({top_left.x, top_right.x, bottom_left.x, bottom_right.x}),
      std::min({top_left.y, top_right.y, bottom_left.y, bottom_right.y}));
}

vec2 calculate_rendering_aabb_bottom_right(vec2 top_left, vec2 top_right,
                                           vec2 bottom_left,
                                           vec2 bottom_right) {
  return vec2(
      std::max({top_left.x, top_right.x, bottom_left.x, bottom_right.x}),
      std::max({top_left.y, top_right.y, bottom_left.y, bottom_right.y}));
}

int main(int argc, char *argv[]) {
  std::string bitmap_write_name = "test_image_write.bmp";
  std::string bitmap_read_name = "test_image_read.bmp";

  int bitmap_write_width = 16;
  int bitmap_write_height = 16;

  // Create a test image (gradient)
  Bitmap bmp_write(bitmap_write_width, bitmap_write_height);

  for (int y = 0; y < bitmap_write_height; ++y) {
    for (int x = 0; x < bitmap_write_width; ++x) {
      uint8_t r =
          static_cast<int>(static_cast<float>(x) / bitmap_write_width * 255.0f);
      uint8_t g = static_cast<int>(static_cast<float>(y) / bitmap_write_height *
                                   255.0f);
      uint8_t b = 0;
      bmp_write.set_pixel(x, y, (r << 24) | (g << 16) | (b << 8) | 0xFF);
    }
  }

  // Dump BMP file
  if (!bmp_write.dump(bitmap_write_name)) {
    std::cerr << "Failed to write BMP file.\n";
    return 1;
  }

  // Read BMP file using constructor
  Bitmap bmp_read(bitmap_read_name);

  int bitmap_read_width = bmp_read.get_width();
  int bitmap_read_height = bmp_read.get_height();

  if (bitmap_read_width == 0 || bitmap_read_height == 0) {
    std::cerr << "Failed to open image file.\n";
    return 1;
  }

  std::vector<uint32_t> bitmap_read_pixels = bmp_read.get_pixels();

  std::cout << "Width: " << bitmap_read_width << std::endl;
  std::cout << "Height: " << bitmap_read_height << std::endl;
  std::cout << "Pixels: " << bitmap_read_pixels.size() << std::endl;

  // SDL setup

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return 1;
  }
  SDL_Log("SDL video driver: %s", SDL_GetCurrentVideoDriver());

  // Scale up for visibility
  const int scale = 4;
  SDL_Window *window =
      SDL_CreateWindow("Checkerboard", WIDTH * scale, HEIGHT * scale, 0);
  if (!window) {
    SDL_Log("Couldn't create window: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  // SDL_GPU
  SDL_GPUDevice *device =
      SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
  SDL_ClaimWindowForGPUDevice(device, window);

  SDL_GPUCommandBuffer *commandBuffer = SDL_AcquireGPUCommandBuffer(device);

  SDL_SubmitGPUCommandBuffer(commandBuffer);

  // SDL_GPUTextureCreateInfo *gpu_texture_create_info;

  // SDL_GPUTexture *gpu_texture =
  //     SDL_CreateGPUTexture(device, gpu_texture_create_info);

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

  Sprite amogus;
  amogus.bitmap = bmp_read;
  amogus.size = vec2(static_cast<float>(bitmap_read_width),
                     static_cast<float>(bitmap_read_height));
  amogus.position = vec2(0.0f, 0.0f);
  amogus.transform = Transform();
  vec2 physics_aabb_top_left = vec2(-4.0f, -4.0f);
  vec2 physics_aabb_bottom_right = vec2(4.0f, 4.0f);
  vec2 bounce_direction = vec2(1.0f, 1.0f);

  // Software buffer for screen pixels
  uint32_t staging_buffer[WIDTH * HEIGHT];

  // FPS timer variables
  uint32_t fps_last_time = SDL_GetTicks();
  int fps_frames = 0;

  bool running = true;

  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
    }

    const bool *keystate = SDL_GetKeyboardState(NULL);

    // WASD Movement
    float speed = 0.1f;
    if (keystate[SDL_SCANCODE_W]) {
      amogus.position = amogus.position + vec2(0.0f, -speed);
    }
    if (keystate[SDL_SCANCODE_A]) {
      amogus.position = amogus.position + vec2(-speed, 0.0f);
    }
    if (keystate[SDL_SCANCODE_S]) {
      amogus.position = amogus.position + vec2(0.0f, speed);
    }
    if (keystate[SDL_SCANCODE_D]) {
      amogus.position = amogus.position + vec2(speed, 0.0f);
    }

    // DVD logo integration
    amogus.position = amogus.position + bounce_direction * 0.05f;

    int ticks = SDL_GetTicks();
    float sine = std::sin(float(ticks) / 256.0f);
    float cosine = std::cos(float(ticks) / 256.0f);

    // Set up the transform object for the sprite
    Transform t;
    t.set_x_basis(vec2(cosine, -sine)); // x_axis
    t.set_y_basis(vec2(sine, cosine));  // y_axis
    t.set_origin(amogus.position);
    amogus.set_transform(t);

    // Recalculate transform and AABB
    vec2 transformed_physics_aabb_top_left =
        physics_aabb_top_left + amogus.position;
    vec2 transformed_physics_aabb_bottom_right =
        physics_aabb_bottom_right + amogus.position;

    // Physics
    bool collided = false;
    if (transformed_physics_aabb_top_left.x < 0) {
      amogus.position.x += -transformed_physics_aabb_top_left.x;
      bounce_direction.x *= -1.0f;
      collided = true;
    }
    if (transformed_physics_aabb_bottom_right.x > WIDTH) {
      amogus.position.x -= (transformed_physics_aabb_bottom_right.x - WIDTH);
      bounce_direction.x *= -1.0f;
      collided = true;
    }
    if (transformed_physics_aabb_top_left.y < 0) {
      amogus.position.y += -transformed_physics_aabb_top_left.y;
      bounce_direction.y *= -1.0f;
      collided = true;
    }
    if (transformed_physics_aabb_bottom_right.y > HEIGHT) {
      amogus.position.y -= (transformed_physics_aabb_bottom_right.y - HEIGHT);
      bounce_direction.y *= -1.0f;
      collided = true;
    }

    // Recalculate sprite rendering AABB after collision adjustment
    amogus.update_aabb();

    // --- Software rendering: fill the pixel buffer (checkerboard) ---
    int checker_size = 1;
    for (int y = 0; y < HEIGHT; ++y) {
      for (int x = 0; x < WIDTH; ++x) {
        int checker = ((x / checker_size) + (y / checker_size)) % 2;
        uint8_t v = checker ? 192 : 128;
        staging_buffer[y * WIDTH + x] = (v << 24) | (v << 16) | (v << 8) | 0xFF;
      }
    }

    // Draw sprite using Sprite's draw method
    amogus.draw(staging_buffer, WIDTH, HEIGHT);

    // --- Upload pixel buffer to texture ---
    void *tex_pixels;
    int pitch;
    SDL_LockTexture(texture, NULL, &tex_pixels, &pitch);
    memcpy(tex_pixels, staging_buffer, WIDTH * HEIGHT * sizeof(uint32_t));
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
  SDL_DestroyGPUDevice(device);
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
