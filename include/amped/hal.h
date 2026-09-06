#pragma once

#include "amped/types.h"

#include <cstdint>

namespace amped {

class DacGp8413 {
 public:
  static constexpr uint8_t kAddrVoltage = 0x58;
  static constexpr uint8_t kAddrCurrent = 0x59;
  static constexpr uint8_t kRegRange = 0x01;
  static constexpr uint8_t kRegCh0 = 0x02;
  static constexpr uint8_t kRegCh1 = 0x04;
  static constexpr uint8_t kRange10V = 0x77;
  static constexpr uint16_t kFullScale = 0x7FFF;

  bool begin();
  bool set_range_10v();
  bool write_speed(int channel, float speed_pct, AnalogPath path);
  float volts(int channel) const;
  float milliamps(int channel) const;
  uint16_t code(int channel) const;
  bool present() const { return present_; }

  static uint16_t pct_to_code(float speed_pct);
  static float code_to_volts(uint16_t code);
  static float pct_to_ma(float speed_pct);

 private:
  bool write_channel_reg(uint8_t addr, uint8_t reg, uint16_t code15);
  uint16_t codes_[kChannelCount] = {};
  bool present_ = false;
};

class DigitalIo {
 public:
  bool begin();
  void set_run(int channel, bool on);
  bool run(int channel) const;
  bool fault(int channel) const;
  void mock_set_fault(int channel, bool fault);

 private:
  bool run_[kChannelCount] = {};
  bool fault_[kChannelCount] = {};
};

class TempSensors {
 public:
  bool begin();
  void poll();
  TempReading reading(int index) const;

 private:
  TempReading temps_[kTempCount] = {};
};

class Rs485Modbus {
 public:
  bool begin();
  bool poll();
  bool enabled() const { return enabled_; }

 private:
  bool enabled_ = false;
};

DacGp8413& dac();
DigitalIo& dio();
TempSensors& temps();
Rs485Modbus& rs485();

uint32_t now_ms();

}  // namespace amped
