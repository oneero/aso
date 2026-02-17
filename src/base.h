#ifndef ASO_BASE_H
#define ASO_BASE_H

#include <cstdint>

// REGION: COMPILER

#if defined(__clang__)
  #define COMPILER_CLANG 1
#elif defined(_MSC_VER)
  #define COMPILER_MSVC 1
#elif defined(__GNUC__)
  #define COMPILER_GCC 1
#else
  #error "Unsupported compiler"
#endif

#if !defined(COMPILER_CLANG)
  #define COMPILER_CLANG 0
#endif
#if !defined(COMPILER_MSVC)
  #define COMPILER_MSVC 0
#endif
#if !defined(COMPILER_GCC)
  #define COMPILER_GCC 0
#endif

// REGION: OS

#if defined(_WIN32)
  #define OS_WINDOWS 1
#elif defined(__linux__)
  #define OS_LINUX 1
#else
  #error "Unsupported OS"
#endif

#if !defined(OS_WINDOWS)
  #define OS_WINDOWS 0
#endif
#if !defined(OS_LINUX)
  #define OS_LINUX 0
#endif

// REGION: ARCH

#if defined(__x86_64__) || defined(_M_AMD64)
  #define ARCH_X64 1
#elif defined(__i386__) || defined(_M_IX86)
  #define ARCH_X86 1
#elif defined(__aarch64__) || defined(_M_ARM64)
  #define ARCH_ARM64 1
#elif defined(__arm__) || defined(_M_ARM)
  #define ARCH_ARM32 1
#else
  #error "Unsupported architecture"
#endif

#if !defined(ARCH_X64)
  #define ARCH_X64 0
#endif
#if !defined(ARCH_X86)
  #define ARCH_X86 0
#endif
#if !defined(ARCH_ARM64)
  #define ARCH_ARM64 0
#endif
#if !defined(ARCH_ARM32)
  #define ARCH_ARM32 0
#endif

// REGION: BUILD TYPE

#if !defined(BUILD_DEBUG)
  #define BUILD_DEBUG 0
#endif
#if !defined(BUILD_RELEASE)
  #define BUILD_RELEASE 0
#endif

// default to debug
#if !BUILD_DEBUG && !BUILD_RELEASE
  #undef  BUILD_DEBUG
  #define BUILD_DEBUG 1
#endif

// REGION: TYPES

typedef int8_t   i8;
typedef int32_t  i32;
typedef int64_t  i64;
typedef uint8_t  u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float    f32;

// REGION: MACROS

#define STMNT(s) do { s } while(0)

// REGION: ASSERT

// trap
#if COMPILER_MSVC
  #define DEBUG_TRAP() __debugbreak()
#elif COMPILER_CLANG
  #define DEBUG_TRAP() __builtin_debugtrap()
#elif COMPILER_GCC
  // GCC doesn't have __builtin_debugtrap, so:
  #if ARCH_X64
    #define DEBUG_TRAP() __asm__ __volatile__("int3")
  #elif ARCH_ARM64
    #define DEBUG_TRAP() __asm__ __volatile__("brk #0xf000")
  #else
    #define DEBUG_TRAP() __builtin_trap()  // Last resort on GCC
  #endif
#else
  #define DEBUG_TRAP() (*(volatile int*)0 = 0)
#endif

// never stripped
#define ASSERT(expr)                                   \
  STMNT(                                               \
      if (!(expr)) {                                   \
        fprintf(stderr, "ASSERT FAILED: %s\n %s:%d\n", \
                #expr, __FILE__, __LINE__);            \
        DEBUG_TRAP();                                  \
      })

// never stripped
#define ASSERT_MSG(expr, fmt, ...)                                      \
  STMNT(                                                                \
      if (!(expr)) {                                                    \
        fprintf(stderr, "ASSERT FAILED: %s\n " fmt "\n %s:%d\n",        \
                #expr __VA_OPT__(, ) __VA_ARGS__, __FILE__, __LINE__); \
        DEBUG_TRAP();                                                   \
      })

// stripped
#ifdef BUILD_DEBUG
  #define D_ASSERT(expr) ASSERT(expr)
#else
  #define D_ASSERT(expr) ((void)0)
#endif

// REGION: LOGGING

#define LOG(fmt, ...)         fprintf(stdout, fmt "\n" __VA_OPT__(, ) __VA_ARGS__)
#define LOG_ERROR(fmt, ...)   fprintf(stderr, "[ERROR] %s:%d: " fmt "\n", __FILE__, __LINE__ __VA_OPT__(, ) __VA_ARGS__)
#define LOG_WARNING(fmt, ...) fprintf(stderr, "[WARN]  %s:%d: " fmt "\n", __FILE__, __LINE__ __VA_OPT__(, ) __VA_ARGS__)

#ifdef BUILD_DEBUG
  #define D_LOG(fmt, ...)     fprintf(stdout, "[D] " fmt "\n" __VA_OPT__(, ) __VA_ARGS__)
#else
  #define D_LOG(fmt, ...)     ((void)0)
#endif

// REGION: UTILITY

#define KB(n) ((n) * 1024ull)
#define MB(n) (KB(n) * 1024ull)
#define GB(n) (MB(n) * 1024ull)

#endif // ASO_BASE_H
