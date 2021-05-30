#ifndef dayschedule_h_
#define dayschedule_h_

#include <ArduinoJson.h>

class DaySchedule
{
public:

    static uint32_t const MS_PER_24H = 86400000; //24*3600*1000

    DaySchedule(uint32_t t_midnight = 0);

    bool active() const;

    bool expired() const;
    void reset_t_zero(uint32_t t0);

    void enabled(bool en) { enabled_ = en; }
    bool enabled() const { return enabled_; }

    // relative time of day in ms from t_zero_
    uint32_t ms_day_time() const;

    void set_interval(int i, uint32_t start, uint32_t stop);
    void clear_interval(int i);

    void serialize(JsonObject& obj) const;
    void unserialize(JsonObject const& obj);

private:

    uint32_t t_zero_;
    uint32_t t_start_[4];
    uint32_t t_end_[4];
    bool enabled_;  
};

#endif // dayschedule_h_