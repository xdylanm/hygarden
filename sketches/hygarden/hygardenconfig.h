#ifndef hygarden_config_h_
#define hygarden_config_h_

#include "weekschedule.h"

#include <Arduino.h>
#include <ArduinoJson.h>

#define MAX_SOIL_MOISTURE_PROBES 8 // analog mux has 8 channels
#define MAX_CONFIG_LEN 2048

struct HyGardenConfig 
{

  HyGardenConfig();
  HyGardenConfig(HyGardenConfig const& o);
  HyGardenConfig& operator=(HyGardenConfig const& rhs);
  
  static int solenoidModeFromText(char const* mode);
  static String solenoidModeToText(int const mode);

  static int moistureProbeOpFromText(char const* op);
  static String moistureProbeOpToText(int const op);

  static void uuidFromText(char const* txt, uint8_t* uuid);
  static String uuidToText(uint8_t const* uuid);
  static bool compare_uuid(uint8_t const* a, uint8_t const* b) {
    for (int i = 0; i < 16; ++i) {
      if (a[i] != b[i]) {
        return false;
      }
    }
    return true;
  }


  void serialize(JsonObject& obj, bool include_state=false);
  void unserialize(JsonObject const& obj);

  String topic;  
  uint8_t uuid[16];
  bool active;
  
  struct BME280Config {
    BME280Config() : enabled (true), connected(false) {}
    bool enabled;
    bool connected;
  } bme;

  struct SoilMoistureConfig {
    SoilMoistureConfig() : count(1), enabled(true), threshold(0.5), op(0), selected_probe(0) {}
    int count;
    bool enabled;
    float threshold;
    int op;
    int selected_probe;
  } soil_moisture;
  
  struct SolenoidConfig {
    SolenoidConfig() : mode(0), state(0), min_on(0), max_on(600) {}
    int mode;   //0-off, 1-on, 2-auto, 3-schedule
    int state;  //0-closed, 1-open
    uint32_t min_on;
    uint32_t max_on;
    WeekSchedule schedule;
  } solenoid;

  struct IntervalConfig {
    IntervalConfig() : sample(5), report(20) {}
    uint32_t sample;
    uint32_t report;
  } interval;

};

#endif // hygarden_config_h_
