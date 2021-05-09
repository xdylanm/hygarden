#include "soilmoistureprobegroup.h"


int const SoilMoistureProbeGroup::MAX_PROBE_COUNT = 8;

SoilMoistureProbeGroup::SoilMoistureProbeGroup(int probe_count, ReduceOp op /*= AVERAGE*/)
  : probe_count_(0),
    op_(op),
    selected_probe_(0), 
    pin_s0_(-1), 
    pin_s1_(-1),
    pin_s2_(-1),
    pin_nen_(-1),
    pin_ain_(-1),
    en_(false)
{
  set_probe_count(probe_count_);
}

void SoilMoistureProbeGroup::set_pins(int pin_s0, int pin_s1, int pin_s2, int pin_nen, int pin_ain)
{
  pin_s0_ = pin_s0;
  pin_s1_ = pin_s1;
  pin_s2_ = pin_s2;
  pin_nen_ = pin_nen;
  pin_ain_ = pin_ain;
  
  pinMode(pin_nen_, OUTPUT);
  pinMode(pin_s0_, OUTPUT);
  pinMode(pin_s1_, OUTPUT);
  pinMode(pin_s2_, OUTPUT);
  
  set_enabled(en_);
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

void SoilMoistureProbeGroup::set_selected_probe(int const sel) 
{
  if (sel >= 0 && sel < MAX_PROBE_COUNT) {
    selected_probe_ = sel;
  }
}

int SoilMoistureProbeGroup::get_selected_probe() const 
{
  return selected_probe_;
}

void SoilMoistureProbeGroup::set_op(int const op) 
{
  if (op >= 0 && op < int(NUM_OPS)) {
    set_op(ReduceOp(op));
  }
}

void SoilMoistureProbeGroup::set_op(ReduceOp op) 
{
  op_ = op;  
}
    
SoilMoistureProbeGroup::ReduceOp SoilMoistureProbeGroup::get_op() const 
{
  return op_;
}

float SoilMoistureProbeGroup::read() const 
{
  bool const selected_probe_in_range = (selected_probe_ < probe_count_) && (selected_probe_ >= 0);
  if (!en_ || ((op_ == NONE) && !selected_probe_in_range)) {
    return 0.F;
  }

  // select single probe
  if (op_ == NONE) {
    select(selected_probe_);
    delay(10);
    return float(analogRead(pin_ain_))/1023.0F;
  }  
  
  // read all probes
  float avg_val = 0;
  float min_val = 1.;
  float max_val = 0.;
  for (int i = 0; i < probe_count_; ++i ) {
    select(i);
    delay(10);
    float const a_val = analogRead(pin_ain_)/1023.0F;
    avg_val += a_val;
    min_val = min_val > a_val ? a_val : min_val;
    max_val = max_val < a_val ? a_val : max_val;
  }

  switch (op_) {
    case AVERAGE:
      return avg_val/float(probe_count_);
    case MIN:
      return min_val;
    case MAX:
      return max_val;
    default:
      break;
  }
  return 0.0F;

}

void SoilMoistureProbeGroup::select(int const iprobe) const
{
  digitalWrite(pin_s0_, static_cast<bool>(iprobe & 0x01));
  digitalWrite(pin_s1_, static_cast<bool>(iprobe & 0x02));
  digitalWrite(pin_s2_, static_cast<bool>(iprobe & 0x04));
}
