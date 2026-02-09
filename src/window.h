#ifndef ASO_WINDOW_H
#define ASO_WINDOW_H

// Currently wraps SDL3

#include "core.h"
#include "gfx.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

struct aso_window {
  SDL_Window *handle;
  int width;
  int height;
};

int aso_window_init(aso_window *window);
void aso_window_cleanup(aso_window *window);
void aso_window_show(aso_window *window);
void aso_get_window_size(aso_window *window, int *width, int *height);

char const * const * aso_get_window_vulkan_extensions(u32 *count);
bool aso_create_vulkan_surface(SDL_Window *window, aso_vk_ctx *vulkan_ctx);

void aso_wait_for_sdl_event(SDL_Event *event);

#endif // ASO_WINDOW_H
