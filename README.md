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
* https://github.com/adafruit/Adafruit_MQTT_Library (see included examples mqtt_esp8266 and mqtt_esp8266_callback)
* https://learn.adafruit.com/mqtt-adafruit-io-and-you?view=all

## HyGarden
The garden hydration monitor. Publishes to /environment and /status, subscribes to
/control and /discovery. 
### Discovery
Discovery is initiated by publishing a short (<16 chars) key to /discovery, e.g.
/discovery/abcd1234. Any connected devices should respond by publishing a JSON-formatted
message to /status that echoes the discover key and shares their UUID, e.g. 
{"discovery_key":"abcd1234", "uuid":"07a75168-6cdf-4333-a548-aae1a9549b16"}

### Control
Control messages can initiate actions including "read", "write" and "store".
Each JSON-formated control message should follow the form 
{
  "action": "<read|write|store>",
  "destination_uuid":"<uuid>",
  // additional config content
}
A read action will trigger the endpoint to publish its current configuration (JSON)
to /status. A write action will update the endpoint's configuration according the 
the remainder of the action body (e.g. force the solenoid on or off). A store action
will trigger the endpoint to write its config back to flash.

### Status
The endpoint will publish information about its configuration to /status with
messages that include its UUID.

### Environment
The endpoint will publish information about its sensors to /environment in a 
message that includes its UUID.

### Todo
* Implement an uninitialized state in the config so that the endpoint will not
broadcast sensor data until after first discovery
* Implement min and max times for solenoid open
