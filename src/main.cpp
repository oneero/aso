#include "aso.h"

#if defined(__SANITIZE_ADDRESS__)
extern "C" const char *__lsan_default_suppressions() {
  return "leak:<unknown module>\n";
}
#endif

int main(int argc, char *argv[]) {
  aso_ctx ctx = {0};
  aso_init(&ctx);
  aso_run();
  aso_cleanup();
  return 0;
}
