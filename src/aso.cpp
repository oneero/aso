#include "aso.h"
#include "core.h"
#include "input.h"
#include "window.h"

void aso_init(ASO_CTX *ctx) {
  aso_log("aso_init\n");

  // init window
  aso_window_init(&ctx->window, &ctx->renderer);

  // init vulkan

  // show window
  aso_window_show(&ctx->window);
}

void aso_run(ASO_CTX *ctx) {
  aso_log("aso_run\n");

  ctx->running = 1;

  // main loop
  while (ctx->running > 0) {
    aso_input_poll(&ctx->cmds);
    aso_process_commands(ctx);
    aso_test_draw(&ctx->renderer);
  }
}

void aso_cleanup(ASO_CTX *ctx) {
  aso_log("aso_cleanup\n");
  // wait for idle
  // clean vulkan

  aso_window_cleanup(&ctx->window, &ctx->renderer);
}

void aso_process_commands(ASO_CTX *ctx) {
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
