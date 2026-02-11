#include "aso.h"

#if defined(__SANITIZE_ADDRESS__) || (defined(__has_feature) && __has_feature(address_sanitizer))
extern "C" const char *__lsan_default_suppressions() {
  return "leak:<unknown module>\n";
}
#endif

int main(int argc, char *argv[]) {
  aso_ctx ctx = {};
  aso_init(&ctx);
  aso_run();
  aso_cleanup();
  return 0;
}
