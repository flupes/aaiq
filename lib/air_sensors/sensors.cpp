#include "sensors.h"

#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

static const size_t kJsonCapacity = 2000;
static const char *kPurpleAirApiUrl = "https://api.purpleair.com";
static const char *kGroupRequest = "/v1/groups/";
static const char *kDataRequest =
    "/members?fields=name%2Chumidity%2Ctemperature%2Cpressure%2Cpm2.5_10minute";
static const char *kFilterRequest = "&location_type=0&max_age=900";

bool AirSensors::UpdateData(const char *key) {
  String url = String(kPurpleAirApiUrl) + String(kGroupRequest) + String(kSensorGroupId) +
               String(kDataRequest) + String(kFilterRequest);

  Serial.print("Request URL = ");
  Serial.println(url);

  Serial.print("Memory heap before retrieving data = ");
  Serial.println(ESP.getFreeHeap());

  BearSSL::WiFiClientSecure *client = new BearSSL::WiFiClientSecure();
  HTTPClient http;

  unsigned long start = millis();
  // Send request
  http.useHTTP10(true);

  // Yes, totally insecure :-(
  client->setInsecure();

  http.begin(*client, url);

  Serial.print("Memory heap after before https.begin() = ");
  Serial.println(ESP.getFreeHeap());

  http.addHeader("X-API-Key", key);
  http.GET();

  Serial.print("Retrieve data time (ms) = ");
  Serial.println(millis() - start);

  size_t count = ParseSensors(http);

  // Disconnect
  http.end();

  Serial.print("Memory heap before after https.end() = ");
  Serial.println(ESP.getFreeHeap());

  // Delete the WiFiSecureClient memory hog of 21K !
  delete client;

  Serial.print("Memory heap after parsing is complete and client release = ");
  Serial.println(ESP.getFreeHeap());

  return (count > 0);
}

void AirSensors::PrintSensorData(const SensorData &data) {
  Serial.print("Sensor # ");
  Serial.println(data.id);
  Serial.print("  temperature = ");
  Serial.println(data.temperature);
  Serial.print("  humidity =    ");
  Serial.println(data.humidity);
  Serial.print("  pressure =    ");
  Serial.println(data.pressure);
  Serial.print("  pm2.5 10min =    ");
  Serial.println(data.pm_2_5_10min);
}

size_t AirSensors::ParseSensors(HTTPClient &http) {
  Serial.print("Memory heap before JSON doc = ");
  Serial.println(ESP.getFreeHeap());
  DynamicJsonDocument doc(kJsonCapacity);
  StaticJsonDocument<400> subdoc;
  Serial.print("Memory heap after JSON doc = ");
  Serial.println(ESP.getFreeHeap());

  unsigned long start = millis();
  auto err = deserializeJson(doc, http.getStream());
  if (err) {
    Serial.print("Error, doc deserializeJson() returned ");
    Serial.println(err.c_str());
    return 0;
  }
  unsigned long step = millis();
  Serial.print("Deserialization (ms) = ");
  Serial.println(step - start);

  // Timestamp
  timestamp_ = doc["time_stamp"];
  Serial.print("timestamp = ");
  Serial.println(timestamp_);

  // Fields returned
  size_t id_index = 1000;
  size_t temperature_index = 1000;
  size_t humidity_index = 1000;
  size_t pressure_index = 1000;
  size_t pm25ten_index = 1000;
  String sensor_index = "sensor_index";
  String temperature = "temperature";
  String humidity = "humidity";
  String pressure = "pressure";
  String pm2_5_10minute = "pm2.5_10minute";

  JsonArray fieldsDescription = doc["fields"].as<JsonArray>();
  size_t index = 0;
  for (JsonVariant field : fieldsDescription) {
    if (sensor_index.equals(field.as<String>())) {
      id_index = index++;
      continue;
    }
    if (temperature.equals(field.as<String>())) {
      temperature_index = index++;
      continue;
    }
    if (humidity.equals(field.as<String>())) {
      humidity_index = index++;
      continue;
    }
    if (pressure.equals(field.as<String>())) {
      pressure_index = index++;
      continue;
    }
    if (pm2_5_10minute.equals(field.as<String>())) {
      pm25ten_index = index++;
      continue;
    }
    index++;
  }
  if (id_index > 100 || temperature_index > 100 || humidity_index > 100 || pressure_index > 100 ||
      pm25ten_index > 100) {
    Serial.println("Error retrieving the field indices!");
    return 0;
  }
  size_t count = 0;
  Serial.print("Temperature Index = ");
  Serial.println(temperature_index);
  Serial.print("PM2.5_10minute Index = ");
  Serial.println(pm25ten_index);

  // Array of sensors
  JsonArray sensorsArray = doc["data"].as<JsonArray>();
  for (JsonVariant sensor : sensorsArray) {
    sensorsData_[count].id = sensor[id_index];
    sensorsData_[count].temperature = sensor[temperature_index];
    sensorsData_[count].humidity = sensor[humidity_index];
    sensorsData_[count].pressure = sensor[pressure_index];
    sensorsData_[count].pm_2_5_10min = sensor[pm25ten_index];
    count++;
  }
  sensorsCount_ = count;

  unsigned long stop = millis();
  Serial.print("Parsing to structure (ms) = ");
  Serial.println(stop - step);
  return count;
}
