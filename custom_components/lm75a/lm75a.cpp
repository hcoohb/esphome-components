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
//   uint16_t real_result = INVALID_LM75A_TEMPERATURE;
//   uint16_t i2c_received = 0;

//   // Go to temperature data register
//   Wire.beginTransmission(LM75A_BASE_ADDRESS);
//   Wire.write(LM75A_REG_ADDR_TEMP);
//   if(Wire.endTransmission()) {
//     ESP_LOGD(TAG, "Transmission error.");
//     return;
//   }

//   // Get content
//   if (Wire.requestFrom(LM75A_BASE_ADDRESS, 2)) {
//     Wire.readBytes((uint8_t*)&i2c_received, 2);
//   } else {
//     ESP_LOGD(TAG, "Can't read temperature.");
//     return;
//   }

//   // Modify the value (only 11 MSB are relevant if swapped)
//   int16_t refactored_value;
//   uint8_t* ptr = (uint8_t*)&refactored_value;

//   // Swap bytes
//   *ptr = *((uint8_t*)&i2c_received + 1);
//   *(ptr + 1) = *(uint8_t*)&i2c_received;

//   // Shift data (left-aligned)
//   refactored_value >>= 5;

//   float real_value;
//   if (refactored_value & 0x0400) {
//     // When sign bit is set, set upper unused bits, then 2's complement
//     refactored_value |= 0xf800;
//     refactored_value = ~refactored_value + 1;
//     real_value = (float)refactored_value * (-1) * LM75A_DEGREES_RESOLUTION;
//   } else {
//     real_value = (float)refactored_value *  LM75A_DEGREES_RESOLUTION;
//   }

//   ESP_LOGD(TAG, "Got Temperature=%.1f°C", real_value);
//   publish_state(real_value);
} 
}
}