#ifndef ASO_MATH_H
#define ASO_MATH_H

#include "base.h"
#include <cmath>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(value, low, high) (MAX(low, MIN(value, high)))
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define SIGN(x) ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))
#define SQUARE(x) ((x) * (x))

// TODO: consider using turns only
#define ASO_PI 3.14159265358979323846f

union v2f32 {
  struct {
    f32 x;
    f32 y;
  };
  f32 v[2];
};

union v3f32 {
  struct {
    f32 x;
    f32 y;
    f32 z;
  };
  f32 v[3];
};

union v4f32 {
  struct {
    f32 x;
    f32 y;
    f32 z;
    f32 w;
  };
  f32 v[4];
};

union m4f32 {
  v4f32 col[4];
  f32   v[4][4];
};

// many functions here are referencing
// https://github.com/HandmadeMath/HandmadeMath

inline f32 aso_rad(f32 degrees) {
  return degrees * (ASO_PI / 180.0f);
}

inline f32 aso_sqrtf(f32 value) {
  return sqrtf(value);
}

inline f32 aso_isqrtf(f32 value) {
  return 1.0f / aso_sqrtf(value);
}

inline v3f32 aso_add_v3(v3f32 left, v3f32 right) {
  v3f32 result = {};

  result.x = left.x + right.x;
  result.y = left.y + right.y;
  result.z = left.z + right.z;

  return result;
}

inline v3f32 aso_sub_v3(v3f32 left, v3f32 right) {
  v3f32 result = {};

  result.x = left.x - right.x;
  result.y = left.y - right.y;
  result.z = left.z - right.z;

  return result;
}

inline f32 aso_dot_v3(v3f32 left, v3f32 right) {
  return (left.x * right.x) + (left.y * right.y) + (left.z * right.z);
}

inline v3f32 aso_cross_v3(v3f32 left, v3f32 right) {
  v3f32 result = {};

  result.x = (left.y * right.z) - (left.z * right.y);
  result.y = (left.z * right.x) - (left.x * right.z);
  result.z = (left.x * right.y) - (left.y * right.x);

  return result;
}

inline v3f32 aso_mul_v3f(v3f32 left, f32 right) {
  v3f32 result = {};

  result.x = left.x * right;
  result.y = left.y * right;
  result.z = left.z * right;

  return result;
}

inline v3f32 aso_norm_v3(v3f32 vector) {
  return aso_mul_v3f(vector, aso_isqrtf(aso_dot_v3(vector, vector))); 
}

inline v4f32 aso_linear_combine_m4v4(v4f32 left, m4f32 right) {
  v4f32 result = {};

  result.v[0]  = left.v[0] * right.v[0][0];
  result.v[1]  = left.v[0] * right.v[0][1];
  result.v[2]  = left.v[0] * right.v[0][2];
  result.v[3]  = left.v[0] * right.v[0][3];

  result.v[0] += left.v[1] * right.v[1][0];
  result.v[1] += left.v[1] * right.v[1][1];
  result.v[2] += left.v[1] * right.v[1][2];
  result.v[3] += left.v[1] * right.v[1][3];

  result.v[0] += left.v[2] * right.v[2][0];
  result.v[1] += left.v[2] * right.v[2][1];
  result.v[2] += left.v[2] * right.v[2][2];
  result.v[3] += left.v[2] * right.v[2][3];

  result.v[0] += left.v[3] * right.v[3][0];
  result.v[1] += left.v[3] * right.v[3][1];
  result.v[2] += left.v[3] * right.v[3][2];
  result.v[3] += left.v[3] * right.v[3][3];

  return result;
}

inline m4f32 aso_mul_m4(m4f32 left, m4f32 right) {
  m4f32 result = {};
  result.col[0] = aso_linear_combine_m4v4(right.col[0], left);
  result.col[1] = aso_linear_combine_m4v4(right.col[1], left);
  result.col[2] = aso_linear_combine_m4v4(right.col[2], left);
  result.col[3] = aso_linear_combine_m4v4(right.col[3], left);

  return result;
}

inline m4f32 aso_m4d(float diagonal) {
  m4f32 result = {};
  result.v[0][0] = diagonal;
  result.v[1][1] = diagonal;
  result.v[2][2] = diagonal;
  result.v[3][3] = diagonal;
  return result;
}

inline m4f32 aso_m4i(void) {
  return aso_m4d(1.0f);
}

// rodrigues rotation
// right handed
// assumes axis is normalized
inline m4f32 aso_rot_m4(f32 angle, v3f32 axis) {
  m4f32 result = aso_m4i();

  f32 sin_angle = sinf(angle);
  f32 cos_angle = cosf(angle);
  f32 one_minus_cos = 1.0f - cos_angle;

  result.v[0][0] = (axis.x * axis.x * one_minus_cos) + cos_angle;
  result.v[0][1] = (axis.x * axis.y * one_minus_cos) + (axis.z * sin_angle);
  result.v[0][2] = (axis.x * axis.z * one_minus_cos) - (axis.y * sin_angle);
  
  result.v[1][0] = (axis.y * axis.x * one_minus_cos) - (axis.z * sin_angle);
  result.v[1][1] = (axis.y * axis.y * one_minus_cos) + cos_angle;
  result.v[1][2] = (axis.y * axis.z * one_minus_cos) + (axis.x * sin_angle);
  
  result.v[2][0] = (axis.z * axis.x * one_minus_cos) + (axis.y * sin_angle);
  result.v[2][1] = (axis.z * axis.y * one_minus_cos) - (axis.x * sin_angle);
  result.v[2][2] = (axis.z * axis.z * one_minus_cos) + cos_angle;

  return result;
}

// right handed, forward = -z
inline m4f32 aso_lookat(v3f32 from, v3f32 to, v3f32 up) {
  m4f32 result = {};

  v3f32 f = aso_norm_v3(aso_sub_v3(to, from)); // forward direction
  v3f32 s = aso_norm_v3(aso_cross_v3(f, up));  // side
  v3f32 u = aso_cross_v3(s, f);                        // up

  // axes are transposed to invert the rotation
  // forward/z is flipped
  // translation is projected onto axes and flipped

  result.v[0][0] = s.x;
  result.v[0][1] = u.x;
  result.v[0][2] = -f.x;
  result.v[0][3] = 0.0f;

  result.v[1][0] = s.y;
  result.v[1][1] = u.y;
  result.v[1][2] = -f.y;
  result.v[1][3] = 0.0f;

  result.v[2][0] = s.z;
  result.v[2][1] = u.z;
  result.v[2][2] = -f.z;
  result.v[2][3] = 0.0f;

  result.v[3][0] = -aso_dot_v3(s, from);
  result.v[3][1] = -aso_dot_v3(u, from);
  result.v[3][2] = aso_dot_v3(f, from); // already inverse
  result.v[3][3] = 1.0f;

  return result;
}

inline m4f32 aso_perspective(f32 fov, f32 aspect_ratio, f32 near, f32 far) {
  m4f32 result = {};

  f32 cotan = 1.0f / tanf(fov / 2.0f);
  result.v[0][0] = cotan / aspect_ratio;
  result.v[1][1] = -cotan; // flipped
  result.v[2][3] = -1.0f;
  result.v[2][2] = (far) / (near - far);
  result.v[3][2] = (near * far) / (near - far);

  return result;
}

// REGION: OPERATOR OVERLOADS

inline v3f32 operator+(v3f32 left, v3f32 right) {
  return aso_add_v3(left, right);
}

inline v3f32 operator-(v3f32 left, v3f32 right) {
  return aso_sub_v3(left, right);
}

inline v3f32 operator*(v3f32 left, f32 right) {
  return aso_mul_v3f(left, right);
}

inline m4f32 operator*(m4f32 left, m4f32 right) {
  return aso_mul_m4(left, right);
}

#endif // ASO_MATH_H
