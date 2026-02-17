#ifndef ASO_H
#define ASO_H

#include "gpu/gpu.h"
#include "input.h"
#include "window.h"
#include "mem.h"

// state context
struct aso_ctx {
  aso_window window;
  aso_vk_ctx vulkan;
  aso_cmd_buffer cmds;
  int running;
  aso_arena *scratch;  
};

extern aso_ctx *g_ctx;

void aso_init(aso_ctx *ctx);
void aso_run(void);
void aso_cleanup(void);

void aso_process_commands(aso_cmd_buffer *cmds);

#endif // ASO_H
