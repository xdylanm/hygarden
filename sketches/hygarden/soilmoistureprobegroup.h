#ifndef soilmoistureprobegroup_h_
#define soilmoistureprobegroup_h_

#include <Arduino.h>

class SoilMoistureProbeGroup
{
  public:
    static int const MAX_PROBE_COUNT; 
  
    SoilMoistureProbeGroup(int probe_count);

    void set_pins(int pin_s0, int pin_s1, int pin_s2, int pin_nen, int pin_ain);
    
    void set_enabled(bool en);
    bool get_enabled() const;

    void set_probe_count(int count);
    int get_probe_count() const;

    float read(int iprobe = -1) const;
    

  private:
    void select(int iprobe) const;

    int probe_count_;
    int pin_s0_;
    int pin_s1_;
    int pin_s2_;
    int pin_nen_;
    int pin_ain_;
    bool en_;


};


#endif // soilmoistureprobegroup_h_
