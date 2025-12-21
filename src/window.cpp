#include "window.h"
#include "core.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
// #include <SDL3/SDL_vulkan.h>

int window_init(ASO_Window **window, ASO_Renderer **renderer) {
  aso_log("sdl_init\n");
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    aso_log("SDL init failed: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Window *w = nullptr;
  SDL_Renderer *r = nullptr;

  // SDL_WindowFlags wflags = SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN |
  // SDL_WINDOW_RESIZABLE;
  SDL_WindowFlags wflags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE;
  w = SDL_CreateWindow("aso", 640, 480, wflags);
  if (!w) {
    aso_log("SDL_CreateWindow failed: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  r = SDL_CreateRenderer(w, nullptr); // NULL);
  if (!r) {
    aso_log("SDL_CreateRenderer failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(w);
    w = NULL;
    return 1;
  }

  SDL_SetWindowPosition(w, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

  SDL_SetRenderLogicalPresentation(r, 640, 480,
                                   SDL_LOGICAL_PRESENTATION_LETTERBOX);

  (*window)->handle = w;
  (*window)->width = 640;
  (*window)->height = 480;
  (*renderer)->handle = r;

  aso_log("SDL ready\n");
  return 0;
}

void window_cleanup(ASO_Window *window, ASO_Renderer *renderer) {
  SDL_DestroyRenderer(renderer->handle);
  SDL_DestroyWindow(window->handle);
  SDL_Quit();
}

void window_show(ASO_Window *window) { SDL_ShowWindow(window->handle); }

void test_draw(ASO_Renderer *renderer) {
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
