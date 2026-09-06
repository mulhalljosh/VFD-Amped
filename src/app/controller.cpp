#include "amped/controller.h"

#include "amped/hal.h"
#include "amped/net.h"
#include "amped/version.h"

namespace amped {
namespace {

Controller g_controller;

}  // namespace

void Controller::begin() {
  nvs_load();
  dac().begin();
  dio().begin();
  temps().begin();
  rs485().begin();
  boot_ms_ = now_ms();
  last_heartbeat_ms_ = boot_ms_;
  started_ = true;
  apply_outputs();
}

void Controller::tick() {
  if (!started_) return;
  temps().poll();
  rs485().poll();
  apply_outputs();
  mqtt_publish_status();
  ota_loop();
}

bool Controller::set_vfd(int channel, const VfdCommand& cmd) {
  if (channel < 0 || channel >= kChannelCount) return false;
  VfdCommand next = cmd;
  next.speed_pct = clampf(next.speed_pct, 0.0f, 100.0f);
  cmd_[channel] = next;
  last_ok_[channel] = next;
  touch_heartbeat();
  apply_outputs();
  return true;
}

VfdCommand Controller::command(int channel) const {
  if (channel < 0 || channel >= kChannelCount) return {};
  return cmd_[channel];
}

void Controller::touch_heartbeat() { last_heartbeat_ms_ = now_ms(); }

bool Controller::heartbeat_ok() const {
  if (cfg_.heartbeat_timeout_ms == 0) return true;
  return (now_ms() - last_heartbeat_ms_) < cfg_.heartbeat_timeout_ms;
}

void Controller::apply_config() { apply_outputs(); }

void Controller::mock_set_fault(int channel, bool fault) { dio().mock_set_fault(channel, fault); }

void Controller::apply_outputs() {
  const bool hb = heartbeat_ok();
  VfdCommand effective[kChannelCount] = {cmd_[0], cmd_[1]};

  if (!hb) {
    for (int i = 0; i < kChannelCount; ++i) {
      switch (cfg_.ch[i].failsafe) {
        case FailsafeAction::Hold:
          effective[i] = last_ok_[i];
          break;
        case FailsafeAction::Preset:
          effective[i].speed_pct = clampf(cfg_.ch[i].preset_pct, 0.0f, 100.0f);
          effective[i].run = cfg_.ch[i].preset_run;
          break;
        case FailsafeAction::Zero:
        default:
          effective[i].speed_pct = 0.0f;
          effective[i].run = false;
          break;
      }
    }
  }

  const bool pump_ok = effective[kPump].run && !dio().fault(kPump);
  bool interlock_block = false;
  if (cfg_.interlock_cooler_requires_pump && !pump_ok) {
    effective[kCooler].run = false;
    effective[kCooler].speed_pct = 0.0f;
    interlock_block = true;
  }
  (void)interlock_block;

  for (int i = 0; i < kChannelCount; ++i) {
    const bool fault = dio().fault(i);
    const bool run = effective[i].run && !fault;
    const float pct = fault ? 0.0f : effective[i].speed_pct;
    dac().write_speed(i, pct, cfg_.ch[i].analog_path);
    dio().set_run(i, run);
  }
}

SystemStatus Controller::status() const {
  SystemStatus s{};
  s.version = AMPED_FW_VERSION;
  s.mock = AMPED_MOCK;
  s.uptime_ms = now_ms() - boot_ms_;
  s.heartbeat_age_ms = now_ms() - last_heartbeat_ms_;
  s.heartbeat_ok = heartbeat_ok();
  s.network_ok = s.heartbeat_ok;
  s.interlock_enabled = cfg_.interlock_cooler_requires_pump;

  const bool pump_running = dio().run(kPump) && !dio().fault(kPump);
  s.interlock_blocking_cooler = cfg_.interlock_cooler_requires_pump && !pump_running;

  for (int i = 0; i < kChannelCount; ++i) {
    VfdStatus& v = s.vfd[i];
    v.channel = i + 1;
    v.name = channel_name(i);
    v.mode = cmd_[i].mode;
    v.speed_pct = cmd_[i].speed_pct;
    v.run = cmd_[i].run;
    v.commanded_speed_pct = dac().volts(i) * 10.0f;  // 0–10 V → 0–100 %
    v.commanded_run = dio().run(i);
    v.fault = dio().fault(i);
    v.ao_volts = dac().volts(i);
    v.ao_ma = dac().milliamps(i);
    v.failsafe = cfg_.ch[i].failsafe;
    v.analog_path = cfg_.ch[i].analog_path;
  }
  for (int i = 0; i < kTempCount; ++i) {
    s.temps[i] = temps().reading(i);
  }
  return s;
}

Controller& controller() { return g_controller; }

}  // namespace amped
