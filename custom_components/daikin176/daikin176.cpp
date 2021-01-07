#include "daikin176.h"
#include "esphome/components/remote_base/remote_base.h"
// #include "esphome/core/log.h"

namespace esphome {
namespace daikin176 {

static const char *TAG = "daikin176.climate";

void Daikin176Climate::transmit_state() {
  uint8_t remote_state[22] = {0x11, 0xDA, 0x17, 0x18, 0x04, 0x00, 0x1E, 0x11, 0xDA, 0x17,
                              0x18, 0x00, 0x73, 0x04, 0x20, 0x0B, 0x00, 0x00, 0x16, 0x00,
                              0x20, 0x00};

  remote_state[12] = this->operation_mode_alt_();
//   remote_state[13] = 0x04 only if the mode is changed
  remote_state[14] = this->operation_mode_();
  remote_state[18] = this->fan_speed_();
  remote_state[17] = this->temperature_();

  //6 checksum?
  //13: modeButton
  //18: fan_speed and swing
  //21: checksum
  // Calculate checksum
//   for (int i = 0; i < 6; i++) {
//     remote_state[6] += remote_state[i];
//   }
  for (int i = 7; i < 21; i++) {
    remote_state[21] += remote_state[i];
  }

  auto transmit = this->transmitter_->transmit();
  auto data = transmit.get_data();
  data->set_carrier_frequency(DAIKIN_IR_FREQUENCY);

  //transmit first part of message (7bytes)
  data->mark(DAIKIN_HEADER_MARK);
  data->space(DAIKIN_HEADER_SPACE);
  for (int i = 0; i < 7; i++) {
//     ESP_LOGD(TAG, "transmit byte%d is %x", i, remote_state[i]);
    for (uint8_t mask = 1; mask > 0; mask <<= 1) {  // iterate through bit mask
      data->mark(DAIKIN_BIT_MARK);
      bool bit = remote_state[i] & mask;
      data->space(bit ? DAIKIN_ONE_SPACE : DAIKIN_ZERO_SPACE);
    }
  }
  data->mark(DAIKIN_BIT_MARK);
  data->space(DAIKIN_MESSAGE_SPACE);

  // transmit second part of message
  data->mark(DAIKIN_HEADER_MARK);
  data->space(DAIKIN_HEADER_SPACE);

  for (int i = 7; i < 22; i++) {
//     ESP_LOGD(TAG, "transmit byte%d is %x", i, remote_state[i]);
    for (uint8_t mask = 1; mask > 0; mask <<= 1) {  // iterate through bit mask
      data->mark(DAIKIN_BIT_MARK);
      bool bit = remote_state[i] & mask;
      data->space(bit ? DAIKIN_ONE_SPACE : DAIKIN_ZERO_SPACE);
    }
  }
  data->mark(DAIKIN_BIT_MARK);
  data->space(DAIKIN_MESSAGE_SPACE);

  transmit.perform();
}

uint8_t Daikin176Climate::operation_mode_() {
  uint8_t operating_mode = DAIKIN_MODE_ON;
  switch (this->mode) {
    case climate::CLIMATE_MODE_COOL:
      operating_mode |= DAIKIN_MODE_COOL;
      break;
    case climate::CLIMATE_MODE_DRY:
      operating_mode |= DAIKIN_MODE_DRY;
      break;
    case climate::CLIMATE_MODE_HEAT:
      operating_mode |= DAIKIN_MODE_HEAT;
      break;
    case climate::CLIMATE_MODE_AUTO:
      operating_mode |= DAIKIN_MODE_AUTO;
      break;
    case climate::CLIMATE_MODE_FAN_ONLY:
      operating_mode |= DAIKIN_MODE_FAN;
      break;
    case climate::CLIMATE_MODE_OFF:
    default:
      operating_mode = DAIKIN_MODE_OFF;
      break;
  }

  return operating_mode;
}

uint8_t Daikin176Climate::operation_mode_alt_() {
  uint8_t mode_alt = 0x03;
  switch (this->mode) {
    case climate::CLIMATE_MODE_DRY:
      mode_alt |= 0x20;
      break;
    case climate::CLIMATE_MODE_FAN_ONLY:
      mode_alt |= 0x60;
      break;
    case climate::CLIMATE_MODE_COOL:
    case climate::CLIMATE_MODE_AUTO:
    case climate::CLIMATE_MODE_HEAT:
    case climate::CLIMATE_MODE_OFF:
    default:
      mode_alt |= 0x70;
      break;
  }
  return mode_alt;
}

uint8_t Daikin176Climate::fan_speed_() {
  uint8_t fan_speed;
  switch (this->fan_mode) {
    case climate::CLIMATE_FAN_LOW:
      fan_speed = DAIKIN_FAN_L;
      break;
    case climate::CLIMATE_FAN_HIGH:
    default:
      fan_speed = DAIKIN_FAN_H;
  }

  // first 4 bits for the swing
  return this->swing_mode == climate::CLIMATE_SWING_VERTICAL ? fan_speed | 0x5 : fan_speed | 0x6;
}

uint8_t Daikin176Climate::temperature_() {
  // Force special temperatures depending on the mode
  switch (this->mode) {
    case climate::CLIMATE_MODE_FAN_ONLY:
    case climate::CLIMATE_MODE_DRY:
      return 0x10;
//     case climate::CLIMATE_MODE_AUTO:
//       return 0xc0;
    default:
      uint8_t temperature = (uint8_t) roundf(clamp(this->target_temperature, DAIKIN_TEMP_MIN, DAIKIN_TEMP_MAX)) - 9;
      return temperature << 1;
  }
}

bool Daikin176Climate::parse_state_frame_(const uint8_t frame[]) {
//   ESP_LOGD(TAG, "Received remote signal, parsing");
  //position in the state frame
  uint8_t checksum = 0;
  for (int i = 0; i < (DAIKIN_STATE_FRAME_SIZE - 1); i++) {
    checksum += frame[i];
  }
  if (frame[DAIKIN_STATE_FRAME_SIZE - 1] != checksum)
    return false;
//   ESP_LOGD(TAG, "Checksum validated");
  uint8_t mode = frame[7];
  if (mode & DAIKIN_MODE_ON) {
    switch (mode & 0xF0) {
      case DAIKIN_MODE_COOL:
        this->mode = climate::CLIMATE_MODE_COOL;
        break;
      case DAIKIN_MODE_DRY:
        this->mode = climate::CLIMATE_MODE_DRY;
        break;
      case DAIKIN_MODE_HEAT:
        this->mode = climate::CLIMATE_MODE_HEAT;
        break;
      case DAIKIN_MODE_AUTO:
        this->mode = climate::CLIMATE_MODE_AUTO;
        break;
      case DAIKIN_MODE_FAN:
        this->mode = climate::CLIMATE_MODE_FAN_ONLY;
        break;
    }
  } else {
    this->mode = climate::CLIMATE_MODE_OFF;
  }
  uint8_t temperature = frame[10];
  if (!(temperature & 0xC0)) {
    this->target_temperature = (temperature >> 1) + 9;
  }
  uint8_t fan_mode = frame[11];
  if ((fan_mode & 0x0F) == 5)
    this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
  else
    this->swing_mode = climate::CLIMATE_SWING_OFF;
  switch (fan_mode & 0xF0) {
    case DAIKIN_FAN_L:
      this->fan_mode = climate::CLIMATE_FAN_LOW;
      break;
    case DAIKIN_FAN_H:
      this->fan_mode = climate::CLIMATE_FAN_HIGH;
      break;
  }
  this->publish_state();
  return true;
}

bool Daikin176Climate::on_receive(remote_base::RemoteReceiveData data) {
  uint8_t state_frame[DAIKIN_STATE_FRAME_SIZE] = {};
  if (!data.expect_item(DAIKIN_HEADER_MARK, DAIKIN_HEADER_SPACE)) {
    return false;
  }
  for (uint8_t pos = 0; pos < DAIKIN_STATE_FRAME_SIZE; pos++) {
    uint8_t byte = 0;
    for (int8_t bit = 0; bit < 8; bit++) {
      if (data.expect_item(DAIKIN_BIT_MARK, DAIKIN_ONE_SPACE))
        byte |= 1 << bit;
      else if (!data.expect_item(DAIKIN_BIT_MARK, DAIKIN_ZERO_SPACE)) {
        return false;
      }
    }
    
//     ESP_LOGD(TAG, "byte%d is %x", pos, byte);
    state_frame[pos] = byte;
    if (pos == 0) {
      // frame header
      if (byte != 0x11)
        return false;
    } else if (pos == 1) {
      // frame header
      if (byte != 0xDA)
        return false;
    } else if (pos == 2) {
      // frame header
      if (byte != 0x17)
        return false;
    } else if (pos == 3) {
      // frame header
      if (byte != 0x18)
        return false;
    } else if (pos == 4) {
      // frame type
      if (byte != 0x00)
        return false;
    }
  }
  return this->parse_state_frame_(state_frame);
}

}  // namespace daikin176
}  // namespace esphome
