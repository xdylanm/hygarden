#include "hygardenconfig.h"
#include <stdlib.h>

HyGardenConfig::HyGardenConfig() 
  : topic("/uninitialized")
{
  for (int i = 0; i < 16; ++i) {
    uuid[i] = 0x00;
  }
}


int HyGardenConfig::solenoidModeFromText(char const* mode) {
  if (strcmp(mode, "off") == 0) {
    return 0;
  } else if (strcmp(mode, "on") == 0) {
    return 1;
  } else if (strcmp(mode, "auto") == 0) {
    return 2;
  }
  return -1;
}

String HyGardenConfig::solenoidModeToText(int const mode) {
  if (mode == 0) {
    return String("off");
  } else if (mode == 1) {
    return String("on");
  } else if (mode == 2) {
    return String("auto");
  }
  return String("off");
}

namespace {
uint8_t charPairToByte(char const* ch)
{
  static char byte_buf[3];
  byte_buf[0] = *ch++;
  byte_buf[1] = *ch;
  byte_buf[2] = '\0';
  return strtoul(byte_buf,nullptr,16);
}
}

void HyGardenConfig::uuidFromText(char const* txt, uint8_t* uuid) 
{
  int ich = 0;
  int range[] = {0,4,6,8,10,16};
  for (int iblk = 0; iblk < 5; ++iblk) {
    for (int ibyte = range[iblk]; ibyte < range[iblk+1]; ++ibyte) {
      uuid[ibyte] = charPairToByte(&txt[ich]);
      ich += 2;
    }
    ich++; // skip -
  }
}

String HyGardenConfig::uuidToText(uint8_t const* uuid) 
{
  int ich = 0;
  char msg[37];
  int range[] = {0,4,6,8,10,16};
  for (int iblk = 0; iblk < 5; ++iblk) {
    for (int ibyte = range[iblk]; ibyte < range[iblk+1]; ++ibyte) {
      uint8_t nibble = (uuid[ibyte] & 0xF0) >> 4;
      msg[ich++] = nibble < 0x0A ? '0'+nibble : nibble - 0x0A + 'a';
      nibble = (uuid[ibyte] & 0x0F);
      msg[ich++] = nibble < 0x0A ? '0'+nibble : nibble - 0x0A + 'a';
    }
    msg[ich++] = '-'; 
  }
  msg[36] = '\0';
  return String(msg);
}

void HyGardenConfig::serialize(JsonObject& obj) 
{
  obj.clear();
  obj["topic"] = topic;
  auto uuidTxt = uuidToText(uuid);
  obj["uuid"] = uuidTxt;
  
  // sensors
  auto sensors_obj = obj.createNestedObject("sensors");
  auto bme_obj = sensors_obj.createNestedObject("BME280");
  bme_obj["enabled"] = bme.enabled;
  bme_obj["count"] = 1;
  auto soil_obj = sensors_obj.createNestedObject("SoilMoisture");
  soil_obj["count"] = soil_moisture.count;
  soil_obj["enabled"] = soil_moisture.enabled;
  soil_obj["threshold"] = soil_moisture.threshold;
  
  // solenoid
  auto solenoid_obj = obj.createNestedObject("solenoid");
  solenoid_obj["mode"] = solenoidModeToText(solenoid.mode);
  solenoid_obj["min_on"] = solenoid.min_on;
  solenoid_obj["max_on"] = solenoid.max_on;

  // interval
  auto interval_obj = obj.createNestedObject("interval");
  interval_obj["sample"] = interval.sample;
  interval_obj["report"] = interval.report;
  
}

void HyGardenConfig::unserialize(JsonObject const& obj) 
{
  if (auto in_topic = obj["topic"].as<char const*>()) {
    topic = String(in_topic);
  }
  if (auto in_id = obj["uuid"].as<char const*>()) {
    uuidFromText(in_id, uuid);
  }
  // sensors
  if (obj["sensors"]["BME280"].containsKey("enabled")) {
    bme.enabled = obj["sensors"]["BME280"]["enabled"].as<bool>();
  }
  JsonObject sm_obj = obj["sensors"]["SoilMoisture"];
  if (!sm_obj.isNull()) {
    if (sm_obj.containsKey("count")) {
      int sm_count = sm_obj["count"].as<int>();
      soil_moisture.count = (sm_count <= MAX_SOIL_MOISTURE_PROBES ? sm_count : MAX_SOIL_MOISTURE_PROBES);
    }
    if (sm_obj.containsKey("enabled")) {
      soil_moisture.count = sm_obj["enabled"].as<bool>();
    }
    if (sm_obj.containsKey("threshold")) {
      soil_moisture.threshold = sm_obj["threshold"].as<float>(); 
    }
  }
  // solenoid
  JsonObject sol_obj = obj["solenoid"];
  if (!sol_obj.isNull()) {
    if (auto mode = sol_obj["mode"].as<char const*>()) {
      solenoid.mode = solenoidModeFromText(mode);
    }
    if (sol_obj.containsKey("min_on")) {
      solenoid.min_on = sol_obj["min_on"].as<uint32_t>();
    }
    if (sol_obj.containsKey("max_on")) {
      solenoid.max_on = sol_obj["max_on"].as<uint32_t>();
    }
  }
  // Intervals
  JsonObject iobj = obj["interval"];
  if (!iobj.isNull()) {
    if (iobj.containsKey("sample")) {
      interval.sample = iobj["sample"].as<uint32_t>();
    }
    if (iobj.containsKey("report")) {
      interval.report = iobj["report"].as<uint32_t>();
    }
  }
  

}
