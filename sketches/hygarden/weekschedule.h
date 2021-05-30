#ifndef weekschedule_h_
#define weekschedule_h_

#include "dayschedule.h"
#include <ArduinoJson.h>

class WeekSchedule
{
public:
    WeekSchedule();

    void update_clock(time_t now);      // how to signal an update & refresh
    bool active();    
    bool last_expired() const { return last_expired_; }

    const char* day_label(int i) const;

    DaySchedule&       day(int i);
    DaySchedule const& day(int i) const;

    void serialize(JsonObject& obj) const;
    void unserialize(JsonObject const& obj);
private:
    DaySchedule daily_[7];
    int current_day_;
    bool last_expired_;
    int tz_offset_;
}; 

#endif //weekschedule_h_
