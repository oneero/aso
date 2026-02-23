#ifndef ASO_TIME_H
#define ASO_TIME_H

#include "base.h"
#include <ctime>

#if OS_WINDOWS
  #include <windows.h> 
#else
  #include <time.h>
#endif

struct aso_clock {
  u64 start;
  u64 freq;
};

static const u64 NS_PER_SECOND = 1'000'000'000ULL;

inline u64 aso_os_time_now(void) {
#if OS_WINDOWS
  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);
  return (u64)counter.QuadPart;
#else
  timespec ts;
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  return (u64)ts.tv_sec * NS_PER_SECOND + (u64)ts.tv_nsec;
#endif
}

inline u64 aso_os_time_freq(void) {
#if OS_WINDOWS
  LARGE_INTEGER freq;
  QueryPerformanceFrequency(&freq);
  return (u64)freq.QuadPart;
#else
  return NS_PER_SECOND;
#endif
}

inline aso_clock aso_clock_start(void) {
  aso_clock c = {
    .start = aso_os_time_now(),
    .freq = aso_os_time_freq()
  };
  return c;
}

inline f64 aso_clock_elapsed_s(aso_clock *clock) {
  u64 now = aso_os_time_now();
  return (f64)(now - clock->start)/(f64)clock->freq;
}

inline f64 aso_clock_elapsed_ms(aso_clock *clock) {
  return aso_clock_elapsed_s(clock) * 1000.0;
}
#endif // ASO_TIME_H
