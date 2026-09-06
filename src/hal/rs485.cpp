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
  // Isolated transceiver is populated on the first PCB. Modbus map stays
  // opt-in until a drive family is commissioned (mix / configurable).
  hw_present_ = true;
  enabled_ = false;
#if AMPED_MOCK
  return true;
#else
  pinMode(AMPED_PIN_RS485_DE, OUTPUT);
  digitalWrite(AMPED_PIN_RS485_DE, LOW);
  Serial1.begin(9600, SERIAL_8E1, AMPED_PIN_RS485_RX, AMPED_PIN_RS485_TX);
  return true;
#endif
}

bool Rs485Modbus::poll() {
  if (!hw_present_ || !enabled_) return false;
  return true;
}

Rs485Modbus& rs485() { return g_rs485; }

}  // namespace amped
