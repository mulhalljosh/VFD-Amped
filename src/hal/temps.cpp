#include "amped/hal.h"
#include "amped/version.h"
#include "hal/board.h"

#include <cmath>

#if !AMPED_MOCK
#include <DallasTemperature.h>
#include <OneWire.h>
#endif

namespace amped {
namespace {

TempSensors g_temps;

const char* kIds[kTempCount] = {"ambient", "water_in", "water_out"};
const char* kLabels[kTempCount] = {"Ambient", "Water in", "Water out"};

#if !AMPED_MOCK
OneWire g_ow(AMPED_PIN_ONEWIRE);
DallasTemperature g_dt(&g_ow);
#endif

}  // namespace

bool TempSensors::begin() {
  for (int i = 0; i < kTempCount; ++i) {
    temps_[i].id = kIds[i];
    temps_[i].label = kLabels[i];
    temps_[i].celsius = 0;
    temps_[i].valid = false;
  }
#if AMPED_MOCK
  temps_[0].celsius = 21.3f;  // onboard ambient / outdoor
  temps_[1].celsius = 42.1f;  // water in
  temps_[2].celsius = 28.4f;  // water out
  temps_[0].valid = temps_[1].valid = temps_[2].valid = true;
  return true;
#else
  g_dt.begin();
  g_dt.setWaitForConversion(false);
  g_dt.requestTemperatures();
  return g_dt.getDeviceCount() > 0;
#endif
}

void TempSensors::poll() {
#if AMPED_MOCK
  const float t = static_cast<float>(now_ms()) / 1000.0f;
  temps_[0].celsius = 21.3f + 0.3f * std::sin(t / 13.0f);
  temps_[1].celsius = 42.1f + 1.2f * std::sin(t / 11.0f);
  temps_[2].celsius = 28.4f + 0.4f * std::sin(t / 7.0f);
  temps_[0].valid = temps_[1].valid = temps_[2].valid = true;
#else
  const int n = g_dt.getDeviceCount();
  for (int i = 0; i < kTempCount; ++i) {
    if (i < n) {
      float c = g_dt.getTempCByIndex(i);
      temps_[i].valid = (c > -50.0f && c < 125.0f);
      temps_[i].celsius = temps_[i].valid ? c : 0.0f;
    } else {
      temps_[i].valid = false;
    }
  }
  g_dt.requestTemperatures();
#endif
}

TempReading TempSensors::reading(int index) const {
  if (index < 0 || index >= kTempCount) return TempReading{};
  return temps_[index];
}

TempSensors& temps() { return g_temps; }

}  // namespace amped
