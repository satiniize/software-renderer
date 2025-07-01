#include <SDL3/SDL.h>
#include <fstream>
#include <stdint.h>
#include <string.h>

#define WIDTH 160
#define HEIGHT 90

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

int main(int argc, char *argv[]) {
  int image_width = 16;
  int image_height = 16;

  int row_size = ((image_width * 3 + 3) / 4) * 4;
  int padding = row_size - image_width * 3;
  int image_size = row_size * image_height;
  int file_size = 14 + 40 + image_size;

  std::ofstream out("test_image.bmp", std::ios::binary);

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

  out.close();

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
        pixels[y * WIDTH + x] = (0xFF << 24) | (v << 16) | (v << 8) | v;
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
