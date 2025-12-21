#include "aso.h"
#include "core.h"
#include "input.h"
#include "window.h"

void aso_init(ASO_CTX *ctx) {
  aso_log("aso_init\n");

  // init window
  window_init(&ctx->window, &ctx->renderer);

  // init vulkan

  // show window
  window_show(ctx->window);
}

void aso_run(ASO_CTX *ctx) {
  aso_log("aso_run\n");

  ctx->running = 1;
  SDL_Event event;

  // main loop
  while (ctx->running > 0) {
    input_poll(&ctx->cmds);
    process_commands(ctx);
    test_draw(ctx->renderer);
  }
}

void aso_cleanup(ASO_CTX *ctx) {
  aso_log("aso_cleanup\n");
  // wait for idle
  // clean vulkan

  window_cleanup(ctx->window, ctx->renderer);
}

void process_commands(ASO_CTX *ctx) {
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
