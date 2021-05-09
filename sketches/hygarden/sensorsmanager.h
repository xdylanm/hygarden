#ifndef sensorsmanager_h_
#define sensorsmanager_h_

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "soilmoistureprobegroup.h"


struct SensorsData {
  SensorsData(); 
  void reset();
  void average(uint32_t count);

  float temperature_;
  float pressure_;
  float humidity_;
  float moisture_;
};

class SensorsManager
{
public:
  SensorsManager(uint32_t sample_interval, uint32_t report_interval);

  void reset();
  bool probe();
  SensorsData const& report() const { return report_data_; }

  void set_sample_interval(uint32_t T);
  void set_report_interval(uint32_t T);

  Adafruit_BME280& bme_sensor() { return bme_; }
  SoilMoistureProbeGroup& soil_moisture_group() { return soil_moisture_group_; }
  
private:
  
  struct Interval {
    Interval(uint32_t dt) : dt_(dt), start_(0) {}
    uint32_t dt_;     // span of interval (ms)
    uint32_t start_;  // start of interval (ms)
  };

  Interval sample_interval_; 
  Interval report_interval_; 
  uint32_t sample_count_;     // number of samples collected in this interval

  SensorsData sample_data_;
  SensorsData report_data_;
 
  bool interval_elapsed(Interval& i);

  Adafruit_BME280 bme_;   
  SoilMoistureProbeGroup soil_moisture_group_;
}; 


#endif // sensorsmanager_h_
