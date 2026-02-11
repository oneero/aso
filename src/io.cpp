#include <cassert>
#include <stdio.h>
#include <stdlib.h>

#include "core.h"
#include "mem.h"

u8 *aso_read_binary_file(aso_arena *arena, const char *file_path, size_t *size) {
  assert(arena != NULL);
  assert(file_path != NULL);
  assert(size != NULL);

  FILE *f = fopen(file_path, "rb");
  if (!f) {
    LOG("Failed to open file: %s", file_path);
    return NULL;
  }
  
  if (fseek(f, 0, SEEK_END) != 0) {
    LOG("Failed to seek in file: %s", file_path);
    fclose(f);
    return NULL;
  }

  long fs = ftell(f);
  if (fs < 0) {
    LOG("Failed to determine file size: %s", file_path);
    fclose(f);
    return NULL;
  }
  
  if (fs == 0) {
    LOG("File is empty: %s", file_path);
    fclose(f);
    return NULL;
  }

  *size = (size_t)fs;

  rewind(f);

  u8 *buf = ASO_ARENA_ALLOC_ARRAY(arena, u8, *size);
  if (!buf) {
    LOG("Failed to allocate memory for file buffer");
    fclose(f);
    return NULL;
  }

  size_t arena_checkpoint = arena->offset;
  size_t read = fread(buf, 1, *size, f);
  if (read != *size) {
    LOG("Failed to read entire file: %s", file_path);
    fclose(f);
    arena->offset = arena_checkpoint;
    return NULL;
  }

  fclose(f);

  return buf;
}
