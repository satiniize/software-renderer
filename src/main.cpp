#include <SDL3/SDL.h>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#define WIDTH 320
#define HEIGHT 180

#include "bitmap.h"
#include "component_storage.h"
#include "entity_manager.h"
#include "sprite_component.h"
#include "sprite_system.h"
#include "transform_component.h"
#include "vec2.h"

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

  EntityManager entity_manager;
  EntityID amogus = entity_manager.create();

  // Add SpriteComponent to amogus
  SpriteComponent sprite_component;
  sprite_component.bitmap = bmp_read;
  sprite_component.size = vec2(static_cast<float>(bitmap_read_width),
                               static_cast<float>(bitmap_read_height));
  // aabb fields will be updated by system later
  sprite_components[amogus] = sprite_component;

  // Add TransformComponent to amogus
  TransformComponent transform_component;
  transform_component.position = vec2(0.0f, 0.0f);
  transform_component.rotation = 0.0f;
  transform_component.scale = vec2(1.0f, 1.0f);
  transform_components[amogus] = transform_component;

  vec2 physics_aabb_top_left = vec2(-4.0f, -4.0f);
  vec2 physics_aabb_bottom_right = vec2(4.0f, 4.0f);
  vec2 velocity = vec2(16.0f, 16.0f);
  vec2 gravity = vec2(0.0f, 256.0f);

  // Software buffer for screen pixels
  uint32_t framebuffer[WIDTH * HEIGHT];

  uint32_t prev_frame_tick = SDL_GetTicks();
  float accumulator = 0.0;
  float physics_frame_rate = 60.0f;
  float physics_delta_time = 1.0f / physics_frame_rate;
  float process_delta_time = 0.0f;
  int physics_frame_count = 0;
  int process_frame_count = 0;

  bool running = true;

  while (running) {
    uint32_t frame_tick = SDL_GetTicks();
    process_delta_time =
        static_cast<float>(frame_tick - prev_frame_tick) / 1000.0f;
    accumulator += process_delta_time;
    prev_frame_tick = frame_tick;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
    }

    const bool *keystate = SDL_GetKeyboardState(NULL);

    process_frame_count++;
    if (accumulator >= physics_delta_time) {
      accumulator -= physics_delta_time;

      physics_frame_count++;
      if (physics_frame_count >= static_cast<int>(physics_frame_rate)) {
        SDL_Log("FPS: %d", process_frame_count);
        process_frame_count = 0;
        physics_frame_count = 0;
      }

      // Get entity transform
      TransformComponent &amogus_transform = transform_components[amogus];

      // Apply gravity
      velocity = velocity + gravity * physics_delta_time;

      // WASD Movement
      float accel = 512.0f;

      if (keystate[SDL_SCANCODE_W]) {
        velocity = velocity + vec2(0.0f, -1.0f) * accel * physics_delta_time;
      }
      if (keystate[SDL_SCANCODE_A]) {
        velocity = velocity + vec2(-1.0f, 0.0f) * accel * physics_delta_time;
      }
      if (keystate[SDL_SCANCODE_S]) {
        velocity = velocity + vec2(0.0f, 1.0f) * accel * physics_delta_time;
      }
      if (keystate[SDL_SCANCODE_D]) {
        velocity = velocity + vec2(1.0f, 0.0f) * accel * physics_delta_time;
      }

      // DVD logo integration
      amogus_transform.position =
          amogus_transform.position + velocity * physics_delta_time;
      amogus_transform.rotation += 2.0f * physics_delta_time;

      // Recalculate transform and AABB
      vec2 transformed_physics_aabb_top_left =
          physics_aabb_top_left + amogus_transform.position;
      vec2 transformed_physics_aabb_bottom_right =
          physics_aabb_bottom_right + amogus_transform.position;

      // Physics
      float coefficient_of_restitution = 0.75f;
      bool collided = false;
      if (transformed_physics_aabb_top_left.x < 0) {
        amogus_transform.position.x += -transformed_physics_aabb_top_left.x;
        velocity.x *= -coefficient_of_restitution;
        collided = true;
      }
      if (transformed_physics_aabb_bottom_right.x > WIDTH) {
        amogus_transform.position.x -=
            (transformed_physics_aabb_bottom_right.x - WIDTH);
        velocity.x *= -coefficient_of_restitution;
        collided = true;
      }
      if (transformed_physics_aabb_top_left.y < 0) {
        amogus_transform.position.y += -transformed_physics_aabb_top_left.y;
        velocity.y *= -coefficient_of_restitution;
        collided = true;
      }
      if (transformed_physics_aabb_bottom_right.y > HEIGHT) {
        amogus_transform.position.y -=
            (transformed_physics_aabb_bottom_right.y - HEIGHT);
        velocity.y *= -coefficient_of_restitution;
        collided = true;
      }
    }

    // --- Software rendering: fill the pixel buffer (checkerboard) ---
    int checker_size = 1;
    for (int y = 0; y < HEIGHT; ++y) {
      for (int x = 0; x < WIDTH; ++x) {
        int checker = ((x / checker_size) + (y / checker_size)) % 2;
        uint8_t v = checker ? 192 : 128;
        framebuffer[y * WIDTH + x] = (v << 24) | (v << 16) | (v << 8) | 0xFF;
      }
    }

    // --- ECS SYSTEMS: Update AABB and draw all sprites ---
    SpriteSystem::update_aabbs();
    SpriteSystem::draw_all(framebuffer, WIDTH, HEIGHT);

    // --- Upload pixel buffer to texture ---
    void *tex_pixels;
    int pitch;
    SDL_LockTexture(texture, NULL, &tex_pixels, &pitch);
    memcpy(tex_pixels, framebuffer, WIDTH * HEIGHT * sizeof(uint32_t));
    SDL_UnlockTexture(texture);

    // --- Render the texture to the window ---
    SDL_RenderClear(renderer);
    SDL_FRect dst = {0, 0, WIDTH * scale, HEIGHT * scale};
    SDL_RenderTexture(renderer, texture, NULL, &dst);
    SDL_RenderPresent(renderer);
  }
  SDL_DestroyGPUDevice(device);
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
