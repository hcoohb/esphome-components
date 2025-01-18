#include "lucci_air.h"
#include "esphome/core/log.h"

namespace esphome {
namespace remote_base {

static const char *const TAG = "remote.lucciair";

/**
 * Example LucciAir Ceiling fan RF packet (24 data bits each with 2 pulses, 48 pulses in total):
 *   0100001100110110         |   11100101
 *   address-16bits (fan ID)  |   command-8bits
 *
 * command seems to have the two first bits as 1

Commands
11 100101 = 229: light
11 111111 = 255: on-off
11 111001 = 249: 1
11 111010 = 250: 2
11 111011 = 251: 3
11 111100 = 252: 4
11 111101 = 253: 5
11 111110 = 254: 6
11 110001 = 241: reverse
11 000101 = 197: cycle

address:
0100001100110110 = 17206: living
1001011101011110 = 38750: study
1100000010010110 = 49302: bedroom
1100010010001001 = 50313: guest

transmission seems to be:
                      ______            ______      ____________
_____________________|      |__________|      |____|            |

|10500                450   |700        450   |325  825         |
|Sync                       |Zero             |One              |

Equivalent to:
RCSwitchBase(10500, 450, 700, 450, 325, 825, true)

 */


void LucciAirProtocol::encode(RemoteTransmitData *dst, const LucciAirData &data) {
  // combine address and data to recreate the code:
  uint64_t code;
  code = data.address;
  code = (code << 8) | data.data;
  // ESP_LOGD(TAG, "encoded LUCCIAIR %u", code);
  this->protocol_.transmit(dst, code, 24);
  
}

optional<LucciAirData> LucciAirProtocol::decode(RemoteReceiveData src) {
  LucciAirData out{
      .address = 0,
      .data = 0
  };

  uint64_t decoded_code;
  uint8_t decoded_nbits;
  if (!this->protocol_.decode(src, &decoded_code, &decoded_nbits))
      return {};
  if (decoded_nbits!=24)
      return {};
  out.data = decoded_code & 0xFF;
  out.address = (decoded_code >> 8) & 0xFFFF;

  // char buffer[25];
  // for (uint8_t j = 0; j < decoded_nbits; j++)
  //     buffer[j] = (decoded_code & ((uint64_t) 1 << (decoded_nbits - j - 1))) ? '1' : '0';
  // buffer[decoded_nbits] = '\0';
  // ESP_LOGD(TAG, "Received LUCCIAIR %u bits data='%s'", decoded_nbits, buffer);

  // ESP_LOGD(TAG, "Received LUCCIAIR %u, address=%u data=%i", decoded_code, out.address, out.data);
  return out;
}

void LucciAirProtocol::dump(const LucciAirData &data) {
  ESP_LOGI(TAG, "Received LucciAir: address=%u, data=%i",  data.address, data.data);
}

}  // namespace remote_base
}  // namespace esphome