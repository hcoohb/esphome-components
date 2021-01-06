#include "lm75a.h"
#include "esphome/core/log.h"


#define INVALID_LM75A_TEMPERATURE 1000

namespace esphome {
namespace lm75a {

static const char *TAG = "lm75a";

// static const uint8_t LM75A_BASE_ADDRESS = 0x48;
static const float LM75A_DEGREES_RESOLUTION = 0.125;
static const uint8_t LM75A_REGISTER_TEMP = 0;
static const uint8_t LM75A_REGISTER_CONFIG = 1;
static const uint8_t LM75A_REGISTER_PRODID = 7;
     
void LM75AComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up LM75A...");
  //Check connection
  uint8_t oldValue;
  uint8_t newValue;
  if(!this->read_byte(LM75A_REGISTER_CONFIG, &oldValue)){
    this->mark_failed();
    return;
  }
  if(!this->write_byte(LM75A_REGISTER_CONFIG, 0x0f)){
      this->mark_failed();
      return;
  }
  if(!this->read_byte(LM75A_REGISTER_CONFIG, &newValue)){
    this->mark_failed();
    return;
  }
  if(!this->write_byte(LM75A_REGISTER_CONFIG, oldValue)){
      this->mark_failed();
      return;
  }

  if (newValue != 0x0F) {
    this->mark_failed();
    return;
  }
  uint8_t value;
  float serial;
  if (!this->read_byte(LM75A_REGISTER_PRODID, &value)){
    this->mark_failed();
    return;
  }
  serial = (value >> 4) + (value & 0x0F) / 10.0f;
  ESP_LOGV(TAG, "    Serial Number: 0x%f", serial);
}

void LM75AComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "LM75A:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with LM75A failed!");
  }
  LOG_UPDATE_INTERVAL(this);

  LOG_SENSOR("  ", "LM75A", this);
}
float LM75AComponent::get_setup_priority() const { return setup_priority::DATA; }
  
void LM75AComponent::update() {
  float real_value = INVALID_LM75A_TEMPERATURE;
  uint16_t i2c_received = 0;

    if(!this->read_byte_16(LM75A_REGISTER_TEMP, &i2c_received)){
        this->status_set_warning();
        return;
  }
  ESP_LOGD(TAG, "READ=%x", i2c_received);
  real_value = (float)i2c_received / 256.0f;

  ESP_LOGD(TAG, "Got Temperature=%.1f°C", real_value);
  publish_state(real_value);
  this->status_clear_warning();
}
}
}