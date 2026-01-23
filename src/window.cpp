#include "window.h"
#include "core.h"
#include "gfx.h"

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

void aso_get_window_size(aso_window *window, int *width, int *height) {
  // TODO: check if we should use SDL_GetWindowSizeInPixels() instead?
  SDL_GetWindowSize(window->handle, width, height);
}

char const * const * aso_get_window_vulkan_extensions(u32 *count) {
  // TODO: find out if SDL frees these..
  return SDL_Vulkan_GetInstanceExtensions(count);
}

bool aso_create_vulkan_surface(SDL_Window *window, aso_vulkan_ctx *vulkan_ctx) {
  return SDL_Vulkan_CreateSurface(window, vulkan_ctx->instance, NULL, &vulkan_ctx->surface);
}
