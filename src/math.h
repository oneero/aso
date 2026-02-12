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

#endif // ASO_MATH_H
