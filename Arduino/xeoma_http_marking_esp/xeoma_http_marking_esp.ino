#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
const uint32_t serialSpeed                 = 115200;
const uint32_t defaultPostInterval         = 3000;
const char ssid[]                          = "your-ssid";
const char password[]                      = "your-pass";
const char xeomaServerHost[]               = "192.168.0.11";
//const char xeomaServerHost[]             = "xeoma.private.local";
const uint16_t xeomaServerPort             = 10090;
//#define XEOMA_MARKING_USE_SECURITY                       
#if defined(XEOMA_MARKING_USE_SECURITY)
const char httpMarkCommand[]                 = "GET /http_marking?login=%s&password=%s&module=HttpMarking.%d&params=millis=%lu|Temperature=%.2f%%C2%%B0C HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n";
#else
const char httpMarkCommand[]                 = "GET /http_marking?module=HttpMarking.%d&params=millis=%lu|Temperature=%.2f%%C2%%B0C HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n";
#endif
const uint8_t httpMarkModuleId               = 15;
const char httpMarkModuleLogin[]             = "username";
const char httpMarkModulePassword[]          = "userpass";
const uint8_t oneWireBusPin                  = 16;
OneWire oneWire(oneWireBusPin);
DallasTemperature sensor(&oneWire);
DeviceAddress insideThermometer;
WiFiClient client;
bool initSensor() {
  sensor.begin();
  return sensor.getAddress(insideThermometer, 0);
}
float readSensor() {
  sensor.requestTemperatures();
  return sensor.getTempC(insideThermometer);
}
void setup() {
  Serial.begin(115200);
  delay(10);

  Serial.print("Connecting to "); Serial.print(ssid); WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("connected");
  Serial.print(F("IP: ")); Serial.println(WiFi.localIP());
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
      float temperature = readSensor();
      Serial.print(F("Connecting to ")); Serial.print(xeomaServerHost); Serial.print(F("..."));
      if (client.connect(xeomaServerHost, xeomaServerPort)) {
        char buffer[200];
        Serial.println(F(" OK"));
#if defined(XEOMA_MARKING_USE_SECURITY)
        snprintf(buffer, sizeof(buffer), httpMarkCommand, httpMarkModuleLogin, httpMarkModulePassword, httpMarkModuleId, millis(), temperature, xeomaServerHost);
#else
        snprintf(buffer, sizeof(buffer), httpMarkCommand, httpMarkModuleId, millis(), temperature, xeomaServerHost);
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
      //Serial.write(c);
      if (!client.connected()) {
        Serial.println(F("\nDisconnecting...\n"));
        client.stop();
        delay(5);
      }
    }
  }
}
