#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/sensor/filter.h"
#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/remote_base/rc_switch_protocol.h"

namespace esphome {
namespace otio {

class OtioSensor;
class OtioLowBatteryBinarySensor;

class OtioComponent : public Component, public remote_base::RemoteReceiverListener {
  public:
    OtioComponent(){
      protocol_ = remote_base::RCSwitchBase(480, 4000, 480, 1000, 480, 2000, false);
    };
    
    OtioSensor *get_temp_sensor_by_channel(uint8_t channel);
    OtioSensor *get_hum_sensor_by_channel(uint8_t channel);
    OtioLowBatteryBinarySensor *get_binary_sensor_by_channel(uint8_t channel);
    void setup() override;
    void dump_config() override;
    float get_setup_priority() const override { return setup_priority::DATA; }

  protected:
    // friend OtioSensor;
    std::vector<OtioSensor *> sensors_temp_;
    std::vector<OtioSensor *> sensors_hum_;
    std::vector<OtioLowBatteryBinarySensor *> binary_sensors_;
    remote_base::RCSwitchBase protocol_;
    bool on_receive(remote_base::RemoteReceiveData data) override;
};



// Internal class that helps us create multiple sensors for one Otio hub.
class OtioSensor : public sensor::Sensor {
 public:
  OtioSensor(uint8_t channel, OtioComponent *parent);
  uint8_t get_channel() const;
  void set_filters(const std::vector<sensor::Filter *> &filters);

 protected:
  OtioComponent *parent_;
  uint8_t channel_;
};

class OtioLowBatteryBinarySensor : public binary_sensor::BinarySensor {
  public:
    OtioLowBatteryBinarySensor(uint8_t channel, OtioComponent *parent);
    uint8_t get_channel() const;
    
  protected:
    OtioComponent *parent_;
    uint8_t channel_;
};


    
}  // namespace otio
}  // namespace esphome