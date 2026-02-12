#include "base.h"
#include "window.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>

int aso_window_init(aso_window *window) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    LOG("SDL init failed: %s", SDL_GetError());
    return 1;
  }
  LOG("SDL initialized");

  SDL_Window *w = nullptr;

  SDL_WindowFlags wflags = SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE;
  w = SDL_CreateWindow("aso", 640, 480, wflags);
  if (!w) {
    LOG("SDL_CreateWindow failed: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }
  LOG("window created");

  SDL_SetWindowPosition(w, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

  window->handle = w;
  window->width = 640;
  window->height = 480;

  LOG("SDL ready");
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
  SDL_GetWindowSizeInPixels(window->handle, width, height);
}

char const * const * aso_get_window_vulkan_extensions(u32 *count) {
  // TODO: find out if SDL frees these..
  return SDL_Vulkan_GetInstanceExtensions(count);
}

bool aso_create_vulkan_surface(SDL_Window *window, VkInstance vk_instance, VkSurfaceKHR *surface) {
  return SDL_Vulkan_CreateSurface(window, vk_instance, NULL, surface);
}

void aso_wait_for_sdl_event(SDL_Event *event) {
  SDL_WaitEvent(event);
}
