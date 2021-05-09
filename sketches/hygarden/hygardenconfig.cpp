#include "hygardenconfig.h"
#include <stdlib.h>

HyGardenConfig::HyGardenConfig() 
  : topic("/uninitialized"), active(false)
{
  for (int i = 0; i < 16; ++i) {
    uuid[i] = 0x00;
  }
}


HyGardenConfig::HyGardenConfig(HyGardenConfig const& o) 
  : topic(o.topic), active(o.active), bme(o.bme), 
    soil_moisture(o.soil_moisture), solenoid(o.solenoid), interval(o.interval)
{
  for (int i = 0; i < 16; ++i) {
    uuid[i] = o.uuid[i];
  }
}


HyGardenConfig& HyGardenConfig::operator=(HyGardenConfig const& o) 
{
  if (this == &o) {
    return *this;
  }
  topic = o.topic;
  for (int i = 0; i < 16; ++i) {
    uuid[i] = o.uuid[i];
  }
  active = o.active;
  bme = o.bme;
  soil_moisture = o.soil_moisture;
  solenoid = o.solenoid;
  interval = o.interval;
}

int HyGardenConfig::solenoidModeFromText(char const* mode) 
{
  if (strcmp(mode, "off") == 0) {
    return 0;
  } else if (strcmp(mode, "on") == 0) {
    return 1;
  } else if (strcmp(mode, "auto") == 0) {
    return 2;
  }
  return -1;
}

String HyGardenConfig::solenoidModeToText(int const mode) 
{
  if (mode == 0) {
    return String("off");
  } else if (mode == 1) {
    return String("on");
  } else if (mode == 2) {
    return String("auto");
  }
  return String("off");
}

int HyGardenConfig::moistureProbeOpFromText(char const* op)
{
  if (strcmp(op, "none") == 0) {
    return 0;
  } else if (strcmp(op, "avg") == 0) {
    return 1;
  } else if (strcmp(op, "min") == 0) {
    return 2;
  } else if (strcmp(op, "max") == 0) {
    return 3;
  }
  return -1;
}

String HyGardenConfig::moistureProbeOpToText(int const op)
{
  switch (op) {
  case 0:
    return String("none");
  case 1:
    return String("avg");
  case 2:
    return String("min");
  case 3:
    return String("max");
  default:
    break;
  }
  return String("none");
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

void HyGardenConfig::serialize(JsonObject& obj, bool const include_state /*=false*/) 
{
  obj["topic"] = topic;
  auto uuidTxt = uuidToText(uuid);
  obj["uuid"] = uuidTxt;
  obj["active"] = active;
  
  // sensors
  auto sensors_obj = obj.createNestedObject("sensors");
  auto bme_obj = sensors_obj.createNestedObject("BME280");
  bme_obj["enabled"] = bme.enabled;
  bme_obj["count"] = 1;
  if (include_state) {
    bme_obj["connected"] = bme.connected;
  }
  auto soil_obj = sensors_obj.createNestedObject("SoilMoisture");
  soil_obj["count"] = soil_moisture.count;
  soil_obj["enabled"] = soil_moisture.enabled;
  soil_obj["threshold"] = soil_moisture.threshold;
  soil_obj["operation"] = moistureProbeOpToText(soil_moisture.op);
  soil_obj["select"] = soil_moisture.selected_probe;
  
  // solenoid
  auto solenoid_obj = obj.createNestedObject("solenoid");
  solenoid_obj["mode"] = solenoidModeToText(solenoid.mode);
  solenoid_obj["min_on"] = solenoid.min_on;
  solenoid_obj["max_on"] = solenoid.max_on;
  if (include_state) {
    solenoid_obj["state"] = solenoid.state;
  }

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
  if (obj.containsKey("active")) {
    active = obj["active"].as<bool>();
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
      soil_moisture.enabled = sm_obj["enabled"].as<bool>();
    }
    if (sm_obj.containsKey("threshold")) {
      float const t = sm_obj["threshold"].as<float>(); 
      if (t >= 0.0F && t <= 1.0F) {
        soil_moisture.threshold = t;
      }
    }
    if (auto op = sm_obj["operation"].as<char const*>()) {
      int const iop = moistureProbeOpFromText(op);
      if (op >= 0) {
        soil_moisture.op = iop;
      }
    }
    if (sm_obj.containsKey("select")) {
      int const sel = sm_obj["select"];
      if (sel >= 0 && sel < soil_moisture.count) {
        soil_moisture.selected_probe = sel;
      }
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
