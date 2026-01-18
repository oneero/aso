#include "aso.h"
#include "core.h"
#include "input.h"
#include "window.h"
#include "gfx.h"

void aso_init(aso_ctx *ctx) {
  aso_log("aso_init\n");

  // init window
  aso_window_init(&ctx->window);

  // init vulkan
  aso_init_vulkan();

  // show window
  aso_window_show(&ctx->window);
}

void aso_run(aso_ctx *ctx) {
  aso_log("aso_run\n");

  ctx->running = 1;

  // main loop
  while (ctx->running > 0) {
    aso_input_poll(&ctx->cmds);
    aso_process_commands(ctx);
  }
}

void aso_cleanup(aso_ctx *ctx) {
  aso_log("aso_cleanup\n");
  // wait for idle
  // clean vulkan

  aso_window_cleanup(&ctx->window);
}

void aso_process_commands(aso_ctx *ctx) {
  for (int i = 0; i < ctx->cmds.count; i++) {
    switch (ctx->cmds.items[i].type) {
    case CMD_QUIT:
      ctx->running = 0;
      break;
    default:
      break;
    }
  }
}
