#ifndef mqttclientmanager_h_
#define mqttclientmanager_h_

#include <ESP8266WiFi.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>

#include "connection_manager.h"
#include "secrets.h"

class MqttClientManager
{
  public:
    static MqttClientManager* instance() {
      if (!instance_) {
        instance_ = new MqttClientManager;
      }
      return instance_;
    }

    bool wifi_connect();
    void mqtt_connect();

    void blink_builtin(int T_high, int T_low, int N);  

    Adafruit_MQTT_Client& mqtt_client() { return mqtt_; }
  private:

    ConnectionManager conn_;
    Adafruit_MQTT_Client mqtt_;

    static MqttClientManager* instance_;

    MqttClientManager();

};

#endif //mqttclientmanager_h_
