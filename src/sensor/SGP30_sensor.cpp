#include "SGP30_sensor.h"

#include <Adafruit_SGP30.h>
#include <Arduino.h>
#include <WString.h>
#include <stdint.h>

#include "../config.h"
#include "../log.h"
#include "DHT11_sensor.h"

SGP30_Sensor SGP30Sensor;

bool SGP30_Sensor::on_init() {
  if (!_sgp.begin()) {
    app_errorln("SGP30_Sensor::init: sensor not found");
    return false;
  }
  delay(SGP30_INIT_DELAY_MS);
  app_traceln("SGP30_Sensor::init: sensor initialized");
  return true;
}

bool SGP30_Sensor::on_measure() {
  uint16_t currentTvoc = 0.0, currentEco2 = 0.0, minTvoc = 0.0, maxTvoc = 0.0, minEco2 = 0.0,
           maxEco2 = 0.0;

  app_traceln("SGP30_Sensor::measure: reading sensor values");
  if (DHT11Sensor.get_temperature() > 0) {
    app_traceln(
        "SGP30_Sensor::measure: compute absolute humidity with real temperature and humidity");
    _sgp.setHumidity(
        get_absolute_humidity(DHT11Sensor.get_temperature(), DHT11Sensor.get_humidity()));
  } else {
    app_traceln("SGP30_Sensor::measure: compute absolute humidity with default temperature and "
                "humidity");
    _sgp.setHumidity(get_absolute_humidity(22.1, 45.2));
  }

  for (int i = 0; i < SENSOR_AVG_WINDOW; ++i) {
    _sgp.IAQmeasure();
    currentTvoc = _sgp.TVOC;
    currentEco2 = _sgp.eCO2;

    if (currentTvoc < minTvoc) {
      minTvoc = currentTvoc;
    }
    if (currentTvoc > maxTvoc) {
      maxTvoc = currentTvoc;
    }
    if (currentEco2 < minEco2) {
      minEco2 = currentEco2;
    }
    if (currentEco2 > maxEco2) {
      maxEco2 = currentEco2;
    }

    _tvoc += currentTvoc;
    _eco2 += currentEco2;
    delay(SGP30_MEASURE_DELAY_MS);
  }
  _tvoc -= minTvoc;
  _tvoc -= maxTvoc;
  _eco2 -= minEco2;
  _eco2 -= maxEco2;

  _tvoc /= (SENSOR_AVG_WINDOW - 2);
  _eco2 /= (SENSOR_AVG_WINDOW - 2);

#ifdef PRINT_SENSORS_ON_READ
  app_infoln("SGP30 TVOC: " + String(_tvoc));
  app_infoln("SGP30 eCO2: " + String(_eco2));
#endif // PRINT_SENSORS_ON_READ

  return true;
}

uint32_t SGP30_Sensor::get_absolute_humidity(float temperature, float humidity) {
  const float absoluteHumidity
      = 216.7f
        * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature))
           / (273.15f + temperature));
  const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity);
  return absoluteHumidityScaled;
}
