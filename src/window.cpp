#include "window.h"
#include "core.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>

int aso_window_init(aso_window *window) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    aso_log("SDL init failed: %s\n", SDL_GetError());
    return 1;
  }
  aso_log("SDL initialized\n");

  SDL_Window *w = nullptr;

  SDL_WindowFlags wflags = SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE;
  w = SDL_CreateWindow("aso", 640, 480, wflags);
  if (!w) {
    aso_log("SDL_CreateWindow failed: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }
  aso_log("window created\n");

  SDL_SetWindowPosition(w, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

  window->handle = w;
  window->width = 640;
  window->height = 480;

  aso_log("SDL ready\n");
  return 0;
}

void aso_window_cleanup(aso_window *window) {
  SDL_DestroyWindow(window->handle);
  SDL_Quit();
}

void aso_window_show(aso_window *window) {
  SDL_ShowWindow(window->handle);
}

char const * const * aso_get_window_vulkan_extensions(u32* count) {
  return SDL_Vulkan_GetInstanceExtensions(count);
}


/*
void aso_test_draw(aso_renderer *renderer) {
  const double now = ((double)SDL_GetTicks()) / 1000.0;
  const float red = (float)(0.5 + 0.5 * SDL_sin(now));
  const float green = (float)(0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 2 / 3));
  const float blue = (float)(0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 4 / 3));

  // draw
  SDL_SetRenderDrawColorFloat(renderer->handle, red, green, blue,
                              SDL_ALPHA_OPAQUE_FLOAT);
  SDL_RenderClear(renderer->handle);

  // present
  SDL_RenderPresent(renderer->handle);

  SDL_Delay(16);
}
*/
