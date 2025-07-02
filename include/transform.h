#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "vec2.h"
#include <cmath>

class transform {
public:
  void set_element(int row, int col, int value);
  float get_element(int row, int col) const;
  void set_x_basis(vec2 basis);
  void set_y_basis(vec2 basis);
  void set_origin(vec2 origin);
  vec2 operator*(const vec2 &other) const;
  vec2 inverse_transform(const vec2 &transformed_point) const;

private:
  float cells[2 * 3];
};

#endif // TRANSFORM_H
