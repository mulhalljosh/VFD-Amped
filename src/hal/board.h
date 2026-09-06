#pragma once

// Waveshare ESP32-S3-ETH / POE-ETH class — application GPIO lock.
// See docs/io-map.md. Do not reuse W5500, USB, or octal PSRAM pins.

#define AMPED_PIN_I2C_SDA 16
#define AMPED_PIN_I2C_SCL 17
#define AMPED_I2C_FREQ_HZ 400000

#define AMPED_PIN_ONEWIRE 8

// RUN: active-high GPIO → MOSFET → 5 V coil. NO dry contacts close VFD FWD–COM.
// Coil off / power loss = contacts open = stop. Do not source 24 V onto FWD.
#define AMPED_PIN_RUN1 38
#define AMPED_PIN_FAULT1 39
#define AMPED_PIN_RUN2 40
#define AMPED_PIN_FAULT2 41

#define AMPED_PIN_RS485_TX 1
#define AMPED_PIN_RS485_RX 2
#define AMPED_PIN_RS485_DE 42

#define AMPED_PIN_STATUS_LED 21

#define AMPED_ETH_MOSI 11
#define AMPED_ETH_MISO 12
#define AMPED_ETH_SCLK 13
#define AMPED_ETH_CS 14
#define AMPED_ETH_RST 9
#define AMPED_ETH_INT 10

#define AMPED_I2C_GP8413 0x58
#define AMPED_I2C_CURRENT_PUMP 0x59
#define AMPED_I2C_CURRENT_COOLER 0x5A
