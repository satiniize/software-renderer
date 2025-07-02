#include "transform.h"

Transform::Transform() {
  cells[0] = 1.0f;
  cells[1] = 0.0f;
  cells[2] = 0.0f;
  cells[3] = 0.0f;
  cells[4] = 1.0f;
  cells[5] = 0.0f;
}

// Set the value at (row, col)
void Transform::set_element(int row, int col, int value) {
  if (row < 0 || row > 1 || col < 0 || col > 2)
    return;
  cells[row * 3 + col] = static_cast<float>(value);
}

// Get the value at (row, col)
float Transform::get_element(int row, int col) const {
  if (row < 0 || row > 1 || col < 0 || col > 2)
    return 0.0f;
  return cells[row * 3 + col];
}

// Set the x basis vector (first column)
void Transform::set_x_basis(vec2 basis) {
  cells[0] = basis.x;
  cells[3] = basis.y;
}

// Set the y basis vector (second column)
void Transform::set_y_basis(vec2 basis) {
  cells[1] = basis.x;
  cells[4] = basis.y;
}

// Set the origin (translation, third column)
void Transform::set_origin(vec2 origin) {
  cells[2] = origin.x;
  cells[5] = origin.y;
}

// Apply the Transform to a vec2
vec2 Transform::operator*(const vec2 &other) const {
  float x = cells[0] * other.x + cells[1] * other.y + cells[2];
  float y = cells[3] * other.x + cells[4] * other.y + cells[5];
  return vec2(x, y);
}

// Compute the inverse Transform of a point
vec2 Transform::inverse_transform(const vec2 &point) const {
  // Extract basis and origin
  float a = cells[0], c = cells[1], tx = cells[2];
  float b = cells[3], d = cells[4], ty = cells[5];

  // Subtract origin (translation)
  float px = point.x - tx;
  float py = point.y - ty;

  // Compute determinant
  float det = a * d - b * c;
  if (fabs(det) < 1e-8f) {
    // Singular matrix, return zero vector (or handle error as needed)
    return vec2(0, 0);
  }
  float inv_det = 1.0f / det;

  // Apply inverse matrix
  float x = (d * px - c * py) * inv_det;
  float y = (-b * px + a * py) * inv_det;

  return vec2(x, y);
}
