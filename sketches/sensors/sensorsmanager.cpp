#include "sensorsmanager.h"
#include <limits>

SensorsData::SensorsData() { 
  reset();
}

void SensorsData::reset() {
  temperature_ = 0;
  pressure_ = 0;
  humidity_ = 0;
  moisture_ = 0;
}

void SensorsData::average(uint32_t count) {
  float const fcount = count;
  temperature_ /= fcount;
  pressure_ /= fcount; 
  humidity_ /= fcount;
  moisture_ /= fcount;
}

SensorsManager::SensorsManager(
  uint32_t sample_interval, 
  uint32_t report_interval,
  Adafruit_BME280& bme,
  SoilMoistureProbeGroup& soil_moisture_group) :
  sample_interval_(sample_interval), report_interval_(report_interval),
  sample_count_(0), bme_(bme), soil_moisture_group_(soil_moisture_group)
{
  reset();    
}

bool SensorsManager::probe() 
{
  if (interval_elapsed(sample_interval_)) {
    sample_data_.temperature_ += bme_.readTemperature();
    sample_data_.pressure_ += bme_.readPressure() / 100.0F;
    sample_data_.humidity_ += bme_.readHumidity();
    sample_data_.moisture_ += soil_moisture_group_.read();
    sample_count_++;
    //Serial.print("Sampled sensors ("); 
    //Serial.print(sample_count_);
    //Serial.println(")");
  }

  bool const report_ready = interval_elapsed(report_interval_);
  if (report_ready) {
    report_data_ = sample_data_;
    report_data_.average(sample_count_);
    
    sample_data_.reset();
    sample_count_ = 0;
  }
  return report_ready;
}

void SensorsManager::set_sample_interval(uint32_t dt) {
  sample_interval_.dt_ = dt;
  reset();
}
void SensorsManager::set_report_interval(uint32_t dt) {
  report_interval_.dt_ = dt;
  reset();
}
  
void SensorsManager::reset() {
  sample_interval_.start_ = millis();
  report_interval_.start_ = sample_interval_.start_;
  sample_count_ = 0;
  
  sample_data_.reset();
  report_data_.reset();
}

bool SensorsManager::interval_elapsed(Interval& i)
{
  uint32_t now = millis();
  uint32_t elapsed = 0;
  if (now < i.start_) {  // clock rolled over
    elapsed = std::numeric_limits<uint32_t>::max() - i.start_;
    elapsed += now + 1;
  } else {
    elapsed = now - i.start_;
  }
  
  if (elapsed >= i.dt_) {
    i.start_ += i.dt_;   // ok to roll over
    return true;
  } 
  return false;
}
