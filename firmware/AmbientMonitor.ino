/*
 * Ambient Monitor — Room Temperature, Humidity & Air Quality
 * ESP32 + DHT11 + MQ135
 * Posts JSON {"roomTemp":.., "humidity":.., "airQuality":..} to the ambient
 * dashboard's API every 3s, matching the dashboard's polling interval.
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

// ------ SETTINGS ------
const char* WIFI_SSID  = "YOUR_WIFI";
const char* WIFI_PASS  = "YOUR_PASSWORD";
const char* SERVER_URL = "http://YOUR_PC_IP/ambient/api/save_reading.php"; // set to your server's LAN IP

// ------ PINS ------
#define PIN_DHT    5    // DHT11 data pin
#define PIN_MQ135  34   // MQ135 analog output — must be an ADC1 pin (32-39) since
                         // ADC2 pins conflict with WiFi on the ESP32

// ------ TIMING ------
const unsigned long SEND_INTERVAL = 3000;  // matches the dashboard's 3s poll
unsigned long lastSend = 0;

// ------ OBJECT ------
DHT dht(PIN_DHT, DHT11);

// ------ STATE (fallback when a read fails) ------
float lastRoomTemp = 25.0;
float lastHumidity = 50.0;

// MQ135 is noisy on a single read — average a handful of quick samples instead
// of trusting one. It also needs a warm-up period (a couple of minutes) after
// power-on before readings settle; expect elevated values right after boot.
int readAirQuality() {
  long total = 0;
  const int samples = 8;
  for (int i = 0; i < samples; i++) {
    total += analogRead(PIN_MQ135);
    delay(5);
  }
  return total / samples;
}

void smartConnectWiFi() {
  WiFi.mode(WIFI_STA);
  if (WiFi.status() == WL_CONNECTED) WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi connecting");
  for (int i = 0; WiFi.status() != WL_CONNECTED && i < 60; i++) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected, IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("WiFi connect FAILED");
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  dht.begin();
  smartConnectWiFi();
}

void loop() {
  if (millis() - lastSend >= SEND_INTERVAL) {
    lastSend = millis();

    float roomTemp = dht.readTemperature();
    float humidity = dht.readHumidity();
    int airQuality = readAirQuality();

    // DHT11 returns NaN on a failed read (common on cheap modules). A NaN in the
    // JSON breaks json_decode() server-side and the whole reading gets dropped,
    // so fall back to the last good value instead of sending garbage.
    if (isnan(roomTemp)) roomTemp = lastRoomTemp; else lastRoomTemp = roomTemp;
    if (isnan(humidity)) humidity = lastHumidity; else lastHumidity = humidity;

    Serial.printf("Room=%.1f C  Humidity=%.1f%%  AirQuality=%d\n", roomTemp, humidity, airQuality);

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[WiFi] Reconnecting...");
      smartConnectWiFi();
    }

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(SERVER_URL);
      http.addHeader("Content-Type", "application/json");

      String json = String("{") +
        "\"roomTemp\":" + String(roomTemp, 1) + "," +
        "\"humidity\":" + String(humidity, 1) + "," +
        "\"airQuality\":" + String(airQuality) +
        "}";

      int code = http.POST(json);
      if (code != 200) Serial.printf("POST failed: %d\n", code);
      http.end();
    }
  }
}
