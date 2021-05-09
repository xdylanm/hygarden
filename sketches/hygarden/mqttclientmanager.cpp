#include "mqttclientmanager.h"

MqttClientManager* MqttClientManager::instance_ = nullptr;

MqttClientManager::MqttClientManager() : 
  conn_(WLAN_SSID, WLAN_PASS), 
  mqtt_(&(conn_.wifi_client()), MQTT_SERVER, MQTT_SERVERPORT)
{

}
    
// N times: off (T_low), on (T_high)
// finished state ON
void MqttClientManager::blink_builtin(int T_high, int T_low, int N) 
{
  for (int i = 0; i < N; i++) {
    digitalWrite(BUILTIN_LED, HIGH);
    delay(T_low);
    digitalWrite(BUILTIN_LED, LOW);
    delay(T_high);
  }
}

void MqttClientManager::mqtt_connect()
{
  int8_t ret;

  // Stop if already connected.
  if (mqtt_.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt_.connect()) != 0) { // connect will return 0 for connected
      Serial.println(mqtt_.connectErrorString(ret));
      Serial.println("Retrying MQTT connection in 5 seconds...");
      mqtt_.disconnect();

      blink_builtin(150,100,4);
      delay(4000);  // wait 5 seconds
      retries--;
      if (retries == 0) {
        // basically die and wait for WDT to reset me
        while (1);
      }
  }

  Serial.println("MQTT Connected!");
}


bool MqttClientManager::wifi_connect()
{
  if (!conn_.load_certs()) {
    Serial.println("Failed to load certificates for secure connection.");
    return false;
  }

  while (conn_.initialize() != WL_CONNECTED) {
    Serial.println(F("Failed to connect to network. Restarting..."));
    
    delay(5000);
    blink_builtin(200,100,2);
    
    ESP.restart();
  }

  blink_builtin(300,200,4);
  
  return true;
}
