#include "amped/controller.h"
#include "amped/hal.h"
#include "amped/net.h"
#include "amped/version.h"

#if defined(__has_include)
#if __has_include("secrets.h")
#include "secrets.h"
#endif
#endif
#include "secrets.example.h"

#include <cstdio>

namespace amped {
namespace {

bool g_mqtt_enabled = false;
uint32_t g_last_pub_ms = 0;

}  // namespace

void mqtt_begin() {
  g_mqtt_enabled = AMPED_MQTT_HOST[0] != '\0';
}

void mqtt_publish_status() {
  if (!g_mqtt_enabled) return;
  const uint32_t t = now_ms();
  if (t - g_last_pub_ms < 2000) return;
  g_last_pub_ms = t;
  // Stub: no broker client linked in v0.1. Payload shape is locked so a
  // PubSubClient / esp-mqtt body can drop in without changing the app layer.
  const SystemStatus s = controller().status();
  (void)s;
#if !AMPED_MOCK
  // Serial.printf("[mqtt] stub publish amped/vfd/status mock=%d\n", (int)s.mock);
#endif
}

}  // namespace amped
