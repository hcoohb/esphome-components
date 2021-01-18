#include "esphome.h"
#include <SPI.h>
#include <nRF905.h>
#include <nRF905_defs.h>
#include <FastCRC.h>

// https://github.com/glmnet/esphome_devices/blob/master/my_home/arduino_port_expander.h

#define BASE_STATION_ADDR	0x33339CC3 //0x63336669 //0x633396c9

#define PAYLOAD_TX_SIZE		20
#define PAYLOAD_SIZE		22 // NRF905_MAX_PAYLOAD
#define LED					A5
#define AM NRF905_PIN_UNUSED //34 //, //27, //NRF905_PIN_UNUSED
#define DR 27 //34,//26, //

#define PACKET_NONE		0
#define PACKET_OK		1
#define PACKET_INVALID	2


#define get_aquamonitor(constructor) static_cast<AquaMonitor *> \
  (const_cast<custom_component::CustomComponentConstructor *>(&constructor)->get_component(0))

#define aquaMonitor_sensor1(am) get_aquamonitor(am)->get_sensor1()
// #define aquaMonitor_switch_power(am) get_aquamonitor(am)->get_switch_power()


FastCRC16 CRC16;
static const char *TAG = "AquaMonitor";
static volatile uint8_t packetStatus; // NOTE: In interrupt mode this must be volatile
static volatile uint8_t drUP; // NOTE: In interrupt mode this must be volatile
static volatile uint8_t amUP; // NOTE: In interrupt mode this must be volatile
static volatile uint8_t txDone;
nRF905 transceiver = nRF905();
static void ICACHE_RAM_ATTR HOT nRF905_int_am(){amUP = 1;transceiver.interrupt_am();} //ESP_LOGD(TAG, "AM" );
static void ICACHE_RAM_ATTR HOT nRF905_int_dr(){drUP = 1;transceiver.interrupt_dr();} //void ICACHE_RAM_ATTR HOT ????


using namespace esphome;

void nRF905_onRxComplete(nRF905* device)
    {
        packetStatus = PACKET_OK;
    }
void nRF905_onRxInvalid(nRF905* device)
{
	packetStatus = PACKET_INVALID;
}
void nRF905_onTxComplete(nRF905* device)
{
  txDone=1;
}

class nrf905SwitchPower : public Component, public Switch {
  public:
    void setup() override {}
    void write_state(bool state) override {
      // This will be called every time the user requests a state change.
      if (state)
        transceiver.powerDown();
      else 
        transceiver.standby();
      publish_state(state); // Acknowledge new state by publishing it
    }
};

class AquaMonitor : public PollingComponent {
 public:

    nrf905SwitchPower *switchPower {nullptr};
    Sensor *sensor1;
    Sensor *sensor2;
    // constructor
    AquaMonitor() : PollingComponent(500) {
      // this->powerSwitch = new nrf905PowerSwitch();
    }
    uint32_t txidx=0;
  
  sensor::Sensor *get_sensor1(){
    sensor1 = new Sensor();
    return sensor1;
  }

  switch_::Switch *get_switch_power(){
    switchPower = new nrf905SwitchPower();
    return switchPower;
  }
  
  // void setPowerSwitch(nrf905PowerSwitch *theSwitch) {
  //   this->powerSwitch = theSwitch;
  //   // this->powerSwitch->add_on_state_callback(powerStitchCB);
  //   // this->powerSwitch->add_on_state_callback([this](bool state) {
  //   //   if (state) {
  //   //     transceiver.powerDown();
  //   //   }
  //   // });
  // }

  // void powerStitchCB (bool state){
  //   if (state)
  //       transceiver.powerDown();
  //     else 
  //       transceiver.standby();
  //     publish_state(state); // Acknowledge new state by publishing it
  // }

  // void loop() override {
  //   transceiver.poll();
  // }

  void setup() override {
  drUP=0;
  amUP=0;
  txDone=0;
  packetStatus = PACKET_NONE;
    // This will be called by App.setup()
    ESP_LOGCONFIG(TAG, "Setting up Sensor...");
    // SPI.pins(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
    // SPI.begin();
    SPI.begin();
    // Minimal wires (interrupt mode)
	transceiver.begin(
		SPI,
		10000000, // SPI Clock speed (10MHz)
		5, // SPI SS
		25, //NRF905_PIN_UNUSED, // CE (standby) pin must be connected to VCC (3.3V)
		33, //NRF905_PIN_UNUSED, // TRX (RX/TX mode) pin must be connected to GND (force RX mode)
		16, //NRF905_PIN_UNUSED, // PWR (power-down) pin must be connected to VCC (3.3V)
		NRF905_PIN_UNUSED, // Without the CD pin collision avoidance will be disabled
		DR, // DR: Data Ready
		AM, //AM: Address Match Without the AM pin the library the library must poll the status register over SPI.
		nRF905_int_dr,
		nRF905_int_am //NULL //nRF905_int_am // No interrupt function
    );
    transceiver.events(
		nRF905_onRxComplete,
		nRF905_onRxInvalid,
		nRF905_onTxComplete,
		NULL
    );
    uint8_t regs[NRF905_REGISTER_COUNT];
    transceiver.getConfigRegisters(regs);
    ESP_LOGCONFIG(TAG, "Registers: %02X %02x %02X %02x %02X %02x %02X %02x %02X %02x ", 
    regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7], regs[8], regs[9]);
    // Set device 
    transceiver.setBand(NRF905_BAND_433);
    transceiver.setChannel(109); //109:433.3
    transceiver.setCRC(NRF905_CRC_DISABLE ); //NRF905_CRC_16 ); 
    transceiver.setAddressSize(4, 4);  //tx, rx
    transceiver.setListenAddress(BASE_STATION_ADDR);
    transceiver.setPayloadSize(PAYLOAD_TX_SIZE, PAYLOAD_SIZE); //tx, rx
    transceiver.setAutoRetransmit(true);
    transceiver.getConfigRegisters(regs);
    
    transceiver.RX();
    this->switchPower->publish_state(true);
  
  ESP_LOGCONFIG(TAG, "Registers: %02X %02x %02X %02x %02X %02x %02X %02x %02X %02x ", 
  regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7], regs[8], regs[9]);

  txidx=0;
  }

  void dump_config() {
  ESP_LOGCONFIG(TAG, "Config:");
  ESP_LOGCONFIG(TAG, "Mode: %i", transceiver.mode());
  uint8_t regs[NRF905_REGISTER_COUNT];
  transceiver.getConfigRegisters(regs);
  ESP_LOGCONFIG(TAG, "Registers: %02X %02x %02X %02x %02X %02x %02X %02x %02X %02x ", 
  regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7], regs[8], regs[9]);

  
	char* str;
	byte data;
	uint16_t channel = ((uint16_t)(regs[1] & 0x01)<<8) | regs[0];
	uint32_t freq = (422400UL + (channel * 100UL)) * (1 + ((regs[1] & ~NRF905_MASK_BAND) >> 1));

	ESP_LOGCONFIG(TAG, "Channel: %hu", channel);
	ESP_LOGCONFIG(TAG, "Freq: %i KHz", freq);
	ESP_LOGCONFIG(TAG, "Auto retransmit: %s", (regs[1] & ~NRF905_MASK_AUTO_RETRAN) ? "Enabled" : "Disabled");
	ESP_LOGCONFIG(TAG, "Low power RX: %s",(regs[1] & ~NRF905_MASK_LOW_RX) ? "Enabled" : "Disabled");
	// TX power
	data = regs[1] & ~NRF905_MASK_PWR;
	switch(data)
	{
		case NRF905_PWR_n10:
			data = -10;
			break;
		case NRF905_PWR_n2:
			data = -2;
			break;
		case NRF905_PWR_6:
			data = 6;
			break;
		case NRF905_PWR_10:
			data = 10;
			break;
		default:
			data = -127;
			break;
	}
	ESP_LOGCONFIG(TAG, "TX Power: %d dBm", (signed char)data);

	// Freq band
	data = regs[1] & ~NRF905_MASK_BAND;
	switch(data)
	{
		case NRF905_BAND_433:
			str = (char*)"433";
			break;
		default:
			str = (char*)"868/915";
			break;
	}
	ESP_LOGCONFIG(TAG, "Band: %s MHz", str);
	ESP_LOGCONFIG(TAG, "TX Address width: %i", regs[2] >> 4);
	ESP_LOGCONFIG(TAG, "RX Address width: %i", regs[2] & 0x07);
	ESP_LOGCONFIG(TAG, "RX Payload size: %i", regs[3]);
	ESP_LOGCONFIG(TAG, "TX Payload size: %i", regs[4]);
  ESP_LOGCONFIG(TAG, "RX Address [0] [1] [2] [3]: %02x %02x %02x %02x ", regs[5], regs[6], regs[7], regs[8]);
	ESP_LOGCONFIG(TAG, "RX Address:  %08lx", ((unsigned long)regs[8]<<24 | (unsigned long)regs[7]<<16 | (unsigned long)regs[6]<<8 | (unsigned long)regs[5]));

	// CRC mode
	data = regs[9] & ~NRF905_MASK_CRC;
	switch(data)
	{
		case NRF905_CRC_16:
			str = (char*)"16 bit";
			break;
		case NRF905_CRC_8:
			str = (char*)"8 bit";
			break;
		default:
			str = (char*)"Disabled";
			break;
	}
	ESP_LOGCONFIG(TAG, "CRC Mode: %s", str);
// Xtal freq
	data = regs[9] & ~NRF905_MASK_CLK;
	switch(data)
	{
		case NRF905_CLK_4MHZ:
			data = 4;
			break;
		case NRF905_CLK_8MHZ:
			data = 8;
			break;
		case NRF905_CLK_12MHZ:
			data = 12;
			break;
		case NRF905_CLK_16MHZ:
			data = 16;
			break;
		case NRF905_CLK_20MHZ:
			data = 20;
			break;
		default:
			data = 0;
			break;
	}
	ESP_LOGCONFIG(TAG, "Xtal freq: %i MHz", data);

	// Clock out freq
	data = regs[9] & ~NRF905_MASK_OUTCLK;
	switch(data)
	{
		case NRF905_OUTCLK_4MHZ:
			str = (char*)"4 MHz";
			break;
		case NRF905_OUTCLK_2MHZ:
			str = (char*)"2 MHz";
			break;
		case NRF905_OUTCLK_1MHZ:
			str = (char*)"1 MHz";
			break;
		case NRF905_OUTCLK_500KHZ:
			str = (char*)"500 KHz";
			break;
		default:
			str = (char*)"Disabled";
			break;
	}
	ESP_LOGCONFIG(TAG, "Clock out freq: %s", str);
	

  // LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with %s failed!", TAG);
  }
  LOG_UPDATE_INTERVAL(this);

  LOG_SENSOR("  ", TAG, sensor1);
}

  void update() override {
    // This will be called by App.loop()
    uint8_t success;
    uint8_t drUPbuf;
    uint8_t amUPbuf;
    success = packetStatus;
    drUPbuf = drUP;
    drUP = 0;
    if (drUPbuf==1){
      ESP_LOGD(TAG, "drUp was 1");
    }
    amUPbuf = amUP;
    amUP = 0;
    if (amUPbuf==1){
      ESP_LOGD(TAG, "amUp was 1");
    }
    if (txDone==1){
      ESP_LOGD(TAG, "tX finished was 1");
      txDone=0;
    }
    
    packetStatus=PACKET_NONE;
    if (txidx % 200 == 0){ //output every 10sec
      ESP_LOGD(TAG, "dr is %i", digitalRead(DR));
      ESP_LOGD(TAG, "am is %i", digitalRead(AM));
      }

    // if (txidx % 240 == 0){ //output every 120sec
    //   transceiver.standby();
    //   delay(10);
    //   if(txidx % (240*3) ==0){
    //     transceiver.setCRC(NRF905_CRC_16);
    //     ESP_LOGD(TAG, "Setting CRC to 16");
    //   }
    //   else if(txidx % (240*2) ==0){
    //     transceiver.setCRC(NRF905_CRC_8);
    //     ESP_LOGD(TAG, "Setting CRC to 8");
    //   }
    //   else{
    //     transceiver.setCRC(NRF905_CRC_DISABLE);
    //     ESP_LOGD(TAG, "Setting CRC to disabled");
    //   }
    //   transceiver.RX();
    // }
    
    if (transceiver.mode()!=NRF905_MODE_RX){
      ESP_LOGD(TAG, "Not in Mode RX" );
      transceiver.RX();
      this->switchPower->publish_state(true);
    }

    if (txidx % 80000 == 0){
      ESP_LOGD(TAG, "On publie?");
      uint8_t buffer[PAYLOAD_TX_SIZE] = {0x08, 0x01, 0xe1, 0x0a, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 ,0x00, 0x55, 0x55, 0x55};
	    // memset(buffer, 6, PAYLOAD_TX_SIZE); //repeat numbr 6, xxx times
      transceiver.standby();
      transceiver.write(0x33339cc3, buffer, sizeof(buffer));
      while(!transceiver.TX(NRF905_NEXTMODE_TX, true));
      delay(1000);
      while(!transceiver.TX(NRF905_NEXTMODE_STANDBY , true));  // NRF905_NEXTMODE_RX
      ESP_LOGD(TAG, "Published good");
      // Get the reply data
      // uint8_t replyBuffer[PAYLOAD_TX_SIZE];
      // transceiver.read(replyBuffer, sizeof(replyBuffer));
      // txidx=0;
    }
    txidx+=1;
    
    
    if(success == PACKET_INVALID){
        ESP_LOGD("nRF905", "Invalid package received");
        uint8_t replyBuffer[PAYLOAD_SIZE];
        transceiver.read(replyBuffer, sizeof(replyBuffer));
        ESP_LOGD(
            "nRF905",
            "Received Invalid:" // %02X %02X %02X %02X %02X   %02X %02X %02X %02X %02X   %02X %02X %02X %02X %02X",
            // replyBuffer[0], replyBuffer[1], replyBuffer[2], replyBuffer[3], replyBuffer[4], replyBuffer[5]//,
            // replyBuffer[6], replyBuffer[7], replyBuffer[8], replyBuffer[9], replyBuffer[10], replyBuffer[11],
            // replyBuffer[12], replyBuffer[13], replyBuffer[14]
        );
        for (uint8_t i=0;i<PAYLOAD_SIZE;i++){
          ESP_LOGD("nRF905", "B[%i] %02x", i, replyBuffer[i]);
        }
        transceiver.RX();
        this->switchPower->publish_state(true);
    }

    if(success == PACKET_OK){
        ESP_LOGD("nRF905", "Valid package received");
        // Get the reply data
        uint8_t replyBuffer[PAYLOAD_SIZE];
        transceiver.read(replyBuffer, sizeof(replyBuffer));
        if (true){ //replyBuffer[0]==0x33 || replyBuffer[0]==0x66){
          ESP_LOGD(
            "nRF905",
            "Received: %02X %02X %02X %02X %02X   %02X %02X %02X %02X %02X   %02X %02X %02X %02X %02X   %02X %02X %02X %02X %02X   %02X %02X",
            replyBuffer[0], replyBuffer[1], replyBuffer[2], replyBuffer[3], replyBuffer[4],
            replyBuffer[5], replyBuffer[6], replyBuffer[7], replyBuffer[8], replyBuffer[9],
            replyBuffer[10], replyBuffer[11], replyBuffer[12], replyBuffer[13], replyBuffer[14],
            replyBuffer[15], replyBuffer[16], replyBuffer[17], replyBuffer[18], replyBuffer[19],
            replyBuffer[20], replyBuffer[21]
          );
          if(replyBuffer[1]==1 && replyBuffer[6]==0x08){
            float val = ( (uint16_t)replyBuffer[2] | (uint16_t)replyBuffer[3]<<8) / 4.76f;
            ESP_LOGD("nRF905", "W: dec=%d val=%f", (uint16_t)replyBuffer[2] | (uint16_t)replyBuffer[3]<<8, val);
            ESP_LOGD("nRF905", "CRC: %04x", CRC16.kermit(replyBuffer, 20));
            sensor1->publish_state(val);
            // ESP_LOGD("nRF905", "kW: %f", ( );
          }
          // for (uint8_t i=0;i<PAYLOAD_SIZE;i++){
          //   ESP_LOGD("nRF905", "B[%i] %02x", i, replyBuffer[i]);
          // }
          // packetStatus = PACKET_NONE;
        }
        
    }

  }
  float get_setup_priority() const override { return esphome::setup_priority::DATA; };



};
// volatile uint8_t AquaMonitorSensor::packetStatus; 





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