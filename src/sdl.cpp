#include "sdl.h"
#include "core.h"

#include <SDL3/SDL.h>
// #include <SDL3/SDL_vulkan.h>

int sdl_init(SDL_Window **window, SDL_Renderer **renderer) {
  aso_log("sdl_init\n");
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    aso_log("SDL init failed: %s\n", SDL_GetError());
    return 1;
  }

  // SDL_WindowFlags wflags = SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN |
  // SDL_WINDOW_RESIZABLE;
  SDL_WindowFlags wflags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE;
  *window = SDL_CreateWindow("aso", 640, 480, wflags);
  if (!*window) {
    aso_log("SDL_CreateWindow failed: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  *renderer = SDL_CreateRenderer(*window, nullptr); // NULL);
  if (!*renderer) {
    aso_log("SDL_CreateRenderer failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(*window);
    window = NULL;
    return 1;
  }

  SDL_SetWindowPosition(*window, SDL_WINDOWPOS_CENTERED,
                        SDL_WINDOWPOS_CENTERED);

  SDL_SetRenderLogicalPresentation(*renderer, 640, 480,
                                   SDL_LOGICAL_PRESENTATION_LETTERBOX);
  aso_log("SDL ready\n");
  return 0;
}
