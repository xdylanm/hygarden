#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ESP8266WiFi.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>

#include "connection_manager.h"
#include "secrets.h"

// Global objects
ConnectionManager conn (WLAN_SSID, WLAN_PASS);
Adafruit_BME280 bme_sensor;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&(conn.wifi_client()), MQTT_SERVER, MQTT_SERVERPORT);

Adafruit_MQTT_Publish pub_sensor_data = Adafruit_MQTT_Publish(&mqtt, "/environment/35c744cb-47f9-45d0-8ced-0b18aec5ac2e");

Adafruit_MQTT_Subscribe sub_onboard_led = Adafruit_MQTT_Subscribe(&mqtt, "/control/35c744cb-47f9-45d0-8ced-0b18aec5ac2e",MQTT_QOS_1);

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

void onoffcallback(char* data, uint16_t len) {
  Serial.print("onoffcallback: ");
  Serial.println(data);

  if (strcmp(data,"on") == 0) {
    digitalWrite(BUILTIN_LED, LOW);
  } else if (strcmp(data,"off") == 0) {
    digitalWrite(BUILTIN_LED, HIGH);
  }
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() 
{
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }

  Serial.println("MQTT Connected!");
}

uint32_t last_pub_time_ms = 0;


void setup() {
  pinMode(BUILTIN_LED, OUTPUT);
  
  Serial.begin(115200);
  delay(10);

  // blinky 2s, ON at end
  blinky(160,40,10);
  Serial.println();
  Serial.println(F("Wifi+MQTT Example"));
  Serial.println();

  bool const bme_connected = bme_sensor.begin(0x76);
  delay(10);
  if (!bme_connected) {
    Serial.println("Failed to find BME280 sensor.");
    return;
  }
  delay(1000);

  if (!conn.load_certs()) {
    Serial.println("Failed to load certificates for secure connection.");
    return;
  }

  while (conn.initialize() != WL_CONNECTED) {
    Serial.println(F("Failed to connect to network. Restarting..."));
    
    blinky(200,100,2);
    delay(5000);
    blinky(200,100,2);
    
    ESP.restart();
  }

  blinky(200,100,4);
  digitalWrite(BUILTIN_LED, HIGH);  // off, connected

  sub_onboard_led.setCallback(onoffcallback);
  mqtt.subscribe(&sub_onboard_led);

  delay(500);

  last_pub_time_ms = millis();


}

uint32_t get_elapsed_time() {
  uint32_t elapsed_time = millis();
  if (elapsed_time > last_pub_time_ms) {  // no roll-over
    elapsed_time -= last_pub_time_ms;
  } else {
    elapsed_time = uint32_t(-1) - (last_pub_time_ms - elapsed_time);
  }
  elapsed_time /= 1000; // to seconds
  return elapsed_time;
}

char const* sensor_msg_format = 
 "{\"type\":\"BME280\","
  "\"temperature\":%3.1f,"
  "\"pressure\":%3.1f,"
  "\"humidity\":%3.1f}";
char sensor_msg_buffer[256];

int ping_count = 0;

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // this is our 'wait for incoming subscription packets and callback em' busy subloop
  // try to spend your time here:
  mqtt.processPackets(10000);

  uint32_t const elapsed_time = get_elapsed_time(); // seconds
  
  if (elapsed_time > 30) {
    // publish
    sprintf(sensor_msg_buffer,
      sensor_msg_format, 
      bme_sensor.readTemperature(),
      bme_sensor.readPressure() / 100.0F,
      bme_sensor.readHumidity());
    Serial.println(sensor_msg_buffer);
    if (! pub_sensor_data.publish(sensor_msg_buffer)) {
      Serial.println(F("Failed pub"));
    } 

    last_pub_time_ms = millis();
    ping_count = 0;
  } else if (elapsed_time > (ping_count*MQTT_CONN_KEEPALIVE + (MQTT_CONN_KEEPALIVE*2)/3)) {
    // ping the server to keep the mqtt connection alive
    // NOT required if you are publishing once every KEEPALIVE seconds
    Serial.println("MQTT.ping");
    if(! mqtt.ping()) {
      mqtt.disconnect();
    }  
  }
  
}
