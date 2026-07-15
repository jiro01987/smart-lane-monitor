#include <WiFiS3.h>

const char* ssid     = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

void setup() {
  Serial.begin(9600);
  delay(3000); // give the Serial Monitor time to fully attach before we print anything

  Serial.println("STEP 1: Serial is alive.");

  Serial.print("STEP 2: WiFi module firmware version: ");
  Serial.println(WiFi.firmwareVersion());

  Serial.println("STEP 3: calling WiFi.begin() now...");
  int status = WiFi.begin(ssid, password);
  Serial.print("STEP 4: WiFi.begin() returned status code: ");
  Serial.println(status);
}

void loop() {
  Serial.print("WiFi.status() = ");
  Serial.println(WiFi.status());
  delay(1000);
}
