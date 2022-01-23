// original code inspired from https://github.com/ayufan/esphome-components

#include "memory_sensor.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#ifdef USE_ARDUINO
#include <Esp.h>
#endif

#ifdef USE_ESP_IDF
#include <esp_heap_caps.h>
#include <esp_system.h>
#endif

namespace esphome {
namespace memory {

static const char *TAG = "memory.sensor";

void MemorySensor::update() {
  #ifdef USE_ARDUINO
    uint32_t free_heap = ESP.getFreeHeap();  // NOLINT(readability-static-accessed-through-instance)
  #elif defined(USE_ESP_IDF)
    uint32_t free_heap = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  #endif
  // uint32_t free_heap = ESP.getFreeHeap();
  // ESP_LOGD(TAG, "Free Heap Size: %u bytes", free_heap);
  this->publish_state(free_heap);
}
std::string MemorySensor::unique_id() { return get_mac_address() + "-memory"; }
float MemorySensor::get_setup_priority() const { return setup_priority::LATE; }
void MemorySensor::dump_config() { LOG_SENSOR("", "Memory Sensor", this); }

}  // namespace memory
}  // namespace esphome