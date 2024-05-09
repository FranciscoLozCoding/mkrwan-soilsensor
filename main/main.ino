#include <MKRWAN.h>
#include <ArduinoLowPower.h>
#include "arduino_secrets.h"
using namespace std;

LoRaModem modem;

// Uncomment if using the Murata chip as a module
// LoRaModem modem(Serial1);

String appEui = SECRET_APP_EUI;
String appKey = SECRET_APP_KEY;

const int MOISTURE_PIN = 0;    // Pin to read soil sensor
const int TEMP_PIN = 1;    // Pin to read soil sensor
int m_volt;
int t_volt;
int UplinkTime = 60000;

void blink() {
  int WaitTime=300;
  for (int i = 0; i < 4; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(WaitTime);
    digitalWrite(LED_BUILTIN, LOW);
    delay(WaitTime);
  }
}

bool joinNetwork() {  
  blink();
  int connected = modem.joinOTAA(appEui, appKey, 15000); // 15 sec timeout
  if (connected) {
    return true;
  } else {
    return false;
  }
}

void setup() {
  delay(5000); //allow some downtime to upload a new sketch
  pinMode(LED_BUILTIN, OUTPUT);

  modem.begin(US915);

  if (joinNetwork()) {
    int waitTime = 10000;
    while (!joinNetwork()) {
      delay(waitTime);
      // Double the wait time, up to a maximum of 15 minutes
      if (waitTime < 900000) {
        waitTime *= 2;
      }
    }
  }

  // Set poll interval to 60 secs.
  modem.minPollInterval(60);
  // NOTE: independent of this setting, the modem will
  // not allow sending more than one message every 2 minutes,
  // this is enforced by firmware and can not be changed.
  modem.setPort(10);
  modem.dataRate(3);
  modem.setADR(true);
}

void end() {
  // set pins as INPUT
  // they were set as OUTPUT by modem.begin()
  // but they can be set back
  // to INPUT to reduce consumption by ~0.30mA
  pinMode(LORA_IRQ_DUMB, INPUT);
  pinMode(LORA_BOOT0, INPUT);
  pinMode(LORA_RESET, INPUT);

  modem.sleep(true);
  LowPower.deepSleep(UplinkTime); 

  //set back
  pinMode(LORA_IRQ_DUMB, OUTPUT);
  pinMode(LORA_BOOT0, OUTPUT);
  pinMode(LORA_RESET, OUTPUT);
  return;
}

void loop() {
  m_volt = analogRead(MOISTURE_PIN);
  t_volt = analogRead(TEMP_PIN);

  //Cayenne encoding
  uint8_t payload[30]; // Allow for up to 30 byte payload
  int tempToSend = (int) (t_volt * 100);
  int MoistToSend = (int) (m_volt * 100);
  int idx = 0;

  //temperature Cayenne encoding
  payload[idx++] = 1; //data channel
  payload[idx++] = 103; //temp data type
  payload[idx++] = highByte(tempToSend); //whole number
  payload[idx++] = lowByte(tempToSend); //decimal number
  
  //moisture Cayenne encoding
  payload[idx++] = 2; //data channel
  payload[idx++] = 104; //moisture data type
  payload[idx++] = highByte(MoistToSend); //whole number
  payload[idx++] = lowByte(MoistToSend); //decimal number

  //send packets
  modem.beginPacket();
  for (uint8_t c = 0; c < idx; c++) {
    modem.write(payload[c]);
  }
  int err = modem.endPacket(true);

  if (!(err > 0)) {
    end();
    return;
  }

  //check for downlink messages from gateway
  if (!modem.available()) {
    end();
    return; //no downlink, end iteration
  }

  //get downlink message
  char rcv[64];
  int i = 0;
  while (modem.available()) {
    rcv[i++] = (char)modem.read();
  }

  int HexValue;

  //decript downlink message (only 00,01...05)
  for (unsigned int j = 0; j < i; j++) {
    //store Hex value
    HexValue = rcv[j] & 0xF;
  }

  //change uplink interval based on downlink message
  if(HexValue == 0) //1 minute
  {
    UplinkTime = 60000;
  }
  else if(HexValue == 1) //5 minutes
  {
    UplinkTime = 300000;
  }
  else if(HexValue == 2) //15 minutes
  {
    UplinkTime = 900000;
  }
  else if(HexValue == 3) //30 minutes
  {
    UplinkTime = 1.8e+6;
  }
  else if(HexValue == 4) //1 hour
  {
    UplinkTime = 3.6e+6;
  }
  else if(HexValue == 5) //Restarts the LoRaWAN module
  {
    digitalWrite(LED_BUILTIN, HIGH);
    modem.restart();
    setup();
    digitalWrite(LED_BUILTIN, LOW);
    return;
  }
  else //default
  {
    UplinkTime = 60000;
  }

  end();
  return; //end iteration
}


