#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Minimal bitmap structure for 24-bit BMP files
struct Bitmap {
  int width;
  int height;
  std::vector<uint32_t> pixels; // RGBA8888 format (0xRRGGBBAA)
};

// Writes a 24-bit BMP file from the Bitmap struct (ignores alpha channel).
// Returns true on success, false on failure.
bool write_bitmap(const std::string &filename, const Bitmap &bmp);

// Reads a 24-bit BMP file into the Bitmap struct (sets alpha to 0xFF).
// Returns true on success, false on failure.
bool read_bitmap(const std::string &filename, Bitmap &bmp);
