// original code inspired from https://github.com/ayufan/esphome-components

#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace memory {

class MemorySensor : public sensor::Sensor, public PollingComponent {
 public:
  void update() override;
  void dump_config() override;

  float get_setup_priority() const override;

 protected:
  uint64_t memory_{0};
};

}  // namespace memory
}  // namespace esphome
