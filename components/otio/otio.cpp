#include "otio.h"
#include "esphome/core/log.h"

namespace esphome {
namespace otio {

static const char *TAG = "otio.sensor";

void OtioComponent::setup() {
    ESP_LOGD(TAG, "Setting up OtioComponent" );
    // sensor->add_filters({
    //     ThrottleFilter()
    //     LambdaFilter([&](float value) -> optional<float> { return 42/value; }),
    //     OffsetFilter(1),
    //     SlidingWindowMovingAverageFilter(15, 15), // average over last 15 values
    // });
    for (auto sensor : this->sensors_temp_) {
        sensor->publish_state(NAN);
    }
    for (auto sensor : this->sensors_hum_) {
        sensor->publish_state(NAN);
    }
}
void OtioComponent::dump_config() {
    ESP_LOGCONFIG(TAG, "OtioComponent:");
    for (auto *sensor : this->sensors_temp_) {
        LOG_SENSOR("  ", "Sensor Temp", sensor);
        ESP_LOGCONFIG(TAG, "    Channel %i", sensor->get_channel());
    }
    for (auto *sensor : this->sensors_hum_) {
        LOG_SENSOR("  ", "Sensor Hum", sensor);
        ESP_LOGCONFIG(TAG, "    Channel %i", sensor->get_channel());
    }
    for (auto *sensor : this->binary_sensors_) {
        LOG_BINARY_SENSOR("  ", "Binary Sensor", sensor);
        ESP_LOGCONFIG(TAG, "    Channel %i", sensor->get_channel());
    }
}

bool OtioComponent::on_receive(remote_base::RemoteReceiveData data) {
    
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
      
    uint8_t hum = (uint8_t)(decoded_code & 0xFF);
    decoded_code >>= 12;
    int temp_raw = (int16_t)((decoded_code & 0x0FFF) << 4); // sign-extend
    float temp =(temp_raw >> 4) * 0.1f;
    // auto temp = float((float)(decoded_code & 0x0FFF) / 10.0);
    decoded_code >>= 12;
    uint8_t channel = (decoded_code & 0x03)+1;
    bool battery_low = (decoded_code & 0x08) == 0;
    uint8_t id = decoded_code >>= 4;
    ESP_LOGD(TAG, "Received OTIO temp %fC, humidity %d%, battery low: %d, channel %d, ID %d", temp, hum, battery_low, channel, id);
      
    for (auto *sensor : this->sensors_temp_) {
      if(sensor->get_channel() == channel){
          sensor->publish_state(temp);
      }
    }  
    for (auto *sensor : this->sensors_hum_) {
      if(sensor->get_channel() == channel){
          sensor->publish_state(hum);
      }
    }  
    for (auto *sensor : this->binary_sensors_) {
      if(sensor->get_channel() == channel){
          sensor->publish_state(battery_low);
      }
    }
    return true;
}

OtioSensor *OtioComponent::get_temp_sensor_by_channel(uint8_t channel) {
    auto s = new OtioSensor(channel, this);
    this->sensors_temp_.push_back(s);
    return s;
}
OtioSensor *OtioComponent::get_hum_sensor_by_channel(uint8_t channel) {
    auto s = new OtioSensor(channel, this);
    this->sensors_hum_.push_back(s);
    return s;
}
OtioLowBatteryBinarySensor *OtioComponent::get_binary_sensor_by_channel(uint8_t channel) {
    auto s = new OtioLowBatteryBinarySensor(channel, this);
    this->binary_sensors_.push_back(s);
    return s;
}


OtioSensor::OtioSensor(uint8_t channel, OtioComponent *parent)
    : parent_(parent){
    this->channel_ = channel;
    this->set_filters({});
}
void OtioSensor::set_filters(const std::vector<sensor::Filter *> &filters) {
    this->clear_filters();
    this->add_filter(new sensor::ThrottleFilter(2000)); //Add extra filter to only publish once per packet
    this->add_filters(filters);
}
uint8_t OtioSensor::get_channel() const { return this->channel_; }


OtioLowBatteryBinarySensor::OtioLowBatteryBinarySensor(uint8_t channel, OtioComponent *parent)
    : parent_(parent){
    this->channel_ = channel;
}
uint8_t OtioLowBatteryBinarySensor::get_channel() const { return this->channel_; }

}  // namespace otio
}  // namespace esphome