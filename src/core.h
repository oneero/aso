#ifndef ASO_CORE_H
#define ASO_CORE_H

#include <cstdarg>
#include <cstdint>
#include <cstdio>

// types
typedef int8_t i8;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;

// util
#define KB(n) ((n) * 1024ull)
#define MB(n) (KB(n) * 1024ull)
#define GB(n) (MB(n) * 1024ull)

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(value, low, high) (MAX(low, MIN(value, high)))
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define SIGN(x) ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))
#define SQUARE(x) ((x) * (x))

// TODO: add proper logging and assertion

// logging function
// currently simply wraps printf
static inline void aso_log(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
}

#endif // ASO_CORE_H
