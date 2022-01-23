// Adapted from https://github.com/crankyoldgit/IRremoteESP8266/blob/master/SupportedProtocols.md

#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

namespace esphome {
namespace daikin176 {

// Values for Daikin ARC43XXX IR Controllers
// Temperature
const uint8_t DAIKIN_TEMP_MIN = 10;  // Celsius
const uint8_t DAIKIN_TEMP_MAX = 32;  // Celsius

// Modes
const uint8_t DAIKIN_MODE_HEAT_COOL = 0x30;
const uint8_t DAIKIN_MODE_COOL = 0x20;
const uint8_t DAIKIN_MODE_HEAT = 0x10;
const uint8_t DAIKIN_MODE_DRY = 0x70;
const uint8_t DAIKIN_MODE_FAN = 0x00;
// Mode byte is OR with on/off
const uint8_t DAIKIN_MODE_OFF = 0x00;
const uint8_t DAIKIN_MODE_ON = 0x01;

// Fan Speed
const uint8_t DAIKIN_FAN_L = 0x10;
const uint8_t DAIKIN_FAN_H = 0x30;

// IR Transmission
const uint32_t DAIKIN_IR_FREQUENCY = 38000;
const uint32_t DAIKIN_HEADER_MARK = 5070;
const uint32_t DAIKIN_HEADER_SPACE = 2140;
const uint32_t DAIKIN_BIT_MARK = 370;
const uint32_t DAIKIN_ONE_SPACE = 1780;
const uint32_t DAIKIN_ZERO_SPACE = 710;
const uint32_t DAIKIN_MESSAGE_SPACE = 29410;

// State Frame size
const uint8_t DAIKIN_STATE_FRAME_SIZE = 15; //22-7

class Daikin176Climate : public climate_ir::ClimateIR {
 public:
  Daikin176Climate()
      : climate_ir::ClimateIR(
            DAIKIN_TEMP_MIN, DAIKIN_TEMP_MAX, 1.0f, true, true,
            {climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_HIGH},
            {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL}) {}

 protected:
  uint8_t prev_mode_=DAIKIN_MODE_COOL;

  // Transmit via IR the state of this climate controller.
  void transmit_state() override;
  uint8_t operation_mode_();
  uint8_t operation_mode_alt_();
  uint8_t fan_speed_();
  uint8_t temperature_();
  // Handle received IR Buffer
  bool on_receive(remote_base::RemoteReceiveData data) override;
  bool parse_state_frame_(const uint8_t frame[]);
};

}  // namespace daikin176
}  // namespace esphome
