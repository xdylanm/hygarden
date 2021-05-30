#include "weekschedule.h"

WeekSchedule::WeekSchedule() : current_day_(0), last_expired_(true), tz_offset_(0)
{

}

void WeekSchedule::update_clock(time_t now) 
{
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);  // no local time here

    Serial.print("Updating schedule clock: ");
    Serial.println(asctime(&timeinfo));

    int const mins_per_24h = 60*24;
    int day_offset = 0;
    int mins_since_midnight = timeinfo.tm_min + 60*timeinfo.tm_hour + tz_offset_;
    if (mins_since_midnight < 0) {
      day_offset = -1;
      mins_since_midnight += mins_per_24h;
    } else if (mins_since_midnight > mins_per_24h) {
      day_offset = 1;
      mins_since_midnight -= mins_per_24h;
    }

    uint32_t const ms_since_midnight = 1000*(timeinfo.tm_sec + 60*mins_since_midnight);
    uint32_t const ms_now = millis();
    uint32_t const ms_zero = ms_now > ms_since_midnight 
        ? ms_now - ms_since_midnight : uint32_t(-1) - (ms_since_midnight - ms_now);

    Serial.print("ms_now=");
    Serial.println(ms_now);
    Serial.print("ms_zero=");
    Serial.println(ms_zero);

    int day_ndx = timeinfo.tm_wday + day_offset;
    current_day_ = day_ndx < 0 ? (day_ndx + 7) : (day_ndx % 7);

    Serial.print("current_day_=");
    Serial.println(current_day_);
    
    for (int iday = 0; iday < 7; ++iday) {
        Serial.print("setting day ");
        Serial.print((current_day_ + iday) % 7);
        Serial.print(" t0=");
        Serial.println(ms_zero + iday*DaySchedule::MS_PER_24H);
        
        daily_[(current_day_ + iday) % 7].reset_t_zero(ms_zero + iday*DaySchedule::MS_PER_24H);
    }
    last_expired_ = false;
}

bool WeekSchedule::active()
{
    //Serial.println("Checking schedule..."); 
    for (int iday = 0; iday < 7; ++iday) {
        //Serial.print("Checking day ");
        //Serial.println(current_day_);
        if (daily_[current_day_].expired()) {
            current_day_ = (current_day_ + 1) % 7;
            last_expired_ = true;
        } else { 
            break;
        }
    }

    return daily_[current_day_].active();
}

const char* WeekSchedule::day_label(int i) const {
    static char const* DAYS = "Su\0Mo\0Tu\0We\0Th\0Fr\0Sa\0";
    if (i >= 0 && i < 7) {
        return &DAYS[3*i];
    }
    return &DAYS[20];
}

DaySchedule& WeekSchedule::day(int i) 
{
    return daily_[i % 7];
}

DaySchedule const& WeekSchedule::day(int i) const 
{
    return daily_[i % 7];
}

void WeekSchedule::serialize(JsonObject& obj) const
{
    obj["tz_offset"] = tz_offset_;
    for (int i = 0; i < 7; ++i) {
        const char* lbl = day_label(i);
        auto day_obj = obj.createNestedObject(lbl);
        daily_[i].serialize(day_obj);
    }
}

void WeekSchedule::unserialize(JsonObject const& obj) 
{
    if (obj.containsKey("tz_offset")) {
      tz_offset_ = obj["tz_offset"].as<int>();
      last_expired_ = true; // trigger a refresh
    }
    for (int i = 0; i < 7; ++i) {
        const char* lbl = day_label(i);
        if (obj.containsKey(lbl)) {
            daily_[i].unserialize(obj[lbl].as<JsonObject>());
        }
    }
}
