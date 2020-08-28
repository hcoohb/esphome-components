#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/remote_base/rc_switch_protocol.h"

namespace esphome {
namespace otio {

class OTIO : public Component, public sensor::Sensor, public remote_base::RemoteReceiverListener {
 public:
  OTIO(){
      protocol_ = remote_base::RCSwitchBase(480, 4000, 480, 1000, 480, 2000, false);
  };
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override;

 protected:
    bool on_receive(remote_base::RemoteReceiveData data) override;
remote_base::RCSwitchBase protocol_;
};
}  // namespace otio
}  // namespace esphome