#ifndef ASO_WINDOW_H
#define ASO_WINDOW_H

// Currently wraps SDL3

#include <SDL3/SDL.h>

struct aso_window {
  SDL_Window *handle;
  int width;
  int height;
};

// renderer used for testing
// todo: remove
struct aso_renderer {
  SDL_Renderer *handle;
};

int aso_window_init(aso_window *window, aso_renderer *renderer);
void aso_window_cleanup(aso_window *window, aso_renderer *renderer);

void aso_window_show(aso_window *window);
void aso_test_draw(aso_renderer *renderer);

#endif // ASO_WINDOW_H
