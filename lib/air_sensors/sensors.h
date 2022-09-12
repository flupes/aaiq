#ifndef AAQIM_SENSORS_H
#define AAQIM_SENSORS_H

#include <stdint.h>
#include <stdlib.h>

// Forward declaration
class HTTPClient;

const size_t kMaxSensors = 9;

// Custom group of sensors to use
static const size_t kSensorGroupId = 1309;

struct SensorData {
  size_t id;
  int16_t temperature;
  int16_t humidity;
  float pressure;
  float pm_2_5_10min;
};

class AirSensors {
 public:
  AirSensors() : sensorsCount_(0), timestamp_(0) {
    noData_.id = 0;
  }

  bool UpdateData(const char* key);
  
  void PrintAllData() {
    for (size_t index = 0; index < sensorsCount_; index++) {
      PrintSensorData(sensorsData_[index]);
    }
  }

  void PrintSensorData(const SensorData &data);

  const SensorData &Data(size_t index) const {
    if (index < sensorsCount_) {
      return sensorsData_[index];
    } else {
      return noData_;
    }
  }

  size_t Count() const { return sensorsCount_; }

  uint32_t Timestamp() const { return timestamp_; }

 protected:
  size_t ParseSensors(HTTPClient &http);

  size_t sensorsCount_;
  uint32_t timestamp_;
  SensorData noData_;
  SensorData sensorsData_[kMaxSensors];
};

#endif
