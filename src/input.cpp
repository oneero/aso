#include "input.h"
#include <SDL3/SDL.h>

void input_poll(Aso_Cmd_Buffer *cmds) {
  cmdbuffer_clear(cmds);

  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    if (e.type == SDL_EVENT_QUIT ||
        (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE)) {
      cmds->items[cmds->count++] = (Aso_Cmd){CMD_QUIT};
    }
  }
}

void cmdbuffer_clear(Aso_Cmd_Buffer *cmds) { cmds->count = 0; }
