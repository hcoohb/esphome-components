#include "yeelight_bt.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

#ifdef USE_ESP32

namespace esphome {
namespace yeelight_bt {

static const char *const TAG = "yeelight_bt";

std::string pkt_to_hex(const uint8_t *data, uint16_t len) {
  char buf[64];
  memset(buf, 0, 64);
  for (int i = 0; i < len; i++)
    sprintf(&buf[i * 2], "%02x", data[i]);
  std::string ret = buf;
  return ret;
}


void Yeelight_bt::dump_config() {
  ESP_LOGCONFIG(TAG, "Yeelight_BT");
  LOG_SENSOR(" ", "Battery", this->battery_);
  LOG_SENSOR(" ", "Illuminance", this->illuminance_);
}

void Yeelight_bt::setup() {
  this->paired_ = false;

  this->logged_in_ = false;
  this->last_battery_update_ = 0;
  this->current_sensor_ = 0;
}

void Yeelight_bt::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
  switch (event) {
    case ESP_GATTC_OPEN_EVT: {
      this->logged_in_ = false;
      ESP_LOGI(TAG, "Connected successfully!");
      break;
    }
    case ESP_GATTC_DISCONNECT_EVT: {
      this->logged_in_ = false;
      ESP_LOGW(TAG, "Disconnected!");
      this->node_state = espbt::ClientState::IDLE;
      if (this->battery_ != nullptr)
        this->battery_->publish_state(NAN);
      if (this->illuminance_ != nullptr)
        this->illuminance_->publish_state(NAN);
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
      // auto *packet = this->encoder_->get_battery_level_request();
      unsigned char newVal[18] = {
            0x43, 0x67, 0x02, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00
          };
      auto status =
          esp_ble_gattc_write_char(this->parent_->gattc_if, this->parent_->conn_id, this->cmd_char_handle_, sizeof(newVal),
                                   newVal, ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
      if (status)
        ESP_LOGW(TAG, "[%s] esp_ble_gattc_write_char failed, status=%d", this->parent_->address_str().c_str(), status);

      this->update();
      break;
    }
    case ESP_GATTC_NOTIFY_EVT: {
      ESP_LOGI(TAG, "ESP_GATTC_NOTIFY_EVT");
      if (param->notify.handle != this->status_char_handle_)
        break;
      ESP_LOGI(TAG, "ESP_GATTC_NOTIFY_EVT correct");
      ESP_LOGV(TAG, "ESP_GATTC_NOTIFY_EVT: handle=0x%x, received %d bytes", param->notify.handle, param->notify.value_len);
      // ESP_LOGV(TAG, "DEC(%d): 0x%s", param->notify.value_len, pkt_to_hex(param->notify.value, param->notify.value_len).c_str());
      this->decode_(param->notify.value, param->notify.value_len);
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

void Yeelight_bt::decode_(const uint8_t *data, uint16_t length){
  ESP_LOGV(TAG, "DEC(%d): 0x%s", length, pkt_to_hex(data, length).c_str());

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

void Yeelight_bt::update() {
  if (this->node_state != espbt::ClientState::ESTABLISHED) {
    ESP_LOGW(TAG, "[%s] Cannot poll, not connected", this->parent_->address_str().c_str());
    return;
  }
  
//   if (this->current_sensor_ == 0) {
//     if (this->battery_ != nullptr) {
//       auto *packet = this->encoder_->get_battery_level_request();
//       auto status =
//           esp_ble_gattc_write_char(this->parent_->gattc_if, this->parent_->conn_id, this->status_char_handle_, packet->length,
//                                    packet->data, ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
//       if (status)
//         ESP_LOGW(TAG, "[%s] esp_ble_gattc_write_char failed, status=%d", this->parent_->address_str().c_str(), status);
//     }
//     this->current_sensor_++;
//   }
}

}  // namespace yeelight_bt
}  // namespace esphome

#endif