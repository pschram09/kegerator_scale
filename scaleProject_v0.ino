//Scale project
//Set up a scale to monitor beer levels
//ESP32 with HX711 load cell amplifier
// I strongly recommend using V1

#include <WiFi.h>
#include "HX711.h"

const char* ssid = "pleaseleaveby9pm";
const char* password = "pineapple";

const int DOUT = 4;
const int HX_SCK = 5;

HX711 scale;
WiFiServer server(80);

const int NUM_READINGS = 8;
long readings[NUM_READINGS];
int readingIndex = 0;
bool bufferFilled = false;

unsigned long lastReadingTime = 0;
const unsigned long readingInterval = 1000;  // every 1 second

float latestAvgPounds = 0.0;
float latestPercent = 0.0;

const float SCALE_OFFSET = 14100.0;
const float SCALE_FACTOR = 5850.0;
const float EMPTY_WEIGHT = 8.0;
const float FULL_WEIGHT = 41.7;

// === WIFI SETUP ===
void connectToWiFi() {
    unsigned long startAttemptTime = millis();
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startAttemptTime > 10000) {  // 10 second timeout
            Serial.println("Failed to connect to WiFi");
            return;
        }
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to Wi-Fi");
    Serial.println(WiFi.localIP());
}

// === SCALE CALIBRATION ===
float rawToPounds(long reading) {
  return (SCALE_OFFSET - reading) / SCALE_FACTOR;
}

float computeAveragePounds() {
  long sum = 0;
  for (int i = 0; i < NUM_READINGS; i++) sum += readings[i];
  float avgRaw = sum / (float)NUM_READINGS;
  return rawToPounds(avgRaw);
}

String getBeerStatus(float percent) {
  if (percent >= 76) return "Plenty of beer!";
  else if (percent >= 51) return "Mostly full.";
  else if (percent >= 26) return "Hope you're not too thirsty.";
  else return "Get more beer you muppet.";
}

// === MAIN SETUP ===
void setup() {
  Serial.begin(115200);
  connectToWiFi();
  if (!scale.begin(DOUT, HX_SCK)) {
      Serial.println("HX711 not found!");
      while (1);
  }
  scale.tare();  // Reset the scale to zero
  server.begin();
}

// === MAIN LOOP ===
void loop() {
  // --- TAKE A NEW SCALE READING EVERY 1 SECOND ---
  if (millis() - lastReadingTime > readingInterval) {
    lastReadingTime = millis();

    long raw = scale.read();
    if (raw == -1) {  // Invalid reading
        Serial.println("Invalid scale reading");
        return;
    }
    readings[readingIndex] = raw;
    readingIndex = (readingIndex + 1) % NUM_READINGS;
    if (readingIndex == 0) bufferFilled = true;

    if (bufferFilled) {
      latestAvgPounds = computeAveragePounds();
      latestPercent = 100.0 * ((latestAvgPounds - EMPTY_WEIGHT) / (FULL_WEIGHT - EMPTY_WEIGHT));
      latestPercent = constrain(latestPercent, 0.0, 100.0);

      Serial.print("Raw: ");
      Serial.print(raw);
      Serial.print(" | Avg (lbs): ");
      Serial.print(latestAvgPounds, 2);
      Serial.print(" | % full: ");
      Serial.println(latestPercent, 1);
    }
  }

  // --- HANDLE WEB CLIENT ---
  WiFiClient client = server.available();
  if (client && bufferFilled) {
    while (client.connected()) {
      if (client.available()) {
        String line = client.readStringUntil('\n');
        if (line.length() == 1 && line[0] == '\r') break;
      }
    }

    String status = getBeerStatus(latestPercent);

    String html = "HTTP/1.1 200 OK\r\n";
    html += "Content-Type: text/html\r\n";
    html += "Connection: close\r\n\r\n";

    html += "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    html += "<meta http-equiv='refresh' content='5'></head><body>";
    html += "<h1>Beer Level Monitor</h1>";
    html += "<p>Weight: " + String(latestAvgPounds, 2) + " lbs</p>";
    html += "<p>Percent full: " + String(latestPercent, 1) + "%</p>";
    html += "<h2>" + status + "</h2>";
    html += "</body></html>";

    client.print(html);
    delay(1);  // allow time for transmission
    client.stop();
  }
}

