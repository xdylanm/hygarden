#include "dayschedule.h"

DaySchedule::DaySchedule(uint32_t t_midnight) : t_zero_(t_midnight), enabled_(false)
{
    for (int i = 0; i < 4; ++i) {
        clear_interval(i);
    }
}

bool DaySchedule::active() const
{
    if (!enabled()) {
        return false;
    }

    auto const rel_now = ms_day_time();
    //Serial.print("relative now in ms: ");
    //Serial.println(rel_now);
    
    if (rel_now < MS_PER_24H) { // not expired
        for (int i = 0; i < 4; ++i) {
            //Serial.print("trange "); 
            //Serial.print(i);
            //Serial.print(": ");
            //Serial.print(t_start_[i]);
            //Serial.print(" to ");
            //Serial.println(t_end_[i]);
            
            if (t_start_[i] != t_end_[i]) {
                if ((rel_now > t_start_[i]) && (rel_now <= t_end_[i])) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool DaySchedule::expired() const
{
    return ms_day_time() > MS_PER_24H;
}

void DaySchedule::reset_t_zero(uint32_t t0)
{
    t_zero_ = t0;
}

// relative time of day in ms from t_zero_
uint32_t DaySchedule::ms_day_time() const
{
    uint32_t const now = millis();
    if (now > t_zero_) {
        return now - t_zero_;
    }
    return uint32_t(-1) - (t_zero_ - now);
}

void DaySchedule::set_interval(int i, uint32_t start, uint32_t stop)
{
    if ((start > stop) || (i < 0) || (i > 3)) {
        return;
    }
    t_start_[i] = start;
    t_end_[i] = stop;
}
void DaySchedule::clear_interval(int i)
{
    if ((i < 0) || (i > 3)) {
        return;
    }
    t_start_[i] = 0;
    t_end_[i] = 0;
}

void DaySchedule::serialize(JsonObject& obj) const 
{
    obj["enabled"] = enabled_;
    auto start_mins = obj.createNestedArray("start");
    auto stop_mins = obj.createNestedArray("stop");
    for (int i = 0; i < 4; ++i) {
        start_mins.add(t_start_[i]/60000);
        stop_mins.add(t_end_[i]/60000);
    }
}

void DaySchedule::unserialize(JsonObject const& obj) 
{
    if (obj.containsKey("enabled")) {
      enabled_ = obj["enabled"].as<bool>();
    }
    JsonArray start_mins = obj["start"].as<JsonArray>();
    if (start_mins.size() == 4) {
        for (int i = 0; i < 4; ++i) {
            int mins_i = start_mins[i].as<int>();
            if (mins_i >= 0) {  // only update for vals >= 0
                t_start_[i] = mins_i*60000;  // to ms
            }
        }
    }
    JsonArray stop_mins = obj["stop"].as<JsonArray>();
    if (stop_mins.size() == 4) {
        for (int i = 0; i < 4; ++i) {
            int mins_i = stop_mins[i].as<int>();
            if (mins_i >= 0) {  // only update for vals >= 0
                t_end_[i] = mins_i*60000;  // to ms
            }
        }
    }

    for (int i = 0; i < 4; ++i) {
      if (t_start_[i] >= t_end_[i]) {
        t_start_[i] = 0;
        t_end_[i] = 0;
      }
    }
}
