#ifndef VEC2_H
#define VEC2_H

class vec2 {
public:
  vec2(float x, float y);
  float x;
  float y;

  vec2 operator+(const vec2 &other) const;
};

#endif // VEC2_H
