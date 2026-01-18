#ifndef ASO_H
#define ASO_H

#include "input.h"
#include "window.h"

// state context
struct aso_ctx {
  aso_window window;
  //aso_renderer renderer;
  aso_vulkan_ctx vulkan;
  aso_cmd_buffer cmds;
  int running;
};

void aso_init(aso_ctx *ctx);
void aso_run(aso_ctx *ctx);
void aso_cleanup(aso_ctx *ctx);

void aso_process_commands(aso_ctx *ctx);

#endif // ASO_H
