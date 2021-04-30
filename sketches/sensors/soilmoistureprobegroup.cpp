#include "soilmoistureprobegroup.h"


int const SoilMoistureProbeGroup::MAX_PROBE_COUNT = 8;

SoilMoistureProbeGroup::SoilMoistureProbeGroup(
    int probe_count, 
    int pin_s0, 
    int pin_s1,
    int pin_s2, 
    int pin_nen, 
    int pin_ain)
  : probe_count_(0), 
    pin_s0_(pin_s0), 
    pin_s1_(pin_s1),
    pin_s2_(pin_s2),
    pin_nen_(pin_nen),
    pin_ain_(pin_ain),
    en_(false)
{
  pinMode(pin_nen_, OUTPUT);
  pinMode(pin_s0_, OUTPUT);
  pinMode(pin_s1_, OUTPUT);
  pinMode(pin_s2_, OUTPUT);
  set_enabled(en_);
  set_probe_count(probe_count_);
}

void SoilMoistureProbeGroup::set_enabled(bool en) 
{
  en_ = en;
  digitalWrite(pin_nen_, !en_);
}

bool SoilMoistureProbeGroup::get_enabled() const 
{
  return en_;
}

void SoilMoistureProbeGroup::set_probe_count(int const count) 
{
  probe_count_ = (count > MAX_PROBE_COUNT) ? MAX_PROBE_COUNT : count;
}

int SoilMoistureProbeGroup::get_probe_count() const 
{
  return probe_count_;
}

float SoilMoistureProbeGroup::read(int const iprobe /*= -1*/) const 
{
  if (!en_ || (iprobe > probe_count_)) {
    return 0.F;
  }

  // select single probe
  if (iprobe >= 0) {
    select(iprobe);
    delay(10);
    return float(analogRead(pin_ain_))/1023.0F;
  }  
  // read and average all probes
  float avg = 0;
  float scaling = 1023*probe_count_;  // include weighting for average
  for (int i = 0; i < probe_count_; ++i ) {
    select(i);
    delay(10);
    avg += float(analogRead(pin_ain_))/scaling;
  }
  return avg;

}

void SoilMoistureProbeGroup::select(int const iprobe) const
{
  //Serial.print("Select probe ");
  //Serial.print(iprobe);
  //Serial.print(" ");
  //Serial.print((iprobe & 0x0001) == 0 ? "0" : "1");
  //Serial.print((iprobe & 0x0002) == 0 ? "0" : "1");
  //Serial.print((iprobe & 0x0004) == 0 ? "0" : "1");
  //Serial.println();
  
  digitalWrite(pin_s0_, static_cast<bool>(iprobe & 0x01));
  digitalWrite(pin_s1_, static_cast<bool>(iprobe & 0x02));
  digitalWrite(pin_s2_, static_cast<bool>(iprobe & 0x04));
}
