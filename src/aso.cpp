#include "aso.h"
#include "core.h"
#include "sdl.h"

#include <SDL3/SDL.h>

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

void aso_init() {
  aso_log("aso_init\n");

  // init SDL
  sdl_init(&window, &renderer);

  // init vulkan

  // show window
  SDL_ShowWindow(window);
}

void aso_run() {
  aso_log("aso_run\n");

  bool running = true;
  SDL_Event event;

  // main loop
  while (running) {
    // poll events
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
      if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
        running = false;
      }
    }

    const double now = ((double)SDL_GetTicks()) / 1000.0;
    const float red = (float)(0.5 + 0.5 * SDL_sin(now));
    const float green = (float)(0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 2 / 3));
    const float blue = (float)(0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 4 / 3));

    // draw
    SDL_SetRenderDrawColorFloat(renderer, red, green, blue,
                                SDL_ALPHA_OPAQUE_FLOAT);
    SDL_RenderClear(renderer);

    // present
    SDL_RenderPresent(renderer);

    SDL_Delay(16);
  }
}

void aso_cleanup() {
  aso_log("aso_cleanup\n");
  // wait for idle
  // clean vulkan
  // clean sdl
  //
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}
