#pragma once

#include "esphome/core/component.h"
#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/remote_base/rc_switch_protocol.h"

#include <cinttypes>

namespace esphome {
namespace remote_base {

struct LucciAirData {
  uint16_t address;
  uint8_t data;

  bool operator==(const LucciAirData &rhs) const {
    return address == rhs.address && data == rhs.data;
  }
};

class LucciAirProtocol : public RemoteProtocol<LucciAirData> {
 public:
  void encode(RemoteTransmitData *dst, const LucciAirData &data) override;
  optional<LucciAirData> decode(RemoteReceiveData src) override;
  void dump(const LucciAirData &data) override;
  remote_base::RCSwitchBase protocol_ = remote_base::RCSwitchBase(10500, 450, 700, 450, 325, 825, true);
};

DECLARE_REMOTE_PROTOCOL(LucciAir)

template<typename... Ts> class LucciAirAction : public RemoteTransmitterActionBase<Ts...> {
 public:
  TEMPLATABLE_VALUE(uint16_t, address)
  TEMPLATABLE_VALUE(uint8_t, data)

  void encode(RemoteTransmitData *dst, Ts... x) override {
    LucciAirData data{};
    data.address = this->address_.value(x...);
    data.data = this->data_.value(x...);
    LucciAirProtocol().encode(dst, data);
  }
};

}  // namespace remote_base
}  // namespace esphome