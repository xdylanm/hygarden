#ifndef soilmoistureprobegroup_h_
#define soilmoistureprobegroup_h_

#include <Arduino.h>

class SoilMoistureProbeGroup
{
  public:
    static int const MAX_PROBE_COUNT; 
    enum ReduceOp { NONE, AVERAGE, MIN, MAX, NUM_OPS };
  
    SoilMoistureProbeGroup(int probe_count, ReduceOp op = AVERAGE);

    void set_pins(int pin_s0, int pin_s1, int pin_s2, int pin_nen, int pin_ain);
    
    void set_enabled(bool en);
    bool get_enabled() const;

    void set_probe_count(int count);
    int get_probe_count() const;

    void set_op(int op);
    void set_op(ReduceOp op);
    ReduceOp get_op() const;

    void set_selected_probe(int iprobe);
    int get_selected_probe() const;

    float read() const;

  private:
    void select(int iprobe) const;

    int probe_count_;

    ReduceOp op_;
    int selected_probe_;
    
    int pin_s0_;
    int pin_s1_;
    int pin_s2_;
    int pin_nen_;
    int pin_ain_;
    bool en_;


};


#endif // soilmoistureprobegroup_h_
