#ifndef ASO_WINDOW_H
#define ASO_WINDOW_H

// Currently wraps SDL3

#include <SDL3/SDL.h>

struct aso_window {
  SDL_Window *handle;
  int width;
  int height;
};

int aso_window_init(aso_window *window);
void aso_window_cleanup(aso_window *window);
void aso_window_show(aso_window *window);

#endif // ASO_WINDOW_H
