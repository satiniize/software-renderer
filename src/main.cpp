#include <SDL3/SDL.h>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdint.h>
#include <string>
#include <vector>

#define WIDTH 160
#define HEIGHT 90

#include "vec2i.h"

// little endian
void write_u16(std::ofstream &out, uint16_t val) {
  out.put(val & 0xFF);
  out.put((val >> 8) & 0xFF);
}

// little endian
void write_u32(std::ofstream &out, uint32_t val) {
  out.put(val & 0xFF);
  out.put((val >> 8) & 0xFF);
  out.put((val >> 16) & 0xFF);
  out.put((val >> 24) & 0xFF);
}

uint16_t read_u16(std::ifstream &in) {
  uint8_t b0 = in.get();
  uint8_t b1 = in.get();
  return (b1 << 8) | b0;
}

uint32_t read_u32(std::ifstream &in) {
  uint8_t b0 = in.get();
  uint8_t b1 = in.get();
  uint8_t b2 = in.get();
  uint8_t b3 = in.get();
  return (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
}

int main(int argc, char *argv[]) {
  std::string image_file_name_write = "test_image_write.bmp";
  std::string image_file_name_read = "test_image_read.bmp";

  int image_width = 16;
  int image_height = 16;

  int row_size = ((image_width * 3 + 3) / 4) * 4;
  int padding = row_size - image_width * 3;
  int image_size = row_size * image_height;
  int file_size = 14 + 40 + image_size;

  std::ofstream out(image_file_name_write, std::ios::binary);

  // BMP File Header (14 bytes)
  write_u16(out, 0x4D42);    // bfType
  write_u32(out, file_size); // bfSize
  write_u16(out, 0);         // bfReserved1
  write_u16(out, 0);         // bfReserved2
  write_u32(out, 54);        // bfOffBits

  // DIB Header (BITMAPINFOHEADER, 40 bytes)
  write_u32(out, 40);           // biSize
  write_u32(out, image_width);  // biWidth
  write_u32(out, image_height); // biHeight
  write_u16(out, 1);            // biPlanes
  write_u16(out, 24);           // biBitCount
  write_u32(out, 0);            // biCompression
  write_u32(out, image_size);   // biSizeImage
  write_u32(out, 2835);         // biXPelsPerMeter
  write_u32(out, 2835);         // biYPelsPerMeter
  write_u32(out, 0);            // biClrUsed
  write_u32(out, 0);            // biClrImportant

  // For each row (bottom-up for BMP)
  for (int y = 0; y < image_height; ++y) {
    // Write each pixel in the row
    for (int x = 0; x < image_width; ++x) {
      out.put(0); // Blue
      out.put(static_cast<int>(static_cast<float>(y) /
                               static_cast<float>(image_height) *
                               255.0f)); // Green
      out.put(static_cast<int>(static_cast<float>(x) /
                               static_cast<float>(image_width) *
                               255.0f)); // Red
    }
    // Write padding bytes (if any)
    for (int p = 0; p < padding; ++p) {
      out.put(0);
    }
  }

  // Write to file
  out.close();

  // Open written file for reading
  std::ifstream image_texture(image_file_name_read, std::ios::binary);
  if (!image_texture) {
    std::cerr << "Failed to open image file.\n";
    return 1;
  }

  image_texture.seekg(10, std::ios::beg);
  uint32_t pixel_data_offset = read_u32(image_texture);
  image_texture.seekg(18, std::ios::beg);
  uint32_t bmp_width = read_u32(image_texture);
  uint32_t bmp_height = read_u32(image_texture);
  image_texture.seekg(28, std::ios::beg);
  uint32_t bit_depth = read_u16(image_texture);

  std::cout << "Pixel data offset: " << pixel_data_offset << std::endl;
  std::cout << "Width: " << bmp_width << std::endl;
  std::cout << "Height: " << bmp_height << std::endl;
  std::cout << "Bit depth: " << bit_depth << std::endl;

  int bytes_per_pixel = bit_depth / 8; // This assumes 8 8 8 BGR
  std::vector<uint32_t> texture_pixels(bmp_width * bmp_height);

  image_texture.seekg(pixel_data_offset, std::ios::beg);

  for (int y = 0; y < bmp_height; ++y) {
    int bmp_y = bmp_height - 1 - y; // BMP is bottom-up
    for (int x = 0; x < bmp_width; ++x) {
      char pixel_buffer[3];
      image_texture.read(pixel_buffer, bytes_per_pixel);
      unsigned char blue = pixel_buffer[0];
      unsigned char green = pixel_buffer[1];
      unsigned char red = pixel_buffer[2];
      unsigned char alpha = (red == 255 && blue == 255) ? 0x00 : 0xFF;
      texture_pixels[bmp_y * bmp_width + x] =
          (red << 24) | (green << 16) | (blue << 8) | alpha;
    }
    // Skip row padding
    int row_size = ((bmp_width * 3 + 3) / 4) * 4;
    int padding = row_size - bmp_width * 3;
    image_texture.ignore(padding);
  }

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
        // int tex_x = x - offset.x;
        // int tex_y = y - offset.y;
        pixels[y * WIDTH + x] = (v << 24) | (v << 16) | (v << 8) | 0xFF;
        // if (tex_x >= 0 && tex_x < bmp_width && tex_y >= 0 &&
        //     tex_y < bmp_height) {
        //   pixels[y * WIDTH + x] = texture_pixels[tex_y * bmp_width + tex_x];
        // } else {
        //   pixels[y * WIDTH + x] = (v << 24) | (v << 16) | (v << 8) | 0xFF;
        // }
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
