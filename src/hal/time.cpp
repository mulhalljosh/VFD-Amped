#include "amped/hal.h"
#include "amped/version.h"

#if AMPED_MOCK
#include <ctime>
#else
#include <Arduino.h>
#endif

namespace amped {

uint32_t now_ms() {
#if AMPED_MOCK
  struct timespec ts {};
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<uint32_t>(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
#else
  return millis();
#endif
}

}  // namespace amped
