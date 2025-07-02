#include "vec2.h"

vec2::vec2() : x(0.0f), y(0.0f) {}

vec2::vec2(float x, float y) : x(x), y(y) {}

vec2 vec2::operator+(const vec2 &other) const {
  return vec2(x + other.x, y + other.y);
}

vec2 vec2::operator*(float scalar) const {
  return vec2(x * scalar, y * scalar);
}
