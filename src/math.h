#ifndef ASO_MATH_H
#define ASO_MATH_H

#include "base.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(value, low, high) (MAX(low, MIN(value, high)))
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define SIGN(x) ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))
#define SQUARE(x) ((x) * (x))

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

inline v4f32 aso_linear_combine_m4f32v4f32(v4f32 left, m4f32 right) {
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

inline m4f32 aso_mul_m4f32(m4f32 left, m4f32 right) {
  m4f32 result = {};
  result.col[0] = aso_linear_combine_m4f32v4f32(right.col[0], left);
  result.col[1] = aso_linear_combine_m4f32v4f32(right.col[1], left);
  result.col[2] = aso_linear_combine_m4f32v4f32(right.col[2], left);
  result.col[3] = aso_linear_combine_m4f32v4f32(right.col[3], left);

  return result;
}

#endif // ASO_MATH_H
