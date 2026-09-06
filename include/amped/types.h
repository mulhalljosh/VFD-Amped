#pragma once

#include <cstdint>

namespace amped {

enum class VfdMode : uint8_t { Manual = 0, Auto = 1 };

enum class FailsafeAction : uint8_t { Hold = 0, Zero = 1, Preset = 2 };

enum class AnalogPath : uint8_t { Voltage = 0, Current = 1, Both = 2 };

constexpr int kChannelCount = 2;
constexpr int kTempCount = 3;
constexpr int kPump = 0;
constexpr int kCooler = 1;

inline const char* channel_name(int ch) {
  if (ch == kPump) return "pump";
  if (ch == kCooler) return "cooler";
  return "unknown";
}

inline const char* mode_name(VfdMode m) { return m == VfdMode::Auto ? "auto" : "manual"; }

inline const char* failsafe_name(FailsafeAction a) {
  switch (a) {
    case FailsafeAction::Hold:
      return "hold";
    case FailsafeAction::Preset:
      return "preset";
    case FailsafeAction::Zero:
    default:
      return "zero";
  }
}

inline const char* analog_path_name(AnalogPath p) {
  switch (p) {
    case AnalogPath::Current:
      return "current";
    case AnalogPath::Both:
      return "both";
    case AnalogPath::Voltage:
    default:
      return "voltage";
  }
}

struct VfdCommand {
  VfdMode mode = VfdMode::Manual;
  float speed_pct = 0.0f;
  bool run = false;
};

struct VfdStatus {
  int channel = 0;
  const char* name = "";
  VfdMode mode = VfdMode::Manual;
  float speed_pct = 0.0f;
  float commanded_speed_pct = 0.0f;
  bool run = false;
  bool commanded_run = false;
  bool fault = false;
  float ao_volts = 0.0f;
  float ao_ma = 4.0f;
  FailsafeAction failsafe = FailsafeAction::Zero;
  AnalogPath analog_path = AnalogPath::Both;
};

struct TempReading {
  const char* id = "";
  const char* label = "";
  float celsius = 0.0f;
  bool valid = false;
};

struct SystemStatus {
  VfdStatus vfd[kChannelCount];
  TempReading temps[kTempCount];
  bool network_ok = true;
  bool heartbeat_ok = true;
  uint32_t uptime_ms = 0;
  uint32_t heartbeat_age_ms = 0;
  bool interlock_enabled = false;
  bool interlock_blocking_cooler = false;
  bool mock = false;
  const char* version = "";
};

}  // namespace amped
