#include "aso.h"

int main(int argc, char *argv[]) {
  aso_ctx ctx = {0};
  aso_init(&ctx);
  aso_run();
  aso_cleanup();
  return 0;
}
