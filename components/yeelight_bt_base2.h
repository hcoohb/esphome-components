#pragma once

#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
// #include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"

namespace esphome {
namespace yeelight_bt {

// namespace espbt = esphome::esp32_ble_tracker;

static const uint16_t AM43_SERVICE_UUID = 0xFE50;
static const uint16_t AM43_CHARACTERISTIC_UUID = 0xFE51;
//
// Tuya identifiers, only to detect and warn users as they are incompatible.
static const uint16_t AM43_TUYA_SERVICE_UUID = 0x1910;
static const uint16_t AM43_TUYA_CHARACTERISTIC_UUID = 0x2b11;

const std::string YEEBT_SERVICE_UUID = "8E2F0CBD-1A66-4B53-ACE6-B494E25F87BD";
const std::string YEEBT_CMD_CHR_UUID = "AA7D3F34-2D4F-41E0-807F-52FBF8CF7443";
const std::string YEEBT_STATUS_CHR_UUID = "8F65073D-9F57-4AAA-AFEA-397D19D5BBEB";

struct YeeBTPacket {
  uint8_t length;
  uint8_t data[18];
};

static const uint8_t COMMAND_STX = 0x43;
static const uint8_t CMD_PAIR = 0x67;
static const uint8_t CMD_PAIR_ON = 0x02;
static const uint8_t RES_PAIR = 0x63;
static const uint8_t CMD_POWER = 0x40;
static const uint8_t CMD_POWER_ON = 0x01;
static const uint8_t CMD_POWER_OFF = 0x02;
static const uint8_t CMD_COLOR = 0x41;
static const uint8_t CMD_BRIGHTNESS = 0x42;
static const uint8_t CMD_TEMP = 0x43;
static const uint8_t CMD_RGB = 0x41;
static const uint8_t CMD_GETSTATE = 0x44;
static const uint8_t CMD_GETSTATE_SEC = 0x02;
static const uint8_t RES_GETSTATE = 0x45;
static const uint8_t CMD_GETNAME = 0x52;
static const uint8_t RES_GETNAME = 0x53;
static const uint8_t CMD_GETVER = 0x5C;
static const uint8_t RES_GETVER = 0x5D;
static const uint8_t CMD_GETSERIAL = 0x5E;
static const uint8_t RES_GETSERIAL = 0x5F;
static const uint8_t RES_GETTIME = 0x62;

static const std::string MODEL_BEDSIDE = "Bedside";
static const std::string MODEL_CANDELA = "Candela";

struct Am43Packet {
  uint8_t length;
  uint8_t data[24];
};

static const uint8_t CMD_GET_BATTERY_LEVEL = 0xA2;
static const uint8_t CMD_GET_LIGHT_LEVEL = 0xAA;
static const uint8_t CMD_GET_POSITION = 0xA7;
static const uint8_t CMD_SEND_PIN = 0x17;
static const uint8_t CMD_SET_STATE = 0x0A;
static const uint8_t CMD_SET_POSITION = 0x0D;
static const uint8_t CMD_NOTIFY_POSITION = 0xA1;

static const uint8_t RESPONSE_ACK = 0x5A;
static const uint8_t RESPONSE_NACK = 0xA5;

class Yeelight_btEncoder {
 public:
  Am43Packet *get_battery_level_request();
  Am43Packet *get_light_level_request();
  Am43Packet *get_position_request();
  Am43Packet *get_send_pin_request(uint16_t pin);
  Am43Packet *get_open_request();
  Am43Packet *get_close_request();
  Am43Packet *get_stop_request();
  Am43Packet *get_set_position_request(uint8_t position);

 protected:
  void checksum_();
  Am43Packet *encode_(uint8_t command, uint8_t *data, uint8_t length);
  Am43Packet packet_;
};

class Yeelight_btDecoder {
 public:
  void decode(const uint8_t *data, uint16_t length);
  bool has_state_response() { return this->has_state_response_; }
  bool has_pair_response() { return this->has_pair_response_; }
  bool has_version_response() { return this->has_version_response_; }
  bool has_serial_response() { return this->has_serial_response_; }

  bool has_battery_level() { return this->has_battery_level_; }
  bool has_light_level() { return this->has_light_level_; }
  bool has_set_position_response() { return this->has_set_position_response_; }
  bool has_set_state_response() { return this->has_set_state_response_; }
  bool has_position() { return this->has_position_; }
  bool has_pin_response() { return this->has_pin_response_; }

  union {
    uint8_t position_;
    uint8_t battery_level_;
    float light_level_;
    uint8_t set_position_ok_;
    uint8_t set_state_ok_;
    uint8_t pin_ok_;
  };

 protected:
  bool has_state_response_;
  bool has_pair_response_;
  bool has_version_response_;
  bool has_serial_response_;

  bool has_battery_level_;
  bool has_light_level_;
  bool has_set_position_response_;
  bool has_set_state_response_;
  bool has_position_;
  bool has_pin_response_;
};

}  // namespace yeelight_bt
}  // namespace esphome