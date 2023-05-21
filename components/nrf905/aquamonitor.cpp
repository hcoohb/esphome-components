#include "esphome.h"
#include "aquamonitor.h"
#include <FastCRC.h>


FastCRC16 CRC16;

static volatile uint8_t packetStatus; // NOTE: In interrupt mode this must be volatile
static volatile uint8_t drUP; // NOTE: In interrupt mode this must be volatile
static volatile uint8_t amUP; // NOTE: In interrupt mode this must be volatile
static volatile uint8_t txDone;
nRF905 transceiver = nRF905(SPI);
static void ICACHE_RAM_ATTR HOT nRF905_int_am(){amUP = 1;transceiver.interrupt_am();} //ESP_LOGD(TAG, "AM" );
static void ICACHE_RAM_ATTR HOT nRF905_int_dr(){drUP = 1;transceiver.interrupt_dr();} //void ICACHE_RAM_ATTR HOT ????

// To convert to string for textsensor:
constexpr char hexmap[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                           '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

std::string hexStr(unsigned char *data, int len)
{
  std::string s(len * 2, ' ');
  for (int i = 0; i < len; ++i) {
    s[2 * i]     = hexmap[(data[i] & 0xF0) >> 4];
    s[2 * i + 1] = hexmap[data[i] & 0x0F];
  }
  return s;
}


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


bool AMSensor::sync(){
  if(this->status_==AMS_NOT_SYNCED){
    ESP_LOGD(TAG, "Syncing sensor xx - starting TX");
    // Requests from Display:
    // 6333993c 01 c39c3333 0000000000000000000000000000002845 water 
    // 633396c9 01 c39c3333 000100000000000000000000000000c23b elec
    uint8_t buffer[PAYLOAD_TX_SIZE] = {0x01, 0xc3, 0x9c, 0x33, 0x33, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc2, 0x3b};
    transceiver.standby();
    ESP_LOGD(TAG, "Setting channel %i", this->channel_);
    transceiver.setChannel(this->channel_); 
    transceiver.write(this->addr_, buffer, sizeof(buffer));
    while(!transceiver.TX(NRF905_NEXTMODE_TX, true));
    this->sync_timer = esphome::millis();
    this->status_ = AMS_SYNCING_TX;
    return false;
  }

  if((this->status_==AMS_SYNCING_TX) && (esphome::millis() - this->sync_timer <= 10000))
    return false; // we are still transmitting
  if(this->status_==AMS_SYNCING_TX){
    while(!transceiver.TX(NRF905_NEXTMODE_RX , true));
    this->status_ = AMS_SYNCING_RX;
    ESP_LOGD(TAG, "Syncing sensor xx - starting RX");
    return true; // we are done transmitting, now listen
  }
  //Now waiting for a reply
  if(esphome::millis() - this->sync_timer <= 15000){
    return true;
  }
  // replied timed out...
  ESP_LOGD(TAG, "Syncing sensor xx - fininshed RX: TIMED OUT");
  this->status_ = AMS_NOT_SYNCED;
  return false;
}

bool AMSensor::setup_listening(){
  if(this->status_ < AMS_SYNCED) return false;
  if((esphome::millis() - this->start_listen_time_>42000) && this->status_==AMS_SYNCED){
    // start listening for this sensor
    ESP_LOGD("nRF905", "Setting up listening for sensor");
    transceiver.setChannel(this->channel_);
    transceiver.RX();
    this->status_ = AMS_LISTENING;
    return true;
  }
  if ((esphome::millis() - this->start_listen_time_>72000) && (esphome::millis() - this->start_listen_time_<72500) && this->status_==AMS_LISTENING){
    // we did not receive data.... Out of sync?
    ESP_LOGD("nRF905", "Sensored data not received - timed out");
    // this->parent_->dump_config();
    this->status_ = AMS_NOT_SYNCED;
  }
  return false;
}

void AMSensor::process_data(uint8_t *replyBuffer, uint8_t size){
  // deal with sync_RX
  if(this->status_==AMS_SYNCING_RX){
    if(replyBuffer[0]==0x08 && replyBuffer[1]==0x01){
      // 08 01 E4 03 00   00 08 00 00 00   00 00 01 00 00   00 00 55 55 55   CB 45
      ESP_LOGD("nRF905", "Syncing sensor xx - fininshed RX: received data");
      this->status_ = AMS_SYNCED;
      this->start_listen_time_ = esphome::millis();
    }
    // else
      return ;
  }

  if(esphome::millis()-this->start_listen_time_ < 1000){
    ESP_LOGD("nRF905", "Ignoring msg as less than 1s from previous");
    return;
  }
  // that should be real data
  this->start_listen_time_ = esphome::millis();
  this->status_ = AMS_SYNCED;
  uint8_t ackn_buffer[PAYLOAD_TX_SIZE] = {0x09, 0xc3, 0x9c, 0x33, 0x33, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x35, 0xf6};
  if(replyBuffer[1]==1){
    //Elec 08 01 E4 03 00   00 08 00 00 00   00 00 01 00 00   00 00 55 55 55   CB 45
    float val = ( (uint16_t)replyBuffer[2] | (uint16_t)replyBuffer[3]<<8) / 4.76f;
    ESP_LOGD("nRF905", "dec=%d val=%f", (uint16_t)replyBuffer[2] | (uint16_t)replyBuffer[3]<<8, val);
    this->publish_state(val);
    ackn_buffer[6] = 0x01;
    ackn_buffer[20] = 0xDF;
    ackn_buffer[21] = 0x88;
  }
  //send ackn
  // (6333993c) 09 c39c3333 0c000000000000000000000000000035f6 water
  // (633396c9) 09 c39c3333 0c0100000000000000000000000000df88 elec
  transceiver.standby();
  // transceiver.setChannel(this->channel_);
  transceiver.write(this->addr_, ackn_buffer, sizeof(ackn_buffer));
  esphome::delay(100);
  while(!transceiver.TX(NRF905_NEXTMODE_TX, true));
  esphome::delay(500);
  while(!transceiver.TX(NRF905_NEXTMODE_RX , true));
  ESP_LOGD("nRF905", "Ackn sent");
}


void AquaMonitor::setup() {
  drUP=0;
  amUP=0;
  txDone=0;
  packetStatus = PACKET_NONE;
    // This will be called by App.setup()
    ESP_LOGCONFIG(TAG, "Setting up AquaMonitor...");
    
    
    // SPI.pins(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
    SPI.begin();
    // Minimal wires (interrupt mode)
	transceiver.begin(
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
  // transceiver.setChannel(109); //109:433.3
  transceiver.setCRC(NRF905_CRC_DISABLE );
  transceiver.setAddressSize(4, 4);  //tx, rx
  transceiver.setListenAddress(this->base_addr_);
  transceiver.setPayloadSize(PAYLOAD_TX_SIZE, PAYLOAD_SIZE); //tx, rx
  transceiver.setAutoRetransmit(true);
    
  transceiver.RX();
  
  transceiver.getConfigRegisters(regs);
  ESP_LOGCONFIG(TAG, "Registers: %02X %02x %02X %02x %02X %02x %02X %02x %02X %02x ", 
  regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7], regs[8], regs[9]);

}

void AquaMonitor::update() {
  // This will be called by App.loop()
  uint8_t success;
  uint8_t drUPbuf;
  uint8_t amUPbuf;
  success = packetStatus;
  drUPbuf = drUP;
  drUP = 0;
  // if (drUPbuf==1){
  //   ESP_LOGD(TAG, "drUp was 1");
  // }
  // amUPbuf = amUP;
  // amUP = 0;
  // if (amUPbuf==1){
  //   ESP_LOGD(TAG, "amUp was 1");
  // }
  // if (txDone==1){
  //   ESP_LOGD(TAG, "tX finished was 1");
  //   txDone=0;
  // }
  packetStatus=PACKET_NONE;

  // we sync the sensors if need
  for (AMSensor *amsensor : this->sensors_){
    if (amsensor->need_sync()){
      if(amsensor->sync()){
        cur_listen = amsensor;
      }
      break;
    }
  }

  // we setup for listening correct sensor
  for (AMSensor *amsensor : this->sensors_){
    if(amsensor->setup_listening())
      cur_listen = amsensor;
  }
  
  //Now look if we received data
  if(success == PACKET_INVALID){
      ESP_LOGD("nRF905", "Invalid package received");
      uint8_t replyBuffer[PAYLOAD_SIZE];
      transceiver.read(replyBuffer, sizeof(replyBuffer));
      ESP_LOGD("nRF905", "Received Invalid:");
      for (uint8_t i=0;i<PAYLOAD_SIZE;i++){
        ESP_LOGD("nRF905", "B[%i] %02x", i, replyBuffer[i]);
      }
      transceiver.RX();
  }
  if(success == PACKET_OK){
    ESP_LOGD("nRF905", "Valid package received");
    // Get the reply data
    uint8_t replyBuffer[PAYLOAD_SIZE];
    transceiver.read(replyBuffer, sizeof(replyBuffer));
    if (replyBuffer[0]==0) return; // ignore 00....
    ESP_LOGD(
      "nRF905",
      "Received: %02X %02X %02X %02X %02X   %02X %02X %02X %02X %02X   %02X %02X %02X %02X %02X   %02X %02X %02X %02X %02X   %02X %02X",
      replyBuffer[0], replyBuffer[1], replyBuffer[2], replyBuffer[3], replyBuffer[4],
      replyBuffer[5], replyBuffer[6], replyBuffer[7], replyBuffer[8], replyBuffer[9],
      replyBuffer[10], replyBuffer[11], replyBuffer[12], replyBuffer[13], replyBuffer[14],
      replyBuffer[15], replyBuffer[16], replyBuffer[17], replyBuffer[18], replyBuffer[19],
      replyBuffer[20], replyBuffer[21]
    );
    // test crc
    bool crc_ok = ( (uint16_t)replyBuffer[20] | (uint16_t)replyBuffer[21]<<8 ) == CRC16.kermit(replyBuffer, 20);
    if(crc_ok){
      cur_listen->process_data(replyBuffer, sizeof(replyBuffer));
    }
      
  }

}


void AquaMonitor::dump_config() {
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

  // LOG_SENSOR("  ", TAG, sensor1);
}
