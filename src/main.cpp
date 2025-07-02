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

  vec2 sprite_position = vec2(0.0f, 0.0f);
  transform sprite_transform;

  float sprite_x_extents = bitmap_read_width / 2.0f;
  float sprite_y_extents = bitmap_read_height / 2.0f;

  vec2 sprite_vertex_top_left =
      sprite_transform * vec2(-sprite_x_extents, -sprite_y_extents);
  vec2 sprite_vertex_top_right =
      sprite_transform * vec2(sprite_x_extents, -sprite_y_extents);
  vec2 sprite_vertex_bottom_left =
      sprite_transform * vec2(-sprite_x_extents, sprite_y_extents);
  vec2 sprite_vertex_bottom_right =
      sprite_transform * vec2(sprite_x_extents, sprite_y_extents);

  vec2 rendering_aabb_top_left = calculate_rendering_aabb_top_left(
      sprite_vertex_top_left, sprite_vertex_top_right,
      sprite_vertex_bottom_left, sprite_vertex_bottom_right);

  vec2 rendering_aabb_bottom_right = calculate_rendering_aabb_bottom_right(
      sprite_vertex_top_left, sprite_vertex_top_right,
      sprite_vertex_bottom_left, sprite_vertex_bottom_right);

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
      sprite_position = sprite_position + vec2(0.0f, -speed);
    }
    if (keystate[SDL_SCANCODE_A]) {
      sprite_position = sprite_position + vec2(-speed, 0.0f);
    }
    if (keystate[SDL_SCANCODE_S]) {
      sprite_position = sprite_position + vec2(0.0f, speed);
    }
    if (keystate[SDL_SCANCODE_D]) {
      sprite_position = sprite_position + vec2(speed, 0.0f);
    }

    // DVD logo integration
    sprite_position = sprite_position + bounce_direction * 0.05f;

    int ticks = SDL_GetTicks();
    float sine = std::sin(float(ticks) / 256.0f);
    float cosine = std::cos(float(ticks) / 256.0f);

    // Set up the transform object
    sprite_transform.set_x_basis(vec2(cosine, -sine)); // x_axis
    sprite_transform.set_y_basis(vec2(sine, cosine));  // y_axis
    sprite_transform.set_origin(sprite_position);

    // Transformed sprite corners using the transform object
    sprite_vertex_top_left =
        sprite_transform * vec2(-sprite_x_extents, -sprite_y_extents);
    sprite_vertex_top_right =
        sprite_transform * vec2(sprite_x_extents, -sprite_y_extents);
    sprite_vertex_bottom_left =
        sprite_transform * vec2(-sprite_x_extents, sprite_y_extents);
    sprite_vertex_bottom_right =
        sprite_transform * vec2(sprite_x_extents, sprite_y_extents);

    rendering_aabb_top_left = calculate_rendering_aabb_top_left(
        sprite_vertex_top_left, sprite_vertex_top_right,
        sprite_vertex_bottom_left, sprite_vertex_bottom_right);

    rendering_aabb_bottom_right = calculate_rendering_aabb_bottom_right(
        sprite_vertex_top_left, sprite_vertex_top_right,
        sprite_vertex_bottom_left, sprite_vertex_bottom_right);

    // --- AABB collision with screen edges (bounce) ---
    // If collision, adjust sprite_position and reverse direction, then
    // recalculate transform and AABB
    vec2 transformed_physics_aabb_top_left =
        physics_aabb_top_left + sprite_position;

    vec2 transformed_physics_aabb_bottom_right =
        physics_aabb_bottom_right + sprite_position;

    bool collided = false;
    if (transformed_physics_aabb_top_left.x < 0) {
      sprite_position.x += -transformed_physics_aabb_top_left.x;
      bounce_direction.x *= -1.0f;
      collided = true;
    }
    if (transformed_physics_aabb_bottom_right.x > WIDTH) {
      sprite_position.x -= (transformed_physics_aabb_bottom_right.x - WIDTH);
      bounce_direction.x *= -1.0f;
      collided = true;
    }
    if (transformed_physics_aabb_top_left.y < 0) {
      sprite_position.y += -transformed_physics_aabb_top_left.y;
      bounce_direction.y *= -1.0f;
      collided = true;
    }
    if (transformed_physics_aabb_bottom_right.y > HEIGHT) {
      sprite_position.y -= (transformed_physics_aabb_bottom_right.y - HEIGHT);
      bounce_direction.y *= -1.0f;
      collided = true;
    }
    if (collided) {
      // Recalculate transform and AABB after collision adjustment
      sprite_transform.set_origin(sprite_position);

      sprite_vertex_top_left =
          sprite_transform * vec2(-sprite_x_extents, -sprite_y_extents);
      sprite_vertex_top_right =
          sprite_transform * vec2(sprite_x_extents, -sprite_y_extents);
      sprite_vertex_bottom_left =
          sprite_transform * vec2(-sprite_x_extents, sprite_y_extents);
      sprite_vertex_bottom_right =
          sprite_transform * vec2(sprite_x_extents, sprite_y_extents);

      rendering_aabb_top_left = calculate_rendering_aabb_top_left(
          sprite_vertex_top_left, sprite_vertex_top_right,
          sprite_vertex_bottom_left, sprite_vertex_bottom_right);

      rendering_aabb_bottom_right = calculate_rendering_aabb_bottom_right(
          sprite_vertex_top_left, sprite_vertex_top_right,
          sprite_vertex_bottom_left, sprite_vertex_bottom_right);
    }

    // --- Software rendering: fill the pixel buffer (checkerboard) ---
    int checker_size = 1;
    for (int y = 0; y < HEIGHT; ++y) {
      for (int x = 0; x < WIDTH; ++x) {
        int checker = ((x / checker_size) + (y / checker_size)) % 2;
        uint8_t v = checker ? 192 : 128;
        staging_buffer[y * WIDTH + x] = (v << 24) | (v << 16) | (v << 8) | 0xFF;
      }
    }

    // Draw sprite within sprite calculated AABB
    for (int y = static_cast<int>(std::floor(rendering_aabb_top_left.y));
         y < static_cast<int>(std::ceil(rendering_aabb_bottom_right.y)); ++y) {
      for (int x = static_cast<int>(std::floor(rendering_aabb_top_left.x));
           x < static_cast<int>(std::ceil(rendering_aabb_bottom_right.x));
           ++x) {
        vec2 tex_coords = sprite_transform.inverse_transform(vec2(x, y));

        // Plus half sprite size to move origin from center to top left
        int u = static_cast<int>(
            std::round(tex_coords.x + bitmap_read_width / 2.0f));
        int v = static_cast<int>(
            std::round(tex_coords.y + bitmap_read_height / 2.0f));

        bool within_bounds = (u >= 0) && (u < bitmap_read_width) && (v >= 0) &&
                             (v < bitmap_read_height);
        bool within_screen =
            (x >= 0) && (x < WIDTH) && (y >= 0) && (y < HEIGHT);
        if (within_bounds && within_screen) {
          uint32_t src = bitmap_read_pixels[v * bitmap_read_width + u];
          uint32_t dst = staging_buffer[y * WIDTH + x];

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

          staging_buffer[y * WIDTH + x] =
              (out_r << 24) | (out_g << 16) | (out_b << 8) | out_a;
        }
      }
    }

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
