#pragma once

#include "amped/config.h"
#include "amped/types.h"

namespace amped {

class Controller {
 public:
  void begin();
  void tick();

  bool set_vfd(int channel, const VfdCommand& cmd);
  VfdCommand command(int channel) const;

  void touch_heartbeat();
  bool heartbeat_ok() const;

  AppConfig& config() { return cfg_; }
  const AppConfig& config() const { return cfg_; }
  void apply_config();

  SystemStatus status() const;

  void mock_set_fault(int channel, bool fault);

 private:
  void apply_outputs();

  AppConfig cfg_{};
  VfdCommand cmd_[kChannelCount]{};
  VfdCommand last_ok_[kChannelCount]{};
  uint32_t boot_ms_ = 0;
  uint32_t last_heartbeat_ms_ = 0;
  bool started_ = false;
};

Controller& controller();

}  // namespace amped
