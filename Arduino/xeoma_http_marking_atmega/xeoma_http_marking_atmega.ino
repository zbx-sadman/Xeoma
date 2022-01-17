#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
const uint32_t serialSpeed                 = 115200;
const uint32_t defaultPostInterval         = 3000;
const uint8_t clientMac[]                  = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
const uint8_t clientIp[]                   = {192, 168, 0, 251};
const uint8_t clientDns[]                  = {192, 168, 0, 1};
const uint8_t clientGateway[]              = {192, 168, 0, 1};
const uint8_t clientSubnet[]               = {255, 255, 255, 0};
const char xeomaServerHost[]               = "192.168.0.11";
//const char xeomaServerHost[]               = "xeoma.private.local";
const uint16_t xeomaServerPort             = 10090;
//#define XEOMA_MARKING_USE_SECURITY                       
#if defined(XEOMA_MARKING_USE_SECURITY)
const char httpMarkCommand[]               = "GET /http_marking?login=%s&password=%s&module=HttpMarking.%d&params=millis=%lu|Light(lux)=%u HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n";
#else
const char httpMarkCommand[]               = "GET /http_marking?module=HttpMarking.%d&params=millis=%lu|Light(lux)=%u HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n";
#endif
const uint8_t httpMarkModuleId             = 8;
const char httpMarkModuleLogin[]           = "username";
const char httpMarkModulePassword[]        = "userpass";

#define BH1750_I2C_ADDRESS                 (0x23)
#define BH1750_CMD_POWERON                 (0x01)
#define BH1750_CONTINUOUS_HIGHRES_2        (0x11)
EthernetClient client;
bool initSensor() {
  uint8_t rc = 0x00;
  Wire.beginTransmission(BH1750_I2C_ADDRESS);
  Wire.write(BH1750_CMD_POWERON);
  rc |= Wire.endTransmission(true);
  Wire.beginTransmission(BH1750_I2C_ADDRESS);
  Wire.write(BH1750_CONTINUOUS_HIGHRES_2);
  rc |= Wire.endTransmission(true);
  return (0x00 == rc);
}
uint16_t readSensor() {
  uint16_t lightLevel;
  Wire.beginTransmission(BH1750_I2C_ADDRESS);
  Wire.requestFrom(BH1750_I2C_ADDRESS, 2);
  lightLevel = ((uint8_t) Wire.read() << 8);
  lightLevel |= (uint8_t) Wire.read();
  lightLevel = (uint32_t)lightLevel * 5 / 6;
  return lightLevel;
}
void setup() {
  Wire.begin();
  Serial.begin(serialSpeed);
  Serial.println(F("Initialize Ethernet"));
  if (Ethernet.begin(clientMac) == 0x00) {
    Ethernet.begin(clientMac, clientIp, clientDns, clientGateway, clientSubnet);
  }
  Serial.print(F("IP: ")); Serial.println(Ethernet.localIP());
  Serial.println(F("Initialize sensor"));
  if (!initSensor()) {
    Serial.println(F("Sensor fail, processing stopped")); Serial.println();
    while (true);
  }
}
void loop() {
  static uint32_t prevPostTime, postInterval;
  if (!client.connected()) {
    if (millis() - prevPostTime >= postInterval) {
      postInterval = defaultPostInterval;
      prevPostTime = millis();
      uint16_t lightLevel = readSensor();
      Serial.print(F("\nConnecting to ")); Serial.print(xeomaServerHost); Serial.print(F("..."));
      if (client.connect(xeomaServerHost, xeomaServerPort)) {
        char buffer[200];
        Serial.println(F(" OK"));
#if defined(XEOMA_MARKING_USE_SECURITY)
        snprintf(buffer, sizeof(buffer), httpMarkCommand, httpMarkModuleLogin, httpMarkModulePassword, httpMarkModuleId, millis(), lightLevel, xeomaServerHost);
#else
        snprintf(buffer, sizeof(buffer), httpMarkCommand, httpMarkModuleId, millis(), lightLevel, xeomaServerHost);
#endif
        Serial.print(F("HTTP request: ")); Serial.println(buffer);
        client.print(buffer);
      } else {
        Serial.println(F(" Fail"));
      }
    }
  } else {
    if (client.available() > 0) {
      char c = client.read();
      Serial.write(c);
      if (!client.connected()) {
        Serial.println(F("\nDisconnecting...\n"));
        client.stop();
        delay(5);
      }
    }
  }
}
