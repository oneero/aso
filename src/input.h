#ifndef ASO_INPUT_H
#define ASO_INPUT_H

#define CMD_BUFFER_MAX 64

enum aso_cmd_type {
  CMD_NONE,
  CMD_QUIT,
};

struct aso_cmd {
  aso_cmd_type type;
};

struct aso_cmd_buffer {
  aso_cmd items[CMD_BUFFER_MAX];
  int count;
};

void aso_input_poll(aso_cmd_buffer *cmds);
void aso_clear_cmdbuffer(aso_cmd_buffer *cmds);

#endif // ASO_INPUT_H
