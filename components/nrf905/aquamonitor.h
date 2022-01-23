#include "esphome.h"
#include <SPI.h>
#include <nRF905.h>
#include <nRF905_defs.h>
#include <string>

// inspiration from
// https://github.com/glmnet/esphome_devices/blob/master/my_home/arduino_port_expander.h

#define BASE_STATION_ADDR	0x33339CC3 //0x63336669 //0x633396c9
#define ELEC_ADDR 0x633396c9

#define PAYLOAD_TX_SIZE		22
#define PAYLOAD_SIZE		22 // NRF905_MAX_PAYLOAD
#define LED					A5
#define AM NRF905_PIN_UNUSED //34 //, //27, //NRF905_PIN_UNUSED
#define DR 27 //34,//26, //

#define PACKET_NONE		0
#define PACKET_OK		1
#define PACKET_INVALID	2


#define get_aquamonitor(constructor) static_cast<AquaMonitor *>(constructor.get_component(0))
#define am_sensor(am, type, addr, channel) get_aquamonitor(am)->get_sensor(type, addr, channel)



#define AMS_NOT_SYNCED 0
#define AMS_SYNCING_TX 1
#define AMS_SYNCING_RX 2
#define AMS_SYNCED 3
#define AMS_LISTENING 4


static const char *TAG = "AquaMonitor";
class AquaMonitor;

using namespace esphome;

class AMSensor : public sensor::Sensor
{
public:
  AMSensor(AquaMonitor *parent, uint8_t type, uint32_t addr, uint8_t channel)
  {
    this->parent_ = parent;
    this->type_ = type;
    this->addr_ = addr;
    this->channel_ = channel;
    this->synced_ = false;
    this->listening_ = false;
    this->status_ = AMS_NOT_SYNCED;
  }
  uint32_t get_addr() { return this->addr_; }
  uint16_t get_channel() { return this->channel_; }
  uint8_t get_status() {return this->status_;}

  bool sync();
  bool need_sync(){return this->status_ < AMS_SYNCED;}
  bool setup_listening();
  void process_data(uint8_t *replyBuffer, uint8_t size);

protected:
  AquaMonitor *parent_;
  uint8_t type_;
  uint32_t addr_;
  uint16_t channel_;
  bool synced_;
  uint32_t start_listen_time_;
  bool listening_;
  uint8_t status_;
  uint32_t sync_timer;
};


class AquaMonitor : public PollingComponent {
  public:

    AquaMonitor(uint32_t base_addr, uint8_t delaybetween_sensors) : PollingComponent(50){
      this->base_addr_ = base_addr;
      this->delaybetween_sensors_ = delaybetween_sensors;
    }
  
    sensor::Sensor *get_sensor(uint8_t type, uint32_t addr, uint16_t channel){
      AMSensor *amSensor = new AMSensor(this, type, addr, channel);
      sensors_.push_back(amSensor);
      return amSensor;
    }

    void setup() override;
    void dump_config();
    void update() override; 
    float get_setup_priority() const override { return esphome::setup_priority::DATA; };

  protected:
    std::vector<AMSensor *> sensors_;
    uint32_t base_addr_;
    uint8_t delaybetween_sensors_;
    AMSensor *cur_listen;

};


// volatile uint8_t AquaMonitorSensor::packetStatus; 


// - Display request (long 10s) sensor 1
// - sensor 1 answer
// -Display request (long) sensor 2
// - sensor 2 answer
// - data sensor 1 received 64s later than previous answer 0.5s
// - display ackn 0.8s
// - data sensor 2 received 64s later than previous answer
// - display ackn 0.8s

// Display addr: 33339cc3
// 6333993c water addr
// 633396c9 Elec addr



// / / NRF905 the registers configuration / /
// unsigned char idata RFConf [11] =
// {
//   0x00, / / the Configuration command / /
//   0x4c, / / CH_NO, configure the frequency range of 430MHZ
//   0x0C, / / output power 10db, not retransmission power saving as the normal mode
//   0x44, / / address width set to 4 bytes
//   0x20, 0x20, / / receiver sending valid data length of 32 bytes
//   0xCC, 0xCC, 0xCC, 0xCC, / / receive address
//   0x58, / / CRC charge Xu, 8-bit CRC, the external clock signal is not enabled, 16M crystal
// };


// req: 10s
// pause: 250-480ms
// sensor: 300ms
// 6s in between requests

// sensor signal: 160ms
// pause: 150-225ms ?
// disp ackn: 505ms


// Latest record
// 10s req - 225ms gap - 300ms
// 6s after last req: 10s req - 200ms gap - 300ms from sensor
// water: 48s after last
// elec 64s after first sync response
// water responded on first sync but then at 48s after.
// therefore disp re-synced at 117s after first water answer

//water registers on 433.2MHz
// elec registers on 433.3MHz

// Preamble: 01010101010110011001 (Manchester IEEE encoded) 1111110101 (bits)
// Address: 4 bytes
// Data: 20 bytes
// CRC: 2 bytes: CRC16-kermit on 20bytes of Data - bytes are reversed (MSB/LE); poly:0x1021 - init: 0x0000 - refIn: true	- refout: true - XorOut:	0x0000 (https://crccalc.com/)