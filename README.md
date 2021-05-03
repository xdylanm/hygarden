# hygarden
Automated hydration for a garden

## Components
* EPS8266 (Wemos D1 Mini)
* Capacitive soil sensors
* 12V solenoid 1/2" water valve (NC)
* LM2596 5VDC buck converter
* 60W AC power adapter (12VDC/5A)
* RFP30N0LE 30A/60V logic level NMOS

# Projects
## Blink
Confirm that the board can be programmed. 

## Wifi
Initial testing of connection to wifi network. Monitor for simple requests to toggle the LED.
Reference: https://randomnerdtutorials.com/esp8266-web-server/

## Sensors
Initial testing of BME280 and soil moisture sensor. No Wifi. Adds configuration
(e.g. solenoid mode, sample intervals, moisture threshold) which serializes
and deserializes with LittleFS.

## MQTT
Test case combining Wifi test with MQTT publish & subscribe. Publish BME280 data,
subscribe on board LED on/off with callbacks. Using LittleFS to load certificate.

References
* https://github.com/adafruit/Adafruit_MQTT_Library
** included examples mqtt_esp8266 and mqtt_esp8266_callback
* https://learn.adafruit.com/mqtt-adafruit-io-and-you?view=all


