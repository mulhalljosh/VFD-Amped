#include "amped/controller.h"
#include "amped/net.h"
#include "amped/version.h"

#if !AMPED_MOCK
#include <ArduinoOTA.h>
#endif

namespace amped {

void ota_begin() {
  // USB-C first. OTA is a later stub — off unless config.ota_enabled.
#if AMPED_MOCK
  return;
#else
  if (!controller().config().ota_enabled) return;
  ArduinoOTA.setHostname(controller().config().hostname);
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
