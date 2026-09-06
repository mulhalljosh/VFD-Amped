#pragma once

#include "amped/types.h"

#include <cstdint>
#include <cstring>

namespace amped {

struct ChannelConfig {
  FailsafeAction failsafe = FailsafeAction::Zero;
  float preset_pct = 0.0f;
  bool preset_run = false;
  AnalogPath analog_path = AnalogPath::Both;
};

// v0.1 auto = outdoor/water temp-band. Law is stubbed; these are the hooks.
struct TempBandConfig {
  float outdoor_low_c = 5.0f;
  float outdoor_high_c = 30.0f;
  float water_low_c = 10.0f;
  float water_high_c = 40.0f;
};

struct AppConfig {
  ChannelConfig ch[kChannelCount];
  uint32_t heartbeat_timeout_ms = 15000;
  bool interlock_cooler_requires_pump = false;
  char api_key[64] = {};
  char hostname[32] = "amped-vfd";
  char drive_family[24] = "mix";
  bool rs485_populated = true;
  bool modbus_enabled = false;
  bool ota_enabled = false;
  TempBandConfig temp_band{};
  uint64_t temp_rom[kTempCount] = {};
};

inline AppConfig default_config() { return AppConfig{}; }

inline VfdMode parse_mode(const char* s) {
  if (s && (s[0] == 'a' || s[0] == 'A')) return VfdMode::Auto;
  return VfdMode::Manual;
}

inline FailsafeAction parse_failsafe(const char* s) {
  if (!s) return FailsafeAction::Zero;
  if (s[0] == 'h' || s[0] == 'H') return FailsafeAction::Hold;
  if (s[0] == 'p' || s[0] == 'P') return FailsafeAction::Preset;
  return FailsafeAction::Zero;
}

inline AnalogPath parse_analog_path(const char* s) {
  if (!s) return AnalogPath::Both;
  if (std::strcmp(s, "voltage") == 0) return AnalogPath::Voltage;
  if (std::strcmp(s, "current") == 0) return AnalogPath::Current;
  return AnalogPath::Both;
}

float clampf(float v, float lo, float hi);

}  // namespace amped
