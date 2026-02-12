#ifndef ASO_MEM_H
#define ASO_MEM_H

#include <cstddef>
#include <cstdint>

#include "base.h"

#if defined BUILD_DEBUG && defined ASO_DEBUG_ARENA
  #define _ASO_DEBUG_ARENA
#endif

#ifdef _ASO_DEBUG_ARENA
  #define A_LOG(fmt, ...) D_LOG(fmt __VA_OPT__(,) __VA_ARGS__)
#else
  #define A_LOG(fmt, ...)  ((void)0)
#endif

#define ASO_ARENA_RESERVE_SIZE GB(64) // ensure this aligns to page size

struct aso_arena
{
  u8    *base;
  size_t size;
  size_t offset;
#ifdef _ASO_DEBUG_ARENA
  size_t alloc_count;
  size_t alloc_total;
  size_t peak;
#endif
};

static inline bool aso_is_power_of_two(uintptr_t x) {
  return (x & (x - 1)) == 0;
}

uintptr_t  aso_align_forward(uintptr_t ptr, size_t align);

aso_arena *aso_arena_create(void); // TODO: allow setting reserve size
void      *aso_arena_alloc(aso_arena *arena, size_t size, size_t align);
void       aso_arena_free(aso_arena *arena);
void       aso_arena_destroy(aso_arena *arena);

// helpers

#define ASO_ARENA_ALLOC_ARRAY(arena, type, count) \
  (type *) aso_arena_alloc((arena), sizeof(type) * (count), alignof(type))

#endif // ASO_MEM_H
