#ifndef MHZ19_SENSOR_H
#define MHZ19_SENSOR_H

#include <MHZCO2.h>

#include "abstract_sensor.h"

// Delay in milliseconds for the sensor init process.
#define MHZ19_INIT_DELAY_MS 300

class MHZ19_Sensor : public AbstractSensor {
  public:
  bool on_init() override;
  bool on_measure() override;

  /*
   * Returns the last measured CO2.
   */
  int get_co2() const { return _co2; }

  private:
  MHZ19B _mhz;
  int    _co2 = 0;
};

/*
 * Global instance of MHZ19_Sensor.
 */
extern MHZ19_Sensor MHZ19Sensor;

#endif // MHZ19_SENSOR_H
