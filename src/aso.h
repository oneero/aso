#ifndef ASO_H
#define ASO_H

#include "input.h"
#include "window.h"

// state context
struct ASO_CTX {
  ASO_Window *window;
  ASO_Renderer *renderer;
  Aso_Cmd_Buffer cmds;
  int running;
};

void aso_init(ASO_CTX *ctx);
void aso_run(ASO_CTX *ctx);
void aso_cleanup(ASO_CTX *ctx);

void process_commands(ASO_CTX *ctx);

#endif // ASO_H
