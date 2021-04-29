#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

#include "sensorsmanager.h"
#include "hygardenconfig.h"

#define SOLENOID_GPIO 0

// Global objects
Adafruit_BME280 bme_sensor;
SensorsManager monitor(5000,20000,bme_sensor,A0);
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
StaticJsonDocument<MAX_BUF_LEN> json_config;
StaticJsonDocument<MAX_BUF_LEN> json_data;
StaticJsonDocument<MAX_BUF_LEN> json_input;
char buffer[MAX_BUF_LEN];
int msg_pos;

void setup() {
  pinMode(SOLENOID_GPIO, OUTPUT);
  pinMode(BUILTIN_LED, OUTPUT);
  
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
    DeserializationError error = deserializeJson(json_config, hf);
    hf.close();
    if (error) {
      Serial.print(F("Config load failed: "));
      Serial.println(error.f_str());
    } else {
      JsonObject config_obj = json_config.as<JsonObject>();
      config.unserialize(config_obj);

      // echo
      Serial.println(F("Loaded config"));
      StaticJsonDocument<512> echo_doc;
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
      json_input.clear();
      DeserializationError error = deserializeJson(json_input, buffer, msg_pos);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
      } else {
        //JsonObject obj = json_input.as<JsonObject>();
        if (auto mode = json_input["solenoid_mode"].as<const char*>()) {
          Serial.print("Solenoid: ");
          Serial.println(mode);
          config.solenoid.mode = HyGardenConfig::solenoidModeFromText(mode);
          if (config.solenoid.mode < 2) {
            config.solenoid.state = config.solenoid.mode;
          }
        }
        if (auto threshold = json_input["threshold"].as<float>()) {
          Serial.print("Threshold: ");
          Serial.println(threshold);
          config.soil_moisture.threshold = threshold;
          
        }
       
      }

      //Reset for the next message
      msg_pos= 0;
    }
  }

  digitalWrite(SOLENOID_GPIO, config.solenoid.state);
  delay(500);
  
}
