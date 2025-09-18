//This is a VERY simple program for displaying your keg stats on your local network. This is a good jumping off point for doing more sophisticated things (like programing a wifi/bluetooth LED display of your keg stats).

#include <WiFi.h>
#include "HX711.h"

// include your wi-fi credentials here---if you are doing bluetooth, I would recommend not using wifi credentials (some cheapter ESP32 have a hard time doing both wifi and bluetooth simultaneously).
const char* ssid = "yourWiFiName";
const char* password = "yourWiFiPassword";

//These relate to your hardware: these are the pins labels on your ESP32 that you are connecting the HX711 Amp to.
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

// === WIFI SETUP ===
void connectToWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");
  Serial.println(WiFi.localIP());
}

// === SCALE CALIBRATION ===
//This will depend on your system. You can get this by checking out the serial monitor and calibrating.
//14100.0 is the "zero point" (empty scale)
//This works when the readings are flipped (more weight means smaller number (the raw reading is negative, absolute value grows)
//5850.0 is what was calibrated to 1lb


float rawToPounds(long reading) {
  return (14100.0 - reading) / 5850.0;
}

float computeAveragePounds() {
  long sum = 0;
  for (int i = 0; i < NUM_READINGS; i++) sum += readings[i];
  float avgRaw = sum / (float)NUM_READINGS;
  return rawToPounds(avgRaw);
}

//My custom alerts. I know, corny.
String getBeerStatus(float percent) {
  if (percent >= 76) return "Plenty of beer!";
  else if (percent >= 51) return "Mostly full.";
  else if (percent >= 20) return "Hope you're not too thirsty.";
  else return "Get more beer you muppet.";
}

// === MAIN SETUP ===
void setup() {
  Serial.begin(115200);
  connectToWiFi();
  scale.begin(DOUT, HX_SCK);
  server.begin();
}

// === MAIN LOOP ===
void loop() {
  // --- TAKE A NEW SCALE READING EVERY 1 SECOND ---
  if (millis() - lastReadingTime > readingInterval) {
    lastReadingTime = millis();

    long raw = scale.read();
    readings[readingIndex] = raw;
    readingIndex = (readingIndex + 1) % NUM_READINGS;
    if (readingIndex == 0) bufferFilled = true;

    if (bufferFilled) {
      latestAvgPounds = computeAveragePounds();
      //This is for the calibration of my keg+tubing setup weights 8lbs, and I know that in a 5 gallon keg there is close to 41.7lbs of beer.
      latestPercent = 100.0 * ((latestAvgPounds - 8.0) / 41.7);
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
