#include "amped/hal.h"
#include "amped/version.h"
#include "hal/board.h"

#if !AMPED_MOCK
#include <Arduino.h>
#endif

namespace amped {
namespace {

Rs485Modbus g_rs485;

}  // namespace

bool Rs485Modbus::begin() {
#if AMPED_MOCK
  enabled_ = false;
  return true;
#else
  pinMode(AMPED_PIN_RS485_DE, OUTPUT);
  digitalWrite(AMPED_PIN_RS485_DE, LOW);
  Serial1.begin(9600, SERIAL_8E1, AMPED_PIN_RS485_RX, AMPED_PIN_RS485_TX);
  enabled_ = false;  // opt-in once a slave map is commissioned
  return true;
#endif
}

bool Rs485Modbus::poll() {
  // v0.1: master stub only. No vendor register map yet.
  return enabled_;
}

Rs485Modbus& rs485() { return g_rs485; }

}  // namespace amped
