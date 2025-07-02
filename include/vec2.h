#pragma once

class vec2 {
public:
  vec2();
  vec2(float x, float y);
  float x;
  float y;

  vec2 operator+(const vec2 &other) const;
  vec2 operator*(float scalar) const;
};
