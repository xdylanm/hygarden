#include <LittleFS.h>

#include "sensorsmanager.h"
#include "hygardenconfig.h"
#include "mqttclientmanager.h"
#include "messagemanager.h"

#define VERSION_MAJOR 0
#define VERSION_MINOR 4

#define SOLENOID_GPIO 0
#define AMUX_EN_GPIO 14
#define AMUX_S0_GPIO 15
#define AMUX_S1_GPIO 13
#define AMUX_S2_GPIO 12

// Global objects
SensorsManager monitor(5000,20000);
HyGardenConfig config;
MessageManager msg;

void load_config() 
{
  File hf = LittleFS.open("/config.json","r");
  if (!hf) {
    Serial.println(F("Failed to load config file for read, config reset"));
    return;
  }
  
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
    MessageManager::updateSensorsMonitor(config, monitor);

    // echo
    Serial.println(F("Loaded config"));
    StaticJsonDocument<MAX_BUF_LEN> echo_doc;
    JsonObject root = echo_doc.to<JsonObject>(); 
    config.serialize(root);
    serializeJsonPretty(root,Serial);
    Serial.println();
  }
  
}

void setup() 
{
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

  auto conn = MqttClientManager::instance();

  // blinky 2s, ON at end
  conn->blink_builtin(160,40,10);
  
  delay(500);
  Serial.println();
  Serial.print("Hygarden v");
  Serial.print(VERSION_MAJOR);
  Serial.print(".");
  Serial.println(VERSION_MINOR);
  Serial.print("- MQTT max packet length is ");
  Serial.println(MAXBUFFERSIZE);

  load_config();

  monitor.soil_moisture_group().set_pins(AMUX_S0_GPIO,AMUX_S1_GPIO,AMUX_S2_GPIO,AMUX_EN_GPIO,A0);
  config.bme.connected = monitor.bme_sensor().begin(0x76);
  delay(10);

  if (!conn->wifi_connect()) {
    Serial.println("Failed to connect to network");
    return;
  }
  
  msg.initialize(conn->mqtt_client());

  monitor.reset();
 
}


void loop() 
{
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  
  MqttClientManager::instance()->mqtt_connect();

  // check for messages via subscriptions. Timeout through config sample interval
  msg.poll_subscriptions(config, monitor);

  if (config.active) {
    if (monitor.probe()) {
      msg.publish_sensor_data(config, monitor);
      if (config.solenoid.mode == 2) { // auto
        config.solenoid.state = (monitor.report().moisture_ < config.soil_moisture.threshold);
      }
    }
  }

  msg.keep_alive();

  if (config.active) {
    digitalWrite(SOLENOID_GPIO, config.solenoid.state);
  } else {
    digitalWrite(SOLENOID_GPIO, LOW);
  }
  delay(500);
  
}
