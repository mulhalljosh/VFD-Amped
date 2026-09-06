#include "amped/config.h"

namespace amped {

float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

}  // namespace amped
