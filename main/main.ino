/*
 * Merged CSU Soil Sensor MKR WAN 1310 Firmware
 * Combines production-ready structure from EXO sonde code with CSU soil sensor readings
 */

#include <Arduino.h>
#include <MKRWAN.h>
#include <ArduinoLowPower.h>
#include <Adafruit_SleepyDog.h>
#include <FlashStorage.h>
#include "arduino_secrets.h"

// Debug flags
const bool DEBUG = false;
#define dbg_print(x)     if (DEBUG) Serial.print(x)
#define dbg_println(x)   if (DEBUG) Serial.println(x)

// LoRaWAN credentials
LoRaModem modem;
String appEui = SECRET_APP_EUI;
String appKey = SECRET_APP_KEY;

// Soil sensor analog pins
const int MOISTURE_PIN = A0;
const int TEMP_PIN     = A1;

// Default uplink interval: 60 seconds (in ms)
const uint32_t DEFAULT_UPLINK_MS = 60000;
uint32_t UplinkTime = DEFAULT_UPLINK_MS;

// Persistent storage of uplink interval
typedef struct {
    uint32_t uplink_ms;
} PersistentConfig;
FlashStorage(config_store, PersistentConfig);

// --- WATCHDOG UTILITIES ---
// Reset the watchdog timer and optionally log a tag
void pingWatchdog(const char* tag = "") {
    Watchdog.reset();
    if (DEBUG && tag[0] != '\0') {
        dbg_print("[WDT] ping: ");
        dbg_println(tag);
    }
}

// Delay in ms while periodically pinging the watchdog
void mydelay(uint32_t ms) {
    uint32_t elapsed = 0;
    while (elapsed < ms) {
        pingWatchdog();
        delay(100);
        elapsed += 100;
    }
}

// Load saved uplink interval or set default
void loadConfig() {
    pingWatchdog("loadConfig");
    PersistentConfig cfg = config_store.read();
    if (cfg.uplink_ms >= 60000 && cfg.uplink_ms <= 7200000) {
        UplinkTime = cfg.uplink_ms;
    } else {
        UplinkTime = DEFAULT_UPLINK_MS;
    }
    dbg_print("Loaded UplinkTime (ms): "); dbg_println(UplinkTime);
}

// Save current uplink interval
void saveConfig() {
    pingWatchdog("saveConfig");
    PersistentConfig current = config_store.read();
    if (current.uplink_ms != UplinkTime) {
        config_store.write({ UplinkTime });
        dbg_print("Saved UplinkTime (ms): "); dbg_println(UplinkTime);
    }
}

// Simple LED blink for status indication
void blink(int times = 4, int delayMs = 300) {
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        mydelay(delayMs);
        digitalWrite(LED_BUILTIN, LOW);
        mydelay(delayMs);
    }
}

// Join LoRaWAN via OTAA with retry/backoff
bool joinNetwork() {
    pingWatchdog("joinNetwork");
    blink();
    bool joined = modem.joinOTAA(appEui, appKey, 15000);
    if (!joined) {
        return false;
    }
    return true;
}

// Enter deep sleep until next uplink
void enterSleep() {
    pingWatchdog("enterSleep");
    // Release LoRa I/O pins to reduce current
    pinMode(LORA_IRQ_DUMB, INPUT);
    pinMode(LORA_BOOT0,    INPUT);
    pinMode(LORA_RESET,    INPUT);
    modem.sleep(true);
    dbg_print("Sleeping for ms: "); dbg_println(UplinkTime);
    LowPower.deepSleep(UplinkTime);
    // Upon wake, pins reset back by hardware
}

// Handle downlink to adjust uplink interval
void handleDownlink() {
    pingWatchdog("handleDownlink");
    if (!modem.available()) return;
    uint8_t buf[64];
    int len = 0;
    while (modem.available() && len < (int)sizeof(buf)) {
        pingWatchdog("downlinkRead");
        buf[len++] = modem.read();
    }
    if (len == 0) return;
    int code = buf[len-1] & 0x0F; // last nibble
    uint32_t newInterval;
    switch (code) {
        case 0: newInterval = 60000;    break;
        case 1: newInterval = 300000;   break;
        case 2: newInterval = 900000;   break;
        case 3: newInterval = 1800000;  break;
        case 4: newInterval = 3600000;  break;
        case 5: modem.restart();        return;
        default: newInterval = DEFAULT_UPLINK_MS;
    }
    if (newInterval != UplinkTime) {
        UplinkTime = newInterval;
        saveConfig();
    }
}

void setup() {
    if (DEBUG) {
        Serial.begin(115200);
        while (!Serial) {
            pingWatchdog("SerialWait");
        }
    }
    pinMode(LED_BUILTIN, OUTPUT);

    // Load uplink interval
    loadConfig();

    // Initialize modem
    if (!modem.begin(US915)) {
        dbg_println("Failed to start modem");
        while (1) {
            pingWatchdog("modemInitFail");
        }
    }
    dbg_print("Your module version is: "); dbg_println(modem.version());
    dbg_print("Your device EUI is: "); dbg_println(modem.deviceEUI());

    modem.minPollInterval(60);
    modem.setPort(10);
    modem.dataRate(3);
    modem.setADR(true);

    // Join network with exponential backoff
    int waitTime = 10000;
    while (!joinNetwork()) {
        dbg_println("Join failed, retrying...");
        mydelay(waitTime);
        if (waitTime < 900000) waitTime *= 2;
    }

    // Enable watchdog (max 16s)
    int wd = Watchdog.enable(16000);
    dbg_print("Watchdog timeout (ms): "); dbg_println(wd);
}

void loop() {
    pingWatchdog("loopStart");
    // Read sensors
    int m_raw = analogRead(MOISTURE_PIN);
    int t_raw = analogRead(TEMP_PIN);
    int moisture = m_raw * 100;
    int temp     = t_raw * 100;

    // Build Cayenne LPP payload
    uint8_t payload[16];
    int idx = 0;
    // Temperature (channel 1)
    payload[idx++] = 1;
    payload[idx++] = 103;
    payload[idx++] = highByte(temp);
    payload[idx++] = lowByte(temp);
    // Moisture (channel 2)
    payload[idx++] = 2;
    payload[idx++] = 104;
    payload[idx++] = highByte(moisture);
    payload[idx++] = lowByte(moisture);

    //print values being sent
    dbg_println();
    dbg_print("Temp: "); dbg_println(moisture);
    dbg_print("Moisture: "); dbg_println(temp);

    // Send packet with retries
    pingWatchdog("beforeSend");
    modem.beginPacket();
    modem.write(payload, idx);
    int err = modem.endPacket(true);
    if (!(err > 0)) {
        dbg_println("Uplink error");
    }

    // Process any downlink
    handleDownlink();

    // Indicate activity
    blink(2, 100);

    // Sleep until next cycle
    enterSleep();
}


