#if defined(ARDUINO)

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
// #include <StreamUtils.h>

#include "credentials.h"
#include "stats.h"
#include "cfaqi.h"

// how to:
// https://arduinojson.org/v6/how-to/use-arduinojson-with-httpclient/

ESP8266WiFiMulti wifiMulti;

String request_str =
    "http://www.purpleair.com/json?show=59927|65489|54857|36667|25301";
// String request_str = "http://www.purpleair.com/json?show=59927";

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  wifiMulti.addAP(SSID, PASS);
}

void loop() {
  static bool connectionWasAlive = true;

  if (wifiMulti.run() != WL_CONNECTED) {
    if (connectionWasAlive == true) {
      connectionWasAlive = false;
      Serial.print("Looking for WiFi ");
    }
    Serial.print(".");
    delay(500);
  } else {
    if (connectionWasAlive == false) {
      Serial.println("Connected to WiFi :-)");
    }

    WiFiClient client;
    HTTPClient http;

    // Send request
    http.useHTTP10(true);
    http.begin(client, request_str);
    http.GET();

    // Parse response
    DynamicJsonDocument doc(16684);

    auto err = deserializeJson(doc, http.getStream());
    // ReadLoggingStream loggingStream(http.getStream(), Serial);
    // deserializeJson(doc, loggingStream);

    // Parse succeeded?
    if (err) {
      Serial.print(F("deserializeJson() returned "));
      Serial.println(err.c_str());
      return;
    }

    auto memoryUsed = doc.memoryUsage();
    Serial.print("Document Memory Usage = ");
    Serial.println(memoryUsed);

    auto version = doc["mapVersion"].as<String>();
    Serial.print("mapVersion = ");
    Serial.println(version);

    JsonArray sensorsArray = doc["results"].as<JsonArray>();
    Serial.print("Array count = ");
    Serial.println(sensorsArray.size());

    const int MAX_READINGS = 32;
    float cfvalues[MAX_READINGS];
    int readingsCount = 0;

    for (JsonObject sensor : sensorsArray) {
      int32_t id = sensor["ID"];
      Serial.print("ID = ");
      Serial.print(id);
      String label = sensor["Label"];
      Serial.print(" / Label = ");
      Serial.println(label);
      int32_t parent = sensor["ParentID"];
      if (parent == 0) {
        // This is the primary sensor (no parent)
        int16_t temperature_f = sensor["temp_f"];
        Serial.print("  Temperature deg F = ");
        Serial.println(temperature_f);
        int16_t humidity = sensor["humidity"];
        Serial.print("  Humidity % = ");
        Serial.println(humidity);
        float pressure = sensor["pressure"];
        Serial.print("  Pressure mb = ");
        Serial.println(pressure);
      } else {
        Serial.print("  parentID = ");
        Serial.println(parent);
      }
      float pm25cf = sensor["pm2_5_cf_1"];
      if (readingsCount < MAX_READINGS - 1) {
        cfvalues[readingsCount++] = pm25cf;
      }
      Serial.print("  pm 2.5 cf = ");
      Serial.println(pm25cf);
    }

    // Disconnect
    http.end();

    Serial.print("Number of readings = ");
    Serial.println(readingsCount);
    float avg;
    float error;
    mean_error(readingsCount, cfvalues, avg, error);
    Serial.print("average PM2.5 = ");
    Serial.print(avg);
    Serial.print(" / error = ");
    Serial.println(error);

    int16_t aqiValue;
    AqiLevel aqiLevel;
    pm25_to_aqi(avg, aqiValue, aqiLevel);
    Serial.print("AQI = ");
    Serial.print(aqiValue);
    Serial.print(" --> ");
    Serial.println(AqiNames[static_cast<int>(aqiLevel)]);

    delay(60 * 1000);
  }
}

#else
// Something trivial to avoid pio errors
int main(void) { return 0; }
#endif
