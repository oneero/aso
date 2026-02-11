#include "input.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

void aso_input_poll(aso_cmd_buffer *cmds) {
  aso_clear_cmdbuffer(cmds);

  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    if (e.type == SDL_EVENT_QUIT ||
        (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE)) {
      cmds->items[cmds->count++] = aso_cmd{CMD_QUIT};
    }
    if (e.type == SDL_EVENT_WINDOW_RESIZED) {
      cmds->items[cmds->count++] = aso_cmd{CMD_WIN_RESIZE};
    }
  }
}

void aso_clear_cmdbuffer(aso_cmd_buffer *cmds) { cmds->count = 0; }
