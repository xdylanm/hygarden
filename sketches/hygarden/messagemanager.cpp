#include "messagemanager.h"

#include "hygardenconfig.h"
#include "sensorsmanager.h"

#include <ArduinoJson.h>

#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <LittleFS.h>

struct MessageManagerImpl 
{
  MessageManagerImpl(Adafruit_MQTT_Client& mqtt_client) :
    mqtt_client_(mqtt_client),
    pub_sensor_data_(&mqtt_client_, "/environment"),
    pub_status_(&mqtt_client_, "/status"),
    sub_control_(&mqtt_client_, "/control",MQTT_QOS_1),
    sub_discovery_(&mqtt_client_, "/discovery",MQTT_QOS_1)
  {
    mqtt_client_.subscribe(&sub_control_);
    mqtt_client_.subscribe(&sub_discovery_);
  }
  
  Adafruit_MQTT_Client& mqtt_client_;
  
  Adafruit_MQTT_Publish pub_sensor_data_;
  Adafruit_MQTT_Publish pub_status_; 
  Adafruit_MQTT_Subscribe sub_control_;
  Adafruit_MQTT_Subscribe sub_discovery_;
};


MessageManager::MessageManager()
  : last_keep_alive_ms_(0), impl_(nullptr)
{
}

MessageManager::~MessageManager() 
{
  delete impl_;
}

void MessageManager::initialize(Adafruit_MQTT_Client& mqtt_client)
{
  if (impl_) {
    delete impl_;
  }
  impl_ = new MessageManagerImpl(mqtt_client);
  reset_keep_alive();
}

void MessageManager::poll_subscriptions(HyGardenConfig& config, SensorsManager& monitor) 
{
  if (!impl_) {
    return;
  }
  Adafruit_MQTT_Subscribe* sub = nullptr;
  uint32_t elapsed = 0;
  uint32_t tstart = millis();
  uint32_t const timeout = config.interval.sample*1000;
  while (elapsed < timeout) {
    if ((sub = impl_->mqtt_client_.readSubscription(timeout-elapsed))) {
      if (sub == &(impl_->sub_control_)) {
        on_control((char*)impl_->sub_control_.lastread, impl_->sub_control_.datalen, config, monitor);
      } else if (sub == &(impl_->sub_discovery_)) {
        on_discovery((char*)impl_->sub_discovery_.lastread, impl_->sub_discovery_.datalen, config);
      }
    }

    uint32_t tnow = millis();
    if (tnow < tstart) { // wrapped around
      elapsed += (uint32_t(-1) - tstart) + tnow;
    } else {
      elapsed += tnow - tstart;
    }
    tstart = tnow;
  }
}

void MessageManager::updateSensorsMonitor(
  HyGardenConfig const& config, 
  SensorsManager& monitor,
  HyGardenConfig const* old_config /* = nullptr */)
{
  if (!old_config || (old_config->interval.sample != config.interval.sample)) {
    monitor.set_sample_interval(config.interval.sample*1000);
  }
  if (!old_config || (old_config->interval.report != config.interval.report)) {
    monitor.set_report_interval(config.interval.report*1000);
  }

  monitor.soil_moisture_group().set_probe_count(config.soil_moisture.count);
  monitor.soil_moisture_group().set_enabled(config.soil_moisture.enabled);
  monitor.soil_moisture_group().set_op(config.soil_moisture.op);
  monitor.soil_moisture_group().set_selected_probe(config.soil_moisture.selected_probe);
}

void MessageManager::on_discovery(char* data, uint16_t const len, HyGardenConfig& config)
{
  //Serial.print("Discovery request: ");
  //Serial.println(data); 

  if (len < MAX_DISCOVERY_KEY_LEN) {
    StaticJsonDocument<MAX_BUF_LEN> json_discovery;
    json_discovery["discovery_key"].set(data);
    json_discovery["uuid"].set(HyGardenConfig::uuidToText(config.uuid));
    char output[MAX_BUF_LEN];
    serializeJson(json_discovery, output);
    
    //Serial.print("Discovery response: ");
    //Serial.println(output);

    if (!impl_->pub_status_.publish(output)) {
      Serial.println(F("Failed to publish discovery response"));
    } else {
      reset_keep_alive();
    }
  }
}

void MessageManager::on_control(
  char* data, 
  uint16_t const len, 
  HyGardenConfig& config, 
  SensorsManager& monitor)
{
  //Serial.println(data);
  if (len >= MAX_BUF_LEN) {
    Serial.println("Control sequence length exceeds buffer size.");
    return;
  }
  StaticJsonDocument<MAX_BUF_LEN> json_input;
  DeserializationError error = deserializeJson(json_input, data, len);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  } 

  if (auto uuid_string = json_input["destination_uuid"].as<const char*>()) {
    // validate UUID
    uint8_t target_uuid[16];
    HyGardenConfig::uuidFromText(uuid_string, target_uuid);

    if (!HyGardenConfig::compare_uuid(target_uuid, config.uuid)) {
      Serial.println("Failed to match UUID");
      return;
    }
    //Serial.println("Action matched UUID");
  } else {
    Serial.println("No UUID");
    return;
  }

  if (auto action = json_input["action"].as<const char*>()) {
    Serial.print("Action=");
    Serial.println(action);
    // parse action
    if (strcmp(action,"read") == 0) {
      StaticJsonDocument<MAX_BUF_LEN> json_output;
      JsonObject doc = json_output.to<JsonObject>();
      config.serialize(doc, true);  // include state
      
      char output[MAX_BUF_LEN];
      serializeJson(doc,output);
      
      //Serial.println(output);
      
      if (!impl_->pub_status_.publish(output)) {
        Serial.println(F("Failed to publish read response"));
      } else {
        reset_keep_alive();
      }
    } else if (strcmp(action, "write") == 0) {
      auto const old_config = config;
      config.unserialize(json_input.as<JsonObject>());
      if (config.solenoid.mode < 2) {
        config.solenoid.state = config.solenoid.mode;
      }
      updateSensorsMonitor(config, monitor, &old_config);

    } else if (strcmp(action, "store") == 0) {
      if (!LittleFS.begin()) {
        Serial.println("failed to mount storage for store action");
      }
      
      if (LittleFS.exists("/config.json")) {
        Serial.println("config file exists");
      } else {
        Serial.println("config file doesn't exist");
      }
      
      File hf = LittleFS.open("/config.json","w");
      if (!hf) {
        Serial.println(F("Failed to open config file for write, config not stored"));
      } else {
        StaticJsonDocument<MAX_BUF_LEN> json_output;
        JsonObject doc = json_output.to<JsonObject>();
        config.serialize(doc, false);  // don't include state
        serializeJson(doc,hf);
        hf.close();
        Serial.println(F("Updated config.json"));
      }
    } 
    
  }
}

void MessageManager::publish_sensor_data(
    HyGardenConfig const& config, 
    SensorsManager const& monitor)
{
  if (!impl_) {
    return;
  }
  StaticJsonDocument<MAX_BUF_LEN> json_data;
  json_data["uuid"] = HyGardenConfig::uuidToText(config.uuid);
  json_data["temperature"] = monitor.report().temperature_;
  json_data["pressure"] = monitor.report().pressure_;
  json_data["humidity"] = monitor.report().humidity_;
  json_data["moisture"] = monitor.report().moisture_;

  char output[MAX_BUF_LEN];
  serializeJson(json_data, output);

  if (!impl_->pub_sensor_data_.publish(output)) {
    Serial.println(F("Failed to publish sensor data"));
  } else {
    reset_keep_alive();
  }

}

void MessageManager::keep_alive()
{ 
  if (!impl_) {
    return;
  }
  uint32_t const elapsed_time = get_elapsed_time(); // seconds
  if (elapsed_time > (MQTT_CONN_KEEPALIVE/2)) {
    Serial.println("keep alive");
    if (!impl_->mqtt_client_.ping()) {
      impl_->mqtt_client_.disconnect();
    }
    reset_keep_alive();
  }
}

void MessageManager::reset_keep_alive()
{
  last_keep_alive_ms_ = millis();
}

uint32_t MessageManager::get_elapsed_time() const
{
  uint32_t elapsed_time = millis();
  if (elapsed_time > last_keep_alive_ms_) {  // no roll-over
    elapsed_time -= last_keep_alive_ms_;
  } else {
    elapsed_time = uint32_t(-1) - (last_keep_alive_ms_ - elapsed_time);
  }
  elapsed_time /= 1000; // to seconds
  return elapsed_time;
}
