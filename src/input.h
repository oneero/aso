#ifndef ASO_INPUT_H
#define ASO_INPUT_H

#define CMD_BUFFER_MAX 64

enum Aso_Cmd_Type {
  CMD_NONE,
  CMD_QUIT,
};

struct Aso_Cmd {
  Aso_Cmd_Type type;
};

struct Aso_Cmd_Buffer {
  Aso_Cmd items[CMD_BUFFER_MAX];
  int count;
};

void aso_input_poll(Aso_Cmd_Buffer *cmds);
void aso_clear_cmdbuffer(Aso_Cmd_Buffer *cmds);

#endif // ASO_INPUT_H
