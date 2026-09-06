#include "amped/hal.h"
#include "amped/version.h"
#include "hal/board.h"

#if !AMPED_MOCK
#include <Arduino.h>
#endif

namespace amped {
namespace {

DigitalIo g_dio;

#if !AMPED_MOCK
int run_pin(int ch) { return ch == 0 ? AMPED_PIN_RUN1 : AMPED_PIN_RUN2; }
int fault_pin(int ch) { return ch == 0 ? AMPED_PIN_FAULT1 : AMPED_PIN_FAULT2; }
#endif

}  // namespace

bool DigitalIo::begin() {
#if AMPED_MOCK
  run_[0] = run_[1] = false;
  fault_[0] = fault_[1] = false;
  return true;
#else
  pinMode(AMPED_PIN_RUN1, OUTPUT);
  pinMode(AMPED_PIN_RUN2, OUTPUT);
  digitalWrite(AMPED_PIN_RUN1, LOW);
  digitalWrite(AMPED_PIN_RUN2, LOW);
  pinMode(AMPED_PIN_FAULT1, INPUT_PULLUP);
  pinMode(AMPED_PIN_FAULT2, INPUT_PULLUP);
  pinMode(AMPED_PIN_STATUS_LED, OUTPUT);
  return true;
#endif
}

void DigitalIo::set_run(int channel, bool on) {
  if (channel < 0 || channel >= kChannelCount) return;
  run_[channel] = on;
#if !AMPED_MOCK
  digitalWrite(run_pin(channel), on ? HIGH : LOW);
#endif
}

bool DigitalIo::run(int channel) const {
  if (channel < 0 || channel >= kChannelCount) return false;
  return run_[channel];
}

bool DigitalIo::fault(int channel) const {
  if (channel < 0 || channel >= kChannelCount) return false;
#if AMPED_MOCK
  return fault_[channel];
#else
  // Opto pulls the pin low when the VFD asserts fault.
  return digitalRead(fault_pin(channel)) == LOW;
#endif
}

void DigitalIo::mock_set_fault(int channel, bool fault) {
  if (channel < 0 || channel >= kChannelCount) return;
  fault_[channel] = fault;
}

DigitalIo& dio() { return g_dio; }

}  // namespace amped
