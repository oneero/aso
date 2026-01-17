#include "input.h"
#include <SDL3/SDL.h>

void aso_input_poll(Aso_Cmd_Buffer *cmds) {
  aso_clear_cmdbuffer(cmds);

  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    if (e.type == SDL_EVENT_QUIT ||
        (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE)) {
      cmds->items[cmds->count++] = (Aso_Cmd){CMD_QUIT};
    }
  }
}

void aso_clear_cmdbuffer(Aso_Cmd_Buffer *cmds) { cmds->count = 0; }
