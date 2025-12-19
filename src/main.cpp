#include "aso.h"

int main(int argc, char *argv[]) {
  aso_init();
  aso_run();
  aso_cleanup();
  return 0;
}
