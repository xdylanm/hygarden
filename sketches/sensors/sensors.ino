#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

#include "soilmoistureprobegroup.h"
#include "sensorsmanager.h"
#include "hygardenconfig.h"

#define SOLENOID_GPIO 0
#define AMUX_EN_GPIO 14
#define AMUX_S0_GPIO 15
#define AMUX_S1_GPIO 13
#define AMUX_S2_GPIO 12

// Global objects
SoilMoistureProbeGroup soil_probe_group (1,AMUX_S0_GPIO,AMUX_S1_GPIO,AMUX_S2_GPIO,AMUX_EN_GPIO,A0);
Adafruit_BME280 bme_sensor;
SensorsManager monitor(5000,20000,bme_sensor,soil_probe_group);
HyGardenConfig config;

// N times: off (T_low), on (T_high)
// finished state ON
void blinky(int T_high, int T_low, int N) {
  for (int i = 0; i < N; i++) {
    digitalWrite(BUILTIN_LED, HIGH);
    delay(T_low);
    digitalWrite(BUILTIN_LED, LOW);
    delay(T_high);
  }
}


#define MAX_BUF_LEN 512
char buffer[MAX_BUF_LEN];
int msg_pos;

void setup() {
  pinMode(SOLENOID_GPIO, OUTPUT);
  digitalWrite(SOLENOID_GPIO, LOW);
  
  pinMode(BUILTIN_LED, OUTPUT);
  
  pinMode(AMUX_EN_GPIO, OUTPUT);
  pinMode(AMUX_S0_GPIO, OUTPUT);
  pinMode(AMUX_S1_GPIO, OUTPUT);
  pinMode(AMUX_S2_GPIO, OUTPUT);
  
  Serial.begin(115200);
  delay(10);

  if (!LittleFS.begin()) {
    Serial.println("Error mounting LittleFS");
    return;
  }

  // blinky 2s, ON at end
  blinky(160,40,10);
  
  delay(500);
  Serial.println();
  Serial.println("Sensors test");
  
  config.bme.connected = bme_sensor.begin(0x76);
  delay(10);

  File hf = LittleFS.open("/config.json","r");
  if (!hf) {
    Serial.println(F("Failed to load config file for read, config reset"));
  } else {
    Serial.println("Loading config...");
    StaticJsonDocument<MAX_BUF_LEN> json_config;
    DeserializationError error = deserializeJson(json_config, hf);
    hf.close();
    if (error) {
      Serial.print(F("Config load failed: "));
      Serial.println(error.f_str());
    } else {
      JsonObject config_obj = json_config.as<JsonObject>();
      config.unserialize(config_obj);

      soil_probe_group.set_probe_count(config.soil_moisture.count);
      soil_probe_group.set_enabled(config.soil_moisture.enabled);

      // echo
      Serial.println(F("Loaded config"));
      StaticJsonDocument<MAX_BUF_LEN> echo_doc;
      JsonObject root = echo_doc.to<JsonObject>(); 
      config.serialize(root);
      serializeJsonPretty(root,Serial);
      Serial.println();
    }
  }
  
  monitor.reset();
  msg_pos = 0;
  
}

void loop() {
  if (config.bme.connected) {
    if (monitor.probe()) {
      // report ready
      StaticJsonDocument<200> json_data;
      json_data["temperature"] = monitor.report().temperature_;
      json_data["pressure"] = monitor.report().pressure_;
      json_data["humidity"] = monitor.report().humidity_;
      json_data["moisture"] = monitor.report().moisture_;
      serializeJson(json_data, Serial);
      Serial.println();
      if (config.solenoid.mode == 2) { // auto
        config.solenoid.state = (monitor.report().moisture_ > 0.54F);
      }
    }
  } else {
    Serial.println("No data. BME sensor not connected.");
  }
  
  while (Serial.available() > 0) {
    char next = Serial.read();
    //Message coming in (check not terminating character) and guard for over message size
    if ( next != '\n' && (msg_pos < MAX_BUF_LEN - 1) ) {
      buffer[msg_pos++] = next;
    } else {
      buffer[msg_pos] = '\0';
      //Print the message (or do other things)
      Serial.println(buffer);
      StaticJsonDocument<MAX_BUF_LEN> json_input;
      DeserializationError error = deserializeJson(json_input, buffer, msg_pos);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
      } else {
        if (auto action = json_input["action"].as<const char*>()) {
          if (strcmp(action,"read") == 0) {
            StaticJsonDocument<MAX_BUF_LEN> json_output;
            JsonObject doc = json_output.to<JsonObject>();
            config.serialize(doc, true);  // include state
            serializeJsonPretty(doc,Serial);
            Serial.println();
          } else if (strcmp(action, "write") == 0) {
            auto const old_interval = config.interval;
            config.unserialize(json_input.as<JsonObject>());
            if (config.solenoid.mode < 2) {
              config.solenoid.state = config.solenoid.mode;
            }
            if (old_interval.sample != config.interval.sample) {
              monitor.set_sample_interval(config.interval.sample*1000);
            }
            if (old_interval.report != config.interval.report) {
              monitor.set_report_interval(config.interval.report*1000);
            }
            soil_probe_group.set_probe_count(config.soil_moisture.count);
            soil_probe_group.set_enabled(config.soil_moisture.enabled);

          } else if (strcmp(action, "store") == 0) {
            File hf = LittleFS.open("/config.json","w");
            if (!hf) {
              Serial.println(F("Failed to open config file for write, config not stored"));
            } else {
              StaticJsonDocument<MAX_BUF_LEN> json_output;
              JsonObject doc = json_output.to<JsonObject>();
              config.serialize(doc, true);  // include state
              serializeJson(doc,hf);
              hf.close();
              Serial.println(F("Updated config.json"));
            }
          }
        }
      }

      //Reset for the next message
      msg_pos= 0;
    }
  }

  digitalWrite(SOLENOID_GPIO, config.solenoid.state);
  delay(500);
  
}
