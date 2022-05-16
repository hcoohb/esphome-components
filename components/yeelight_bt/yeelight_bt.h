#pragma once

#include "esphome/core/component.h"
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/sensor/sensor.h"
// #include "esphome/components/yeelight_bt/yeelight_bt_base.h"

#ifdef USE_ESP32

#include <esp_gattc_api.h>

namespace esphome {
namespace yeelight_bt {

namespace espbt = esphome::esp32_ble_tracker;

static const std::string YEEBT_SERVICE_UUID = "8E2F0CBD-1A66-4B53-ACE6-B494E25F87BD";
static const std::string YEEBT_CMD_CHR_UUID = "AA7D3F34-2D4F-41E0-807F-52FBF8CF7443";
static const std::string YEEBT_STATUS_CHR_UUID = "8F65073D-9F57-4AAA-AFEA-397D19D5BBEB";

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

struct YeeBTPacket {
  uint8_t length;
  uint8_t data[18];
};

class Yeelight_bt : public esphome::ble_client::BLEClientNode, public PollingComponent {
 public:
  void setup() override;
  void update() override;
  void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                           esp_ble_gattc_cb_param_t *param) override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_battery(sensor::Sensor *battery) { battery_ = battery; }
  void set_illuminance(sensor::Sensor *illuminance) { illuminance_ = illuminance; }

 protected:
  espbt::ESPBTUUID yeebt_service_uuid_ = espbt::ESPBTUUID::from_raw(YEEBT_SERVICE_UUID);
  espbt::ESPBTUUID yeebt_cmd_chr_uuid_ = espbt::ESPBTUUID::from_raw(YEEBT_CMD_CHR_UUID);
  espbt::ESPBTUUID yeebt_status_chr_uuid_ = espbt::ESPBTUUID::from_raw(YEEBT_STATUS_CHR_UUID);
  uint16_t status_char_handle_;
  uint16_t cmd_char_handle_;
  bool paired_;
  void decode_(const uint8_t *data, uint16_t length);

  bool logged_in_;
  sensor::Sensor *battery_{nullptr};
  sensor::Sensor *illuminance_{nullptr};
  uint8_t current_sensor_;
  // The AM43 often gets into a state where it spams loads of battery update
  // notifications. Here we will limit to no more than every 10s.
  uint8_t last_battery_update_;
};

}  // namespace yeelight_bt
}  // namespace esphome

#endif