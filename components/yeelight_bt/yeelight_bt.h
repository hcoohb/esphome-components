#pragma once

#include "esphome/core/component.h"
#include "esphome/components/ble_client/ble_client.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/light/light_output.h"
#include "esphome/components/light/light_state.h"

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
static const uint8_t CMD_FLOW = 0x4A;
static const uint8_t CMD_GETSTATE = 0x44;
static const uint8_t RES_GETSTATE = 0x45;
static const uint8_t CMD_GETNAME = 0x52;
static const uint8_t RES_GETNAME = 0x53;
static const uint8_t CMD_GETVER = 0x5C;
static const uint8_t RES_GETVER = 0x5D;
static const uint8_t CMD_GETSERIAL = 0x5E;
static const uint8_t RES_GETSERIAL = 0x5F;
static const uint8_t RES_GETTIME = 0x62;
static const uint8_t MODE_COLOR = 0x01;
static const uint8_t MODE_WHITE = 0x02;
static const uint8_t MODE_FLOW = 0x03;

static const std::string MODEL_BEDSIDE = "Bedside";
static const std::string MODEL_CANDELA = "Candela";

struct YeeBTPacket {
  uint8_t length;
  uint8_t data[18];
};

class Yeelight_bt : public light::LightOutput, public esphome::ble_client::BLEClientNode, public Component {
 public:
  // Component methods
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  
  // BLEClientNode methods
  void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                           esp_ble_gattc_cb_param_t *param) override;
  
  // LightOutput methods
  light::LightTraits get_traits() override;
  void setup_state(light::LightState *state) override { this->light_state_ = state; }
  void write_state(light::LightState *state) override;

  // Custom methods
  void request_pair();
  void request_state();
  void turn_on();
  void turn_off();
  void set_brightness(float brightness);
  void set_color_temperature(float color_temperature, float brightness);
  void set_rgb(float red, float green, float blue, float brightness);
  void request_flow_state();
  void flow_mode();
  // void send_cmd(const char *data);
  void send_cmd(const std::string& hexStr);

 protected:
  espbt::ESPBTUUID yeebt_service_uuid_ = espbt::ESPBTUUID::from_raw(YEEBT_SERVICE_UUID);
  espbt::ESPBTUUID yeebt_cmd_chr_uuid_ = espbt::ESPBTUUID::from_raw(YEEBT_CMD_CHR_UUID);
  espbt::ESPBTUUID yeebt_status_chr_uuid_ = espbt::ESPBTUUID::from_raw(YEEBT_STATUS_CHR_UUID);
  uint16_t status_char_handle_;
  uint16_t cmd_char_handle_;
  bool paired_;
  void char_decode_(const uint8_t *data, uint16_t length);
  YeeBTPacket *char_encode_(uint8_t command, uint8_t *data, uint8_t length);
  YeeBTPacket *char_encode_(uint8_t *full_hex, uint8_t length);
  YeeBTPacket packet_;
  void char_send_(YeeBTPacket *packet);
  light::LightState *light_state_{nullptr};
  bool publish_notif_state_;

  float correct_mireds_from_HA(float ct);
  float correct_mireds_to_HA(float ct);

  // Yeelight status as state values:
  float state_;
  light::ColorMode mode_;
  float red_;
  float green_;
  float blue_;
  float color_temperature_;
  float brightness_;

};

/// yeelight_bt_flow effect.
// class Yeelight_BT_Flow : public LightEffect {
//  public:
//   explicit Yeelight_BT_Flow(const std::string &name) : LightEffect(name) {}

//   void apply() override {
//     const uint32_t now = millis();
//     if (now - this->last_color_change_ < this->update_interval_) {
//       return;
//     }
//     auto call = this->state_->turn_on();
//     float out = this->on_ ? 1.0 : 0.0;
//     call.set_brightness_if_supported(out);
//     this->on_ = !this->on_;
//     call.set_transition_length_if_supported(this->transition_length_);
//     // don't tell HA every change
//     call.set_publish(false);
//     call.set_save(false);
//     call.perform();

//     this->last_color_change_ = now;
//   }

//   void set_transition_length(uint32_t transition_length) { this->transition_length_ = transition_length; }

//   void set_update_interval(uint32_t update_interval) { this->update_interval_ = update_interval; }

//  protected:
//   bool on_ = false;
//   uint32_t last_color_change_{0};
//   uint32_t transition_length_{};
//   uint32_t update_interval_{};
// };

}  // namespace yeelight_bt
}  // namespace esphome

#endif