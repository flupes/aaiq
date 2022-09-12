#ifndef AAQIM_ANALYZE_H
#define AAQIM_ANALYZE_H

#if defined(ARDUINO)

#include <stdlib.h>

#include "air_sample.h"
#include "sensors.h"
#include "stats.h"

size_t ComputeStats(const AirSensors& sensors, AirSample& sample,
                    int32_t& primaryIndex) {
  primaryIndex = -1;
  size_t count = 0;
  float cfvalues[kMaxSensors];

  for (size_t i = 0; i < sensors.Count(); i++) {
    const SensorData& data = sensors.Data(i);

    // Age check is part of the group request (exclude sensors with measurement older than x minutes)

    // Consistency check is already part of the pm2.5_10minute field: does not include downgraded channels

    // keep which sensor is considered the primary one
    if (primaryIndex < 0) {
      primaryIndex = i;
    }

    cfvalues[count++] = data.pm_2_5_10min;;
  }

  if (primaryIndex < 0) {
    Serial.println("Could not identify a primary sensor!");
    sample.Set(0, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0);
  } else {
    Serial.print("Compute stats with n sensors = ");
    Serial.println(count);
    float avg, mae, nmae;
    avg = mean_error(count, cfvalues, mae, nmae);
    Serial.print("avg = ");
    Serial.print(avg);
    Serial.print(" | MAE = ");
    Serial.print(mae);
    Serial.print(" / NMAE = ");
    Serial.println(nmae);

    const SensorData& data = sensors.Data(primaryIndex);
    // Just forget about pm_1_0 and pm_10_0 for now
    sample.Set(sensors.Timestamp(), 0.0, avg, 0.0, data.pressure, data.temperature,
               data.humidity, count, mae);
  }

  return count;
}

#endif

#endif
