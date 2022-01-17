#include <WiFi.h>
#include <WebServer.h>

const uint32_t serialSpeed                 = 115200;
const char ssid[]                          = "your-ssid";
const char password[]                      = "your-pass";
const uint16_t xeomaSlavePort              = 80;
const uint8_t xeomaSlaveLedPin             = LED_BUILTIN;
WebServer server(xeomaSlavePort);
void setup() {
  pinMode(xeomaSlaveLedPin, OUTPUT);
  Serial.begin(115200);
  delay(10);
  Serial.print("Connecting to "); Serial.print(ssid); WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("connected");
  Serial.print(F("IP: ")); Serial.println(WiFi.localIP());
  server.on("/event-begin", handleEventBegin);
  server.on("/event-end", handleEventEnd);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}
void loop() {
  server.handleClient();
}
void handleEventBegin() {
  digitalWrite(xeomaSlaveLedPin, HIGH);
  server.send(200, "text/html", "OK");
}
void handleEventEnd() {
  digitalWrite(xeomaSlaveLedPin, LOW);
  server.send(200, "text/html", "OK");
}
void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}
