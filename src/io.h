#ifndef ASO_IO_H
#define ASO_IO_H

#include "core.h"
#include "mem.h"

u8 *aso_read_binary_file(aso_arena *arena, const char *file_path, long *size);

#endif // ASO_IO_H
