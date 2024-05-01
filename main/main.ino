/*
  Lora Send And Receive
  This sketch demonstrates how to send and receive data with the MKR WAN 1300/1310 LoRa module.
  This example code is in the public domain.
*/

#include <MKRWAN.h>
#include "arduino_secrets.h"
using namespace std;

LoRaModem modem;

// Uncomment if using the Murata chip as a module
// LoRaModem modem(Serial1);

// Please enter your sensitive data in the Secret tab or arduino_secrets.h
String appEui = SECRET_APP_EUI;
String appKey = SECRET_APP_KEY;

const int MOISTURE_PIN = 0;    // Pin to read soil sensor
const int TEMP_PIN = 1;    // Pin to read soil sensor
int m_volt;
int t_volt;
int UplinkTime = 60000;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  // put your setup code here, to run once:
  Serial.begin(115200); // Initialize Serial for debuging purposes.
  while (!Serial);
  // change this to your regional band (eg. US915, AS923, ...)
  if (!modem.begin(US915)) {
    Serial.println("Failed to start module");
    while (1) {}
  };
  Serial.print("Your module version is: ");
  Serial.println(modem.version());
  Serial.print("Your device EUI is: ");
  Serial.println(modem.deviceEUI());

  delay(5000); //adding a delay(5000) between modem.begin(US915) and modem.joinOTAA(..) makes joining much more reliable. I am not sure why.

  int connected = modem.joinOTAA(appEui, appKey,300000); //five minute timeout
  if (!connected) {
    Serial.println("Something went wrong; are you indoor? Move near a window and retry");
    while (1) {}
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

void loop() {
  m_volt = analogRead (MOISTURE_PIN);
  t_volt = analogRead (TEMP_PIN);

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

  //print values being sent
  Serial.println();
  Serial.print("Sending values:");
  Serial.println();
  Serial.print("Temp: ");
  Serial.print(t_volt*1000);
  Serial.println();
  Serial.print("Moisture: ");
  Serial.println();
  Serial.print(m_volt*1000);
  Serial.println();

  //send packets
  int err;
  modem.beginPacket();
  for (uint8_t c = 0; c < idx; c++) {
    modem.write(payload[c]);
  }
  err = modem.endPacket(true);
  if (err > 0) {
    Serial.println("Message sent correctly!");
  } else {
    Serial.println("Error sending message :(");
    Serial.println("(you may send a limited amount of messages per minute, depending on the signal strength");
    Serial.println("it may vary from 1 message every couple of seconds to 1 message every minute)");
  }

  //check for downlink messages from gateway
  delay(5000);
  if (!modem.available()) {
    Serial.println("No downlink message received at this time.");
    delay(UplinkTime); //wait until next reading
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
  Serial.print("Received: ");
  for (unsigned int j = 0; j < i; j++) {

    Serial.print(rcv[j] >> 4, HEX);
    Serial.print(rcv[j] & 0xF, HEX);
    Serial.print(" ");

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

  Serial.println();

  delay(UplinkTime); //wait until next reading

  return; //end iteration
}


