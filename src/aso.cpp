#include "aso.h"
#include "core.h"
#include "gfx.h"
#include "input.h"
#include "mem.h"
#include "window.h"

aso_ctx *g_ctx = NULL;

void aso_init(aso_ctx *ctx) {
  aso_log("aso_init\n");

  // set global context pointer
  g_ctx = ctx;

  // initialize a scratch arena
  ctx->scratch = aso_arena_create();

  // init window
  aso_window_init(&ctx->window);

  // init vulkan
  aso_init_vulkan(&ctx->vulkan);

  // show window
  aso_window_show(&ctx->window);
}

void aso_run(void) {
  aso_log("aso_run\n");

  g_ctx->running = 1;

  // main loop
  while (g_ctx->running > 0) {
    aso_input_poll(&g_ctx->cmds);
    aso_process_commands(&g_ctx->cmds);
  }
}

void aso_cleanup(void) {
  aso_log("aso_cleanup\n");
  // wait for idle

  aso_cleanup_vulkan(&g_ctx->vulkan);

  aso_window_cleanup(&g_ctx->window);

  // destroy arenas
  aso_arena_destroy(g_ctx->scratch);
}

void aso_process_commands(aso_cmd_buffer *cmds) {
  for (int i = 0; i < cmds->count; i++) {
    switch (cmds->items[i].type) {
    case CMD_QUIT:
      g_ctx->running = 0;
      break;
    default:
      break;
    }
  }
}
