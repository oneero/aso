#include "aso.h"
#include "base.h"
#include "gpu/gpu.h"
#include "input.h"
#include "mem.h"
#include "window.h"

aso_ctx *g_ctx = NULL;

void aso_init(aso_ctx *ctx) {
  LOG("aso_init");

  // set global context pointer
  g_ctx = ctx;

  // initialize a scratch arena
  ctx->scratch = aso_arena_create();

  // init window
  aso_window_init(&ctx->window);

  // init vulkan
  aso_vk_init(ctx->scratch, &ctx->vulkan);

  // show window
  aso_window_show(&ctx->window);
}

void aso_run(void) {
  LOG("aso_run");

  g_ctx->running = 1;

  int frame = 0;
  int max_frames = 10000;

  // main loop
  while (g_ctx->running > 0 && frame < max_frames) {
    aso_input_poll(&g_ctx->cmds);
    aso_process_commands(&g_ctx->cmds);
    aso_vk_draw_frame(&g_ctx->vulkan);
    frame++;
  }
}

void aso_cleanup(void) {
  LOG("aso_cleanup");

  aso_vk_cleanup(&g_ctx->vulkan);

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
    case CMD_WIN_RESIZE:
      g_ctx->vulkan.window_resized = true;
    default:
      break;
    }
  }
}
