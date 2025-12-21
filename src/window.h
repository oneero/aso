#ifndef ASO_WINDOW_H
#define ASO_WINDOW_H

// Currently wraps SDL3

#include <SDL3/SDL.h>

struct ASO_Window {
  SDL_Window *handle;
  int width;
  int height;
};

// renderer used for testing
// todo: remove
struct ASO_Renderer {
  SDL_Renderer *handle;
};

int window_init(ASO_Window **window, ASO_Renderer **renderer);
void window_cleanup(ASO_Window *window, ASO_Renderer *renderer);

void window_show(ASO_Window *window);
void test_draw(ASO_Renderer *renderer);

#endif // ASO_WINDOW_H
