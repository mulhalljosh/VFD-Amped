#include "amped/hal.h"
#include "amped/config.h"
#include "amped/version.h"
#include "hal/board.h"

#if !AMPED_MOCK
#include <Wire.h>
#endif

namespace amped {
namespace {

DacGp8413 g_dac;

}  // namespace

uint16_t DacGp8413::pct_to_code(float speed_pct) {
  float p = clampf(speed_pct, 0.0f, 100.0f);
  return static_cast<uint16_t>((p / 100.0f) * static_cast<float>(kFullScale) + 0.5f);
}

float DacGp8413::code_to_volts(uint16_t code) {
  return 10.0f * (static_cast<float>(code) / static_cast<float>(kFullScale));
}

float DacGp8413::pct_to_ma(float speed_pct) {
  return 4.0f + 16.0f * (clampf(speed_pct, 0.0f, 100.0f) / 100.0f);
}

bool DacGp8413::write_channel_reg(uint8_t addr, uint8_t reg, uint16_t code15) {
  if (code15 > kFullScale) code15 = kFullScale;
#if AMPED_MOCK
  (void)addr;
  (void)reg;
  (void)code15;
  return true;
#else
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(static_cast<uint8_t>(code15 & 0xFF));
  Wire.write(static_cast<uint8_t>((code15 >> 8) & 0x7F));
  return Wire.endTransmission() == 0;
#endif
}

bool DacGp8413::begin() {
#if AMPED_MOCK
  present_ = true;
  codes_[0] = codes_[1] = 0;
  return true;
#else
  Wire.begin(AMPED_PIN_I2C_SDA, AMPED_PIN_I2C_SCL, AMPED_I2C_FREQ_HZ);
  present_ = set_range_10v();
  write_speed(0, 0.0f, AnalogPath::Both);
  write_speed(1, 0.0f, AnalogPath::Both);
  return present_;
#endif
}

bool DacGp8413::set_range_10v() {
#if AMPED_MOCK
  return true;
#else
  Wire.beginTransmission(kAddrVoltage);
  Wire.write(kRegRange);
  Wire.write(kRange10V);
  return Wire.endTransmission() == 0;
#endif
}

bool DacGp8413::write_speed(int channel, float speed_pct, AnalogPath path) {
  if (channel < 0 || channel >= kChannelCount) return false;
  const uint16_t code15 = pct_to_code(speed_pct);
  codes_[channel] = code15;
  const uint8_t reg = (channel == 0) ? kRegCh0 : kRegCh1;
  bool ok = true;
  if (path == AnalogPath::Voltage || path == AnalogPath::Both) {
    ok = write_channel_reg(kAddrVoltage, reg, code15) && ok;
  }
  if (path == AnalogPath::Current || path == AnalogPath::Both) {
    // Companion current DAC (optional 0x59) or V/I transmitter driven by the
    // same 0–10 V. Write is a no-op ACK miss on hardware without the chip.
    write_channel_reg(kAddrCurrent, reg, code15);
  }
  return ok;
}

float DacGp8413::volts(int channel) const {
  if (channel < 0 || channel >= kChannelCount) return 0.0f;
  return code_to_volts(codes_[channel]);
}

float DacGp8413::milliamps(int channel) const {
  if (channel < 0 || channel >= kChannelCount) return 4.0f;
  return 4.0f + 16.0f * (static_cast<float>(codes_[channel]) / static_cast<float>(kFullScale));
}

uint16_t DacGp8413::code(int channel) const {
  if (channel < 0 || channel >= kChannelCount) return 0;
  return codes_[channel];
}

DacGp8413& dac() { return g_dac; }

}  // namespace amped
