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
  uint32_t report_interval) :
  sample_interval_(sample_interval), report_interval_(report_interval),
  sample_count_(0), soil_moisture_group_(1)
{
  reset();    
}

bool SensorsManager::probe() 
{
  if (interval_elapsed(sample_interval_)) {
    sample_data_.temperature_ += bme_.readTemperature();
    sample_data_.pressure_ += bme_.readPressure() / 100.0F;
    sample_data_.humidity_ += bme_.readHumidity();
    sample_data_.moisture_ += 1.0F - soil_moisture_group_.read(); // invert: more capacitance = more moisture
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
  uint32_t tnow = millis();
  uint32_t elapsed = 0;
  if (tnow < i.start_) {  // clock rolled over
    elapsed = std::numeric_limits<uint32_t>::max() - i.start_;
    elapsed += tnow + 1;
  } else {
    elapsed = tnow - i.start_;
  }
  
  if (elapsed >= i.dt_) {
    i.start_ += i.dt_;   // ok to roll over
    if (i.start_ < tnow) {
      i.start_ = tnow;
    }
    return true;
  } 
  return false;
}
