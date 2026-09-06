#include "amped/controller.h"
#include "amped/net.h"
#include "amped/version.h"

#include <cstdio>

#if !AMPED_MOCK
#include <Preferences.h>
#endif

#if defined(__has_include)
#if __has_include("secrets.h")
#include "secrets.h"
#endif
#endif
#include "secrets.example.h"

namespace amped {
namespace {

#if !AMPED_MOCK
Preferences g_prefs;
#endif

void apply_compile_time_secrets(AppConfig& c) {
  if (!c.api_key[0] && AMPED_API_KEY[0]) {
    std::snprintf(c.api_key, sizeof(c.api_key), "%s", AMPED_API_KEY);
  }
}

}  // namespace

bool nvs_load() {
  AppConfig& c = controller().config();
#if AMPED_MOCK
  apply_compile_time_secrets(c);
  return true;
#else
  if (!g_prefs.begin("amped", true)) {
    apply_compile_time_secrets(c);
    return false;
  }
  c.heartbeat_timeout_ms = g_prefs.getUInt("hb_ms", c.heartbeat_timeout_ms);
  c.interlock_cooler_requires_pump = g_prefs.getBool("interlock", c.interlock_cooler_requires_pump);
  String key = g_prefs.getString("api_key", "");
  if (key.length() && key.length() < sizeof(c.api_key)) {
    std::snprintf(c.api_key, sizeof(c.api_key), "%s", key.c_str());
  }
  for (int i = 0; i < kChannelCount; ++i) {
    char k[16];
    std::snprintf(k, sizeof(k), "fs_%d", i);
    c.ch[i].failsafe = static_cast<FailsafeAction>(g_prefs.getUChar(k, static_cast<uint8_t>(c.ch[i].failsafe)));
    std::snprintf(k, sizeof(k), "pre_%d", i);
    c.ch[i].preset_pct = g_prefs.getFloat(k, c.ch[i].preset_pct);
  }
  g_prefs.end();
  apply_compile_time_secrets(c);
  return true;
#endif
}

bool nvs_save() {
#if AMPED_MOCK
  return true;
#else
  const AppConfig& c = controller().config();
  if (!g_prefs.begin("amped", false)) return false;
  g_prefs.putUInt("hb_ms", c.heartbeat_timeout_ms);
  g_prefs.putBool("interlock", c.interlock_cooler_requires_pump);
  g_prefs.putString("api_key", c.api_key);
  for (int i = 0; i < kChannelCount; ++i) {
    char k[16];
    std::snprintf(k, sizeof(k), "fs_%d", i);
    g_prefs.putUChar(k, static_cast<uint8_t>(c.ch[i].failsafe));
    std::snprintf(k, sizeof(k), "pre_%d", i);
    g_prefs.putFloat(k, c.ch[i].preset_pct);
  }
  g_prefs.end();
  return true;
#endif
}

}  // namespace amped
