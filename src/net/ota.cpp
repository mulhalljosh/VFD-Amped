#include "amped/net.h"
#include "amped/version.h"

#if !AMPED_MOCK
#include <ArduinoOTA.h>
#endif

namespace amped {

void ota_begin() {
#if AMPED_MOCK
  return;
#else
  // Stub: hostname + no password until NVS/secrets land a real policy.
  ArduinoOTA.setHostname("amped-vfd");
  ArduinoOTA.begin();
#endif
}

void ota_loop() {
#if AMPED_MOCK
  return;
#else
  ArduinoOTA.handle();
#endif
}

}  // namespace amped
