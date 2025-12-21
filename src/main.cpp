#include "aso.h"

int main(int argc, char *argv[]) {
  ASO_CTX ctx = {0};
  aso_init(&ctx);
  aso_run(&ctx);
  aso_cleanup(&ctx);
  return 0;
}
