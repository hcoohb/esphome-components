#include "yeelight_bt.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

#ifdef USE_ESP32

namespace esphome {
namespace yeelight_bt {

static const char *const TAG = "yeelight_bt";


void Yeelight_bt::dump_config() {
  // ESP_LOGCONFIG(TAG, "Yeelight_BT");
  ESP_LOGCONFIG(TAG, "Yeelight_BT: '%s'", this->light_state_ ? this->light_state_->get_name().c_str() : "");
  // LOG_SENSOR(" ", "Battery", this->battery_);
  // LOG_SENSOR(" ", "Illuminance", this->illuminance_);
}

void Yeelight_bt::setup() {
  this->paired_ = false;
  this->publish_notif_state_ = true;
}

void Yeelight_bt::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
  switch (event) {
    case ESP_GATTC_OPEN_EVT: {
      ESP_LOGI(TAG, "Connected successfully!");
      break;
    }
    case ESP_GATTC_DISCONNECT_EVT: {
      ESP_LOGW(TAG, "Disconnected!");
      this->node_state = espbt::ClientState::IDLE;
      break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT: {
      auto *status_chr = this->parent_->get_characteristic(this->yeebt_service_uuid_, this->yeebt_status_chr_uuid_);
      if (status_chr == nullptr) {
          ESP_LOGE(TAG, "[%s] No status characterisctic found on device, not a Yeelight BT..?",
                   this->parent_->address_str().c_str());
        break;
      }
      ESP_LOGI(TAG, "Status service has been found!");
      this->status_char_handle_ = status_chr->handle;
      ESP_LOGI(TAG, "Handle is %x", this->status_char_handle_);

      // Register for notification
      auto status_notify =
          esp_ble_gattc_register_for_notify(this->parent()->gattc_if, this->parent()->remote_bda, this->status_char_handle_);
      if (status_notify) {
        ESP_LOGW(TAG, "esp_ble_gattc_register_for_notify failed, status=%d", status_notify);
      }

      // Get CMD handle
      auto *cmd_chr = this->parent_->get_characteristic(this->yeebt_service_uuid_, this->yeebt_cmd_chr_uuid_);
      if (cmd_chr == nullptr) {
          ESP_LOGE(TAG, "[%s] No CMD characterisctic found on device, not a Yeelight BT..?",
                   this->parent_->address_str().c_str());
        break;
      }
      this->cmd_char_handle_ = cmd_chr->handle;

      // Try to read
      // auto status_read =
      //   esp_ble_gattc_read_char(this->parent()->gattc_if, this->parent()->conn_id, this->status_char_handle_, ESP_GATT_AUTH_REQ_NONE);
      // if (status_read) {
      //   this->status_set_warning();
      //   ESP_LOGW(TAG, "[%s] Error sending read request , status=%d", this->parent_->address_str().c_str(), status_read);
      // }

      break;
    }
    case ESP_GATTC_READ_CHAR_EVT: {
      ESP_LOGI(TAG, "ESP_GATTC_READ_CHAR_EVT");
      if (param->read.conn_id != this->parent()->conn_id)
        break;
      if (param->read.status != ESP_GATT_OK) {
        ESP_LOGW(TAG, "Error reading char at handle %d, status=%d", param->read.handle, param->read.status);
        break;
      }
      if (param->read.handle == this->status_char_handle_) {
        this->status_clear_warning();
        ESP_LOGI(TAG, "Received: %x", *param->read.value);
        // this->publish_state(this->parse_data_(param->read.value, param->read.value_len));
      }
      break;
    }
    case ESP_GATTC_WRITE_CHAR_EVT: {
      if (param->write.status != ESP_GATT_OK) {
        ESP_LOGW(TAG, "Error writing char at handle %d, status=%d", param->write.handle, param->write.status);
      }
      break;
    }
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
      ESP_LOGI(TAG, "ESP_GATTC_REG_FOR_NOTIFY_EVT");
      if (param->reg_for_notify.status != ESP_GATT_OK){
          ESP_LOGE(TAG, "reg for notify failed, error status = %x", param->reg_for_notify.status);
          break;
      }
      ESP_LOGI(TAG, "ESP_GATTC_REG_FOR_NOTIFY_EVT with handle: %d", param->reg_for_notify.handle);
      // Write in the descriptor to receive the notifications
      uint16_t notify_en = 1;
      esp_ble_gattc_write_char_descr (
        this->parent_->gattc_if,
        this->parent_->conn_id,
        param->reg_for_notify.handle+1,
        sizeof(notify_en),
        (uint8_t *)&notify_en,
        ESP_GATT_WRITE_TYPE_RSP,
        ESP_GATT_AUTH_REQ_NONE);

      this->node_state = espbt::ClientState::ESTABLISHED;

      // Sending a pair command
      this->request_pair();
      break;
    }
    case ESP_GATTC_NOTIFY_EVT: {
      ESP_LOGI(TAG, "ESP_GATTC_NOTIFY_EVT");
      if (param->notify.handle != this->status_char_handle_)
        break;
      ESP_LOGI(TAG, "ESP_GATTC_NOTIFY_EVT correct");
      ESP_LOGV(TAG, "ESP_GATTC_NOTIFY_EVT: handle=0x%x, received %d bytes", param->notify.handle, param->notify.value_len);
      ESP_LOGV(TAG, "DEC(%d): 0x%s", param->notify.value_len, format_hex_pretty(param->notify.value, param->notify.value_len).c_str());
      this->char_decode_(param->notify.value, param->notify.value_len);
      // this->decoder_->decode(param->notify.value, param->notify.value_len);

      // if (this->battery_ != nullptr && this->decoder_->has_battery_level() &&
      //     millis() - this->last_battery_update_ > 10000) {
      //   this->battery_->publish_state(this->decoder_->battery_level_);
      //   this->last_battery_update_ = millis();
      // }

      // if (this->illuminance_ != nullptr && this->decoder_->has_light_level()) {
      //   this->illuminance_->publish_state(this->decoder_->light_level_);
      // }

      // if (this->current_sensor_ > 0) {
      //   if (this->illuminance_ != nullptr) {
      //     auto *packet = this->encoder_->get_light_level_request();
      //     auto status = esp_ble_gattc_write_char(this->parent_->gattc_if, this->parent_->conn_id, this->status_char_handle_,
      //                                            packet->length, packet->data, ESP_GATT_WRITE_TYPE_NO_RSP,
      //                                            ESP_GATT_AUTH_REQ_NONE);
      //     if (status) {
      //       ESP_LOGW(TAG, "[%s] esp_ble_gattc_write_char failed, status=%d", this->parent_->address_str().c_str(),
      //                status);
      //     }
      //   }
      //   this->current_sensor_ = 0;
      // }
      break;
    }
    default:
      break;
  }
}

void Yeelight_bt::char_decode_(const uint8_t *data, uint16_t length){
  ESP_LOGV(TAG, "DEC(%d): 0x%s", length, format_hex_pretty(data, length).c_str());
  

  if (length != 18 || data[0] !=COMMAND_STX) {
    ESP_LOGV(TAG, "Unknown command");
    return;
  }

  switch (data[1]) {
    case RES_PAIR: {
      ESP_LOGV(TAG, "Pair Response");
      switch (data[2]) {
        case 0x01: {
          // Light is in pairing mode. => Push the small button on the lamp
          this->paired_ = false;
          ESP_LOGW(TAG, "Light is in pairing mode. => Push the small button on the lamp.");
          break;
        }
        case 0x02: {
          // Light has successfully paired
          this->paired_ = true;
          ESP_LOGV(TAG, "Light has successfully paired");
          this->request_state();
          break;
        }
        case 0x03: {
          // Light is not paired...
          this->paired_ = false;
          ESP_LOGW(TAG, "Light is not paired. Restart the lamp to start the pairing process");
          break;
        }
        case 0x04: {
          // Light is already paired
          ESP_LOGV(TAG, "Light is already paired");
          this->paired_ = true;
          this->request_state();
          break;
        }
        default:
          ESP_LOGW(TAG, "Unknown pairing mode. Best to reset the lamp to start the pairing process");
          break;
      }
      break;
    }
    case RES_GETSTATE: {
      ESP_LOGV(TAG, "State Response");
      if(!this->publish_notif_state_) break;
      auto call = this->light_state_->make_call();

      // read state
      bool state = (data[2] == CMD_POWER_ON);
      this->state_ = (float) state;
      ESP_LOGV(TAG, "Light is %s", state ? "ON" : "OFF");
      call.set_state(this->state_);

      // read lamp mode
      switch (data[3]) {
        case MODE_FLOW: {
          ESP_LOGV(TAG, "Light is in FLOW mode");
          this->mode_ = light::ColorMode::BRIGHTNESS;
          break;
        }
        case MODE_WHITE: {
          ESP_LOGV(TAG, "Light is in WHITE mode");
          uint16_t color_temp = data[10] | (data[9] << 8);
          ESP_LOGV(TAG, "Temperature is %d K [1700-6500]", color_temp);
          this->color_temperature_ = this->correct_mireds_to_HA(1000000.0f / (float) color_temp);
          call.set_color_temperature(this->color_temperature_);
          this->mode_ = light::ColorMode::COLOR_TEMPERATURE;
          break;
        }
        case MODE_COLOR: {
          ESP_LOGV(TAG, "Light is in COLOR mode");
          uint8_t red = data[4];
          uint8_t green = data[5];
          uint8_t blue = data[6];
          ESP_LOGV(TAG, "R,G,B is %d, %d, %d [0-255]", red, green, blue);
          this->red_ = (float) red / 255.0f;
          call.set_red(this->red_);
          this->green_ = (float) green / 255.0f;
          call.set_green(this->green_);
          this->blue_ = (float) blue / 255.0f;
          call.set_blue(this->blue_);
          this->mode_ = light::ColorMode::RGB;
          break;
        }
        default:
          break;
      }
      call.set_color_mode(this->mode_);

      // read brightness
      uint8_t brightness = data[8];
      ESP_LOGV(TAG, "Brightness is %d [0-100]", brightness);
      this->brightness_ = (float) brightness / 100.0f;
      call.set_brightness(this->brightness_);

      // push infos - will trigger a call to write_state but we check there if anything needs to be sent
      call.perform();
      break;
    }
    case RES_GETVER: {
      ESP_LOGV(TAG, "Version Response");
      break;
    }
    case RES_GETSERIAL: {
      ESP_LOGV(TAG, "Serial Response");
      break;
    }
    default:
      break;
  }
}

YeeBTPacket *Yeelight_bt::char_encode_(uint8_t command, uint8_t *data, uint8_t length) {
  this->packet_.length = 18;
  memset(this->packet_.data, 0, this->packet_.length); //reset packet
  this->packet_.data[0] = COMMAND_STX;
  this->packet_.data[1] = command;
  memcpy(&this->packet_.data[2], data, length);
  ESP_LOGV(TAG, "ENC(%d): 0x%s", this->packet_.length, format_hex_pretty(this->packet_.data, this->packet_.length).c_str());
  return &this->packet_;
}

YeeBTPacket *Yeelight_bt::char_encode_(uint8_t *full_hex, uint8_t length)  {
  return this->char_encode_(full_hex[0], &full_hex[1], length-1);
}

void Yeelight_bt::request_pair() {
  uint8_t data = CMD_PAIR_ON;
  this->char_send_(this->char_encode_(CMD_PAIR, &data, 1));
  // It will send a pair notif
}

void Yeelight_bt::request_state() {
  uint8_t data = 0x00;
  this->char_send_(this->char_encode_(CMD_GETSTATE, &data, 0));
  // It will send a state notif
}

void Yeelight_bt::request_flow_state() {
  uint8_t data = 0x02;
  this->char_send_(this->char_encode_(0x4c, &data, 1));
  // It will send a state notif
}

void Yeelight_bt::turn_on() {
  uint8_t data = CMD_POWER_ON;
  this->char_send_(this->char_encode_(CMD_POWER, &data, 1));
  // It will send a state notif
}

void Yeelight_bt::turn_off() {
  uint8_t data = CMD_POWER_OFF;
  this->char_send_(this->char_encode_(CMD_POWER, &data, 1));
  // It will send a state notif
}

void Yeelight_bt::set_brightness(float brightness) {
  // brightness given [0-1], needed [0-100]
  uint8_t data = (uint8_t) (brightness*100);
  this->char_send_(this->char_encode_(CMD_BRIGHTNESS, &data, 1));
  // It will NOT send a state notif, so schedule it:
  this->set_timeout("req_state", 1000, [this]() { this->request_state(); });
  this->set_timeout("req_flow", 3000, [this]() { this->flow_mode(); });
}

float Yeelight_bt::correct_mireds_from_HA(float ct){
  float min_mired = (1000000.0f / 6500.0f);
  float max_mired = (1000000.0f / 1700.0f);
  float white_mired_ui = 366;
  float white_mired_real = 235;

  if (ct < white_mired_ui)
    return remap(ct, min_mired, white_mired_ui, min_mired, white_mired_real);
  else
      return remap(ct, white_mired_ui, max_mired, white_mired_real, max_mired);
}

float Yeelight_bt::correct_mireds_to_HA(float ct){
  float min_mired = (1000000.0f / 6500.0f);
  float max_mired = (1000000.0f / 1700.0f);
  float white_mired_ui = 366;
  float white_mired_real = 235;

  if (ct < white_mired_real)
    return remap(ct, min_mired, white_mired_real, min_mired, white_mired_ui);
  else
      return remap(ct, white_mired_real, max_mired, white_mired_ui, max_mired);
}

void Yeelight_bt::set_color_temperature(float color_temperature, float brightness){
  // color_temperature given in mireds, needed kelvin [1700-6500]
  // brightness given [0-1], needed [0-100]
  uint8_t data[3];
  uint16_t ct_k = (uint16_t) (1000000.0f / this->correct_mireds_from_HA(color_temperature));
  data[0] = (ct_k >> 8);
  data[1] = ct_k & 0x00FF;
  data[2] = (uint8_t) (brightness*100);
  this->char_send_(this->char_encode_(CMD_TEMP, data, 3));
  // It will NOT send a state notif, so schedule it:
  this->set_timeout("req_state", 1000, [this]() { this->request_state(); });
}

void Yeelight_bt::set_rgb(float red, float green, float blue, float brightness){
  // red, green, blue given [0-1], needed [1-255]
  // brightness given [0-1], needed [0-100]
  uint8_t data[5];
  data[0] = (uint8_t) (red*255);
  data[1] = (uint8_t) (green*255);
  data[2] = (uint8_t) (blue*255);
  data[3] = 0x01;
  data[4] = (uint8_t) (brightness*100);
  this->char_send_(this->char_encode_(CMD_RGB, data, 5));
  // It will NOT send a state notif, so schedule it:
  this->set_timeout("req_state", 1000, [this]() { this->request_state(); });
}


void Yeelight_bt::flow_mode() {
  uint8_t data = 0x01;
  this->char_send_(this->char_encode_(0x4c, &data, 1));
  // It will send a state notif
  this->set_timeout("req_flow2", 3000, [this]() { this->request_flow_state(); });
}

// void Yeelight_bt::send_cmd(const char *data){
//   ESP_LOGV(TAG, "In Send_CMD to be executed is: %s, of size %d", data, sizeof(data));
void Yeelight_bt::send_cmd(const std::string& hexStr){
  // convert to hex:
  // const char *c = hexStr.c_str();
  ESP_LOGV(TAG, "In Send_CMD to be executed is: %s, of size %d", hexStr.c_str(), hexStr.size());
  uint8_t data[17];
  parse_hex(hexStr, data, 17);
  // char b[3] = "00";
  // for (unsigned int i = 0; i < hexStr.size(); i += 2) {
  //   b[0] = *(txt + i);
  //   b[1] = *(txt + i + 1);
  //   *(buf + (i >> 1)) = strtoul(b, NULL, 16);
  // }
  ESP_LOGV(TAG, "In Send_CMD to be executed is: %s, of size %d", data, sizeof(data));
  // this->char_send_(this->char_encode_(CMD_RGB, data, 5));


}

void Yeelight_bt::char_send_(YeeBTPacket *packet){
  auto status = esp_ble_gattc_write_char(this->parent_->gattc_if, this->parent_->conn_id, this->cmd_char_handle_,
                                          packet->length, packet->data, ESP_GATT_WRITE_TYPE_NO_RSP,
                                          ESP_GATT_AUTH_REQ_NONE);
  if (status) {
    ESP_LOGW(TAG, "[%s] esp_ble_gattc_write_char failed, status=%d", this->parent_->address_str().c_str(),
              status);
  }
}


// Set the device's traits
light::LightTraits Yeelight_bt::get_traits() {
  auto traits = light::LightTraits();
  // traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS});
  traits.set_supported_color_modes({light::ColorMode::BRIGHTNESS , light::ColorMode::RGB, light::ColorMode::COLOR_TEMPERATURE});
  traits.set_min_mireds(1000000.0f / 6500.0f);
  traits.set_max_mireds(1000000.0f / 1700.0f);
  return traits;
}

void Yeelight_bt::write_state(light::LightState *state) {
  float status, red, green, blue, color_temperature, brightness;
  light::ColorMode mode;
  status = state->current_values.get_state();
  red = state->current_values.get_red();
  green = state->current_values.get_green();
  blue = state->current_values.get_blue();
  color_temperature = state->current_values.get_color_temperature();
  brightness = state->current_values.get_brightness();
  mode = state->current_values.get_color_mode();
  bool just_turned_on = false;
    // state->current_values_as_rgbct(&red, &green, &blue, &color_temperature, &white_brightness);

  //Did anything actually changed?
  ESP_LOGD(TAG, "SHould we write anything to the light??");
  if(this->state_ != status) {
    ESP_LOGD(TAG, "State changed %f -> %f", state_, status);
    if (status==0) {
      this->turn_off();
      return;
    }
    else {
      this->turn_on();
      just_turned_on = true;
    }
  }
  if(status==0){
    // we do no other request until state changed...
    return;
  }
  if(this->mode_ != mode) ESP_LOGD(TAG, "Mode changed %d -> %d", (uint8_t) mode_, (uint8_t) mode);
  if((mode == light::ColorMode::RGB) && ((this->red_ != red) || (this->green_ != green) || (this->blue_ != blue))) {
    ESP_LOGD(TAG, "RGB changed %f,%f,%f -> %f,%f,%f", red_, green_, blue_, red, green, blue);
    this->set_rgb(red, green, blue, brightness);
    return;
  }
  if((mode == light::ColorMode::COLOR_TEMPERATURE) && (this->color_temperature_ != color_temperature)){
    ESP_LOGD(TAG, "Color Temp changed %f -> %f", color_temperature_, color_temperature);
    set_color_temperature(color_temperature, brightness);
    return;
  } 
  if(this->brightness_ != brightness) {
    ESP_LOGD(TAG, "Brightness changed %f -> %f", brightness_, brightness);
    // brightness at the end as it may already been changed by setting color_temp or color
    if(just_turned_on){
      // Let's schedule it otherwise if we change it now, it will stop the transition to turn on
      this->publish_notif_state_ = false; // disable pushing state just yet as we need to modif brightness
      this->set_timeout("set_brightness", 1200, [this, brightness]() { this->set_brightness(brightness); this->publish_notif_state_ = true;});
      ESP_LOGD(TAG, "Brightness scheduled");
    } else {
      this->set_brightness(brightness);
    }
    return;
  }

  // Fill our variables with the device's current state
  ESP_LOGD(TAG, "current_values_as_rgbct() returns %f, %f, %f, %f, %f", red, green, blue, color_temperature, brightness);
  ESP_LOGD(TAG, "current_values wb and cb returns %f, %f", brightness, state->current_values.get_color_brightness());
  ESP_LOGD(TAG, "current_values state and mode returns %f, %d", status, (uint8_t) mode);

}


}  // namespace yeelight_bt
}  // namespace esphome

#endif