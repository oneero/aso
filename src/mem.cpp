#include "mem.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

uintptr_t aso_align_forward(uintptr_t ptr, size_t align) {
  assert(aso_is_power_of_two(align));
  return (ptr + align - 1) & ~(align - 1);
};

static size_t aso_get_os_page_size(void) {
  static size_t aso_os_page_size = 0;
  if (aso_os_page_size == 0) {
#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    aso_os_page_size = si.dwPageSize;
#else
    aso_os_page_size = sysconf(_SC_PAGESIZE);
#endif // _WIN32
  }
  return aso_os_page_size;
}

// reserve ASO_ARENA_RESERVE_SIZE of address space
// also allocate the first page for embedding the aso_arena instance
aso_arena* aso_arena_create(void) {
  size_t page_size = aso_get_os_page_size();
#ifdef _WIN32
  void *region = VirtualAlloc(NULL, ASO_ARENA_RESERVE_SIZE, MEM_RESERVE, PAGE_NOACCESS);
  if (!region) {
    return NULL;
  }
  if (!VirtualAlloc(region, page_size, MEM_COMMIT, PAGE_READWRITE)) {
    VirtualFree(region, 0, MEM_RELEASE); // free entire region
    return NULL;
  }

#else
  void *region = mmap(NULL, ASO_ARENA_RESERVE_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (region == MAP_FAILED) {
    return NULL;
  }
  if (mprotect(region, page_size, PROT_READ | PROT_WRITE) != 0) {
    munmap(region, ASO_ARENA_RESERVE_SIZE);
    return NULL;
  }
#endif

  aso_arena *arena = (aso_arena *)region;
  arena->base = (u8 *)region;
  arena->size = page_size;
  arena->offset = aso_align_forward(sizeof(aso_arena), 16);

#ifdef ASO_ARENA_DEBUG
  arena->alloc_count = 0;
  arena->alloc_total = 0;
  arena->peak = arena->offset;
#endif

  return arena;
}

void* aso_arena_alloc(aso_arena *arena, size_t size, size_t align) {
  assert(arena != NULL);
  size_t aligned_offset = aso_align_forward(arena->offset, align);
  size_t new_offset = aligned_offset + size;
  if (new_offset > ASO_ARENA_RESERVE_SIZE) {
    return NULL; // out of reserved memory
  }

  if (new_offset > arena->size) { // need to commit more memory
    // check if we are already at max capacity
    if (arena->size == ASO_ARENA_RESERVE_SIZE) {
      return NULL;
    }

    // align to page size
    size_t page_size = aso_get_os_page_size();
    size_t new_size = (new_offset + page_size - 1) / page_size * page_size;

    // clamp
    if (new_size > ASO_ARENA_RESERVE_SIZE) {
      new_size = ASO_ARENA_RESERVE_SIZE;
    }

    size_t needed = new_size - arena->size;
    void* start = arena->base + arena->size;

#ifdef _WIN32
    if (!VirtualAlloc(start, needed, MEM_COMMIT, PAGE_READWRITE)) {
      return NULL;
    }
#else
    if (mprotect(start, needed, PROT_READ | PROT_WRITE) != 0) {
      return NULL;
    }
#endif

    arena->size = new_size;
  }

  void* memory = arena->base + aligned_offset;
  arena->offset = new_offset;

  // TODO: always set to 0?

#ifdef ASO_ARENA_DEBUG 
  arena->alloc_count++;
  arena->alloc_total += size; // use offset instead?
  arena->peak = MAX(arena->peak, arena->offset);
  aso_log("[arena %p] alloc #%zu: %zu bytes @ %p (usage: %zu / %zu)\n", (void *)arena, arena->alloc_count, size, memory, arena->offset - aso_align_forward(sizeof(aso_arena), 16), arena->size);

#endif

  return memory;
}

void aso_arena_free(aso_arena *arena) {
  assert(arena != NULL);
#ifdef ASO_ARENA_DEBUG
  fprintf(stderr, "[arena %p] reset: freed %zu bytes from %zu allocations\n", (void*)arena, arena->offset - aso_align_forward(sizeof(aso_arena), 16), arena->alloc_count);
  arena->alloc_count = 0;
  arena->alloc_total = 0;
#endif
  arena->offset = aso_align_forward(sizeof(aso_arena), 16);
}

void aso_arena_destroy(aso_arena *arena) {
  assert(arena != NULL);
#ifdef _WIN32
  VirtualFree(arena->base, 0, MEM_RELEASE);
#else
  munmap(arena->base, ASO_ARENA_RESERVE_SIZE);
#endif
}
