#include "daikin176.h"
#include "esphome/components/remote_base/remote_base.h"
#include "esphome/core/log.h"

namespace esphome {
namespace daikin176 {

static const char *const TAG = "daikin176.climate";

climate::ClimateTraits Daikin176Climate::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(this->sensor_ != nullptr);
  traits.set_supported_modes({climate::CLIMATE_MODE_OFF});
  // traits.set_supported_modes({climate::CLIMATE_MODE_OFF, climate::CLIMATE_MODE_HEAT_COOL});
  if (this->supports_cool_)
    traits.add_supported_mode(climate::CLIMATE_MODE_COOL);
  if (this->supports_heat_)
    traits.add_supported_mode(climate::CLIMATE_MODE_HEAT);
  if (this->supports_dry_)
    traits.add_supported_mode(climate::CLIMATE_MODE_DRY);
  if (this->supports_fan_only_)
    traits.add_supported_mode(climate::CLIMATE_MODE_FAN_ONLY);

  traits.set_supports_two_point_target_temperature(false);
  traits.set_visual_min_temperature(this->minimum_temperature_);
  traits.set_visual_max_temperature(this->maximum_temperature_);
  traits.set_visual_temperature_step(this->temperature_step_);
  traits.set_supported_fan_modes(this->fan_modes_);
  traits.set_supported_swing_modes(this->swing_modes_);
  traits.set_supported_presets(this->presets_);
  return traits;
}


void Daikin176Climate::transmit_state() {
  uint8_t remote_state[22] = {0x11, 0xDA, 0x17, 0x18, 0x04, 0x00, 0x1E, 0x11, 0xDA, 0x17,
                              0x18, 0x00, 0x73, 0x00, 0x20, 0x00, 0x00, 0x20, 0x16, 0x00,
                              0x20, 0x00};

  remote_state[12] = this->operation_mode_alt_();
  remote_state[14] = this->operation_mode_();
  //13: modeButton: only if the mode is changed
  remote_state[13] = ((remote_state[14] & 0xF0) == this->prev_mode_) ? 0x00 : 0x04;
  this->prev_mode_ = (remote_state[14] & 0xF0);
  //18: fan_speed and swing
  remote_state[18] = this->fan_speed_();
  remote_state[17] = this->temperature_();

  //6: checksum of bytes before: if ID unchanged: 1E
  // remote_state[6] = 0;
  // for (int i = 0; i < 6; i++) {
  //   remote_state[6] += remote_state[i];
  // }

  //21: checksum
  for (int i = 7; i < 21; i++) {
    remote_state[21] += remote_state[i];
  }

  auto transmit = this->transmitter_->transmit();
  auto data = transmit.get_data();
  data->set_carrier_frequency(DAIKIN_IR_FREQUENCY);

  for (uint16_t repeat = 0; repeat <= 0; repeat++) {  // repeat whole message

    //transmit first part of message (7bytes)
    data->mark(DAIKIN_HEADER_MARK);
    data->space(DAIKIN_HEADER_SPACE);
    for (int i = 0; i < 7; i++) {
      // ESP_LOGD(TAG, "transmit byte%d is %x", i, remote_state[i]);
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
      // ESP_LOGD(TAG, "transmit byte%d is %x", i, remote_state[i]);
      for (uint8_t mask = 1; mask > 0; mask <<= 1) {  // iterate through bit mask
        data->mark(DAIKIN_BIT_MARK);
        bool bit = remote_state[i] & mask;
        data->space(bit ? DAIKIN_ONE_SPACE : DAIKIN_ZERO_SPACE);
      }
    }
    data->mark(DAIKIN_BIT_MARK);
    data->space(DAIKIN_MESSAGE_SPACE);
  }

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
    case climate::CLIMATE_MODE_HEAT_COOL:
      operating_mode |= DAIKIN_MODE_HEAT_COOL;
      break;
    case climate::CLIMATE_MODE_FAN_ONLY:
      operating_mode |= DAIKIN_MODE_FAN;
      break;
    case climate::CLIMATE_MODE_OFF:
    default:
      operating_mode = this->prev_mode_ & 0xF0;
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
    case climate::CLIMATE_MODE_HEAT_COOL:
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
  switch (this->fan_mode.value()) {
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
//     case climate::CLIMATE_MODE_HEAT_COOL:
//       return 0xc0;
    default:
      uint8_t temperature = (uint8_t) roundf(clamp<float>(this->target_temperature, DAIKIN_TEMP_MIN, DAIKIN_TEMP_MAX)) - 9;
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
      case DAIKIN_MODE_HEAT_COOL:
        this->mode = climate::CLIMATE_MODE_HEAT_COOL;
        break;
      case DAIKIN_MODE_FAN:
        this->mode = climate::CLIMATE_MODE_FAN_ONLY;
        break;
    }
  } else {
    this->mode = climate::CLIMATE_MODE_OFF;
  }
  this->prev_mode_ = (mode & 0xF0);
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
  /* Validate header */
  /* because of protocol, if remote_receiver.idle >35ms in config,
  we will be also capturing the first frame, so we move to find the second
  frame where the data is */
  if (data.size()>= 300) {
    data.advance(6*8*2); //we move at least 6bytes (minus headers)
  }
  // we search for the headers in the next 24  bits
  uint8_t i = 0;
  while (i<24 && !data.expect_item(DAIKIN_HEADER_MARK, DAIKIN_HEADER_SPACE)){
    data.advance(2);
    i++;
  }
  if (i==24) return false;
  ESP_LOGD(TAG, "Found headers at i=%d", i);

  /* we read all bits of 2nd frame to bytes LSB first */
  uint8_t state_frame[DAIKIN_STATE_FRAME_SIZE] = {};
  for (uint8_t pos = 0; pos < DAIKIN_STATE_FRAME_SIZE; pos++) {
    uint8_t byte = 0;
    for (int8_t bit = 0; bit < 8; bit++) {
      if (data.expect_item(DAIKIN_BIT_MARK, DAIKIN_ONE_SPACE))
        byte |= 1 << bit;
      else if (!data.expect_item(DAIKIN_BIT_MARK, DAIKIN_ZERO_SPACE)) {
        return false;
      }
    }
    state_frame[pos] = byte;
  }
    
  /* check protocol frame constants */
  if (state_frame[0] != 0x11 || 
      state_frame[1] != 0xDA || 
      state_frame[2] != 0x17 || 
      state_frame[3] != 0x18 || 
      state_frame[4] != 0x00)
      return false;  
  ESP_LOGD(
      TAG,
      "Received: %02X %02X %02X %02X %02X   %02X %02X %02X %02X %02X   %02X %02X %02X %02X %02X",
      state_frame[0], state_frame[1], state_frame[2], state_frame[3], state_frame[4], state_frame[5],
      state_frame[6], state_frame[7], state_frame[8], state_frame[9], state_frame[10], state_frame[11],
      state_frame[12], state_frame[13], state_frame[14]);
  
  return this->parse_state_frame_(state_frame);
}

}  // namespace daikin176
}  // namespace esphome
