#include "otio.h"
#include "esphome/core/log.h"

namespace esphome {
namespace otio {

static const char *TAG = "otio";

// OTIO::OTIO(){
// protocol_ = RCSwitchBase(480, 4000, 480, 1000, 480, 2000, false);
// }

void OTIO::setup() {
    ESP_LOGD(TAG, "Setting up OTIO sensor !!!" );
    // sensor->add_filters({
    //     ThrottleFilter()
    //     LambdaFilter([&](float value) -> optional<float> { return 42/value; }),
    //     OffsetFilter(1),
    //     SlidingWindowMovingAverageFilter(15, 15), // average over last 15 values
    // });
    this->publish_state(NAN);
}
void OTIO::dump_config() { LOG_SENSOR("", "OTIO Sensor", this) }
float OTIO::get_setup_priority() const { return setup_priority::DATA; }

bool OTIO::on_receive(remote_base::RemoteReceiveData data) {
    
    /*
    Data received through PPM
    36bits of data repeated 20 times. stored in uint64t
    
    The data is grouped in 9 nibbles:
    [id0] [id1] [flags] [temp0] [temp1] [temp2] [const] [const] [const]
    - The 8-bit id can change when the battery is disconnected from the sensor.
    - flags are 4 bits B 0 C C, where B is the battery status: 1=OK, 0=LOW
        and CC is the channel: 0=CH1, 1=CH2, 2=CH3
    - temp is 12 bit signed scaled by 10
    - const is always 1111 (0x0F)
    - Last 8 bits is 0 on the otio (some sensors expose humidity here)
    
    */
    
    uint64_t decoded_code;
  uint8_t decoded_nbits;
  if (!this->protocol_.decode(data, &decoded_code, &decoded_nbits))
    return false;
  if (decoded_nbits!=36)
    return false;
  
    char buffer[65];
      for (uint8_t j = 0; j < decoded_nbits; j++)
        buffer[j] = (decoded_code & ((uint64_t) 1 << (decoded_nbits - j - 1))) ? '1' : '0';

      buffer[decoded_nbits] = '\0';
      ESP_LOGD(TAG, "Received OTIO %u bits data='%s'", decoded_nbits, buffer);
      
      
    //   uint8_t *b = (uint8_t *)&decoded_code;
    //   ESP_LOGD(TAG, "OTIO b array %x, %x, %x, %x, %x, %x, %x, %x", b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7]);
      
      decoded_code >>= 12;
      uint64_t mask = 0b111111111111;
      auto temp = float((float)(decoded_code & 0x0FFF) / 10.0);
      ESP_LOGD(TAG, "OTIO Temp %f", temp);
      decoded_code >>= 12;
      uint8_t channel = (decoded_code & 0x03)+1;
      bool battery_low = (decoded_code & 0x08) == 0;
      uint8_t id = decoded_code >>= 4;
      ESP_LOGD(TAG, "Received OTIO battery low: %d, channel %d, ID %d", battery_low, channel, id);
      
    // ESP_LOGD(TAG, "Got data %s",data.get_raw_data());
    

// void NTC::process_(float value) {
//   if (isnan(value)) {
//     this->publish_state(NAN);
//     return;
//   }

//   double lr = log(double(value));
//   double v = this->a_ + this->b_ * lr + this->c_ * lr * lr * lr;
//   auto temp = float(1.0 / v - 273.15);

//   ESP_LOGD(TAG, "'%s' - Temperature: %.1f°C", this->name_.c_str(), temp);
  this->publish_state(temp);
  return true;
}

}  // namespace otio
}  // namespace esphome