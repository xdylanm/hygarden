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
Initial testing of BME280 and soil moisture sensor. No Wifi. Adds configuration (e.g. solenoid mode, sample intervals, moisture threshold) which serializes and deserializes with LittleFS.

## MQTT
Test case combining Wifi test with MQTT publish & subscribe. Publish BME280 data, subscribe on board LED on/off with callbacks. Using LittleFS to load certificate.

References
* https://github.com/adafruit/Adafruit_MQTT_Library (see included examples mqtt_esp8266 and mqtt_esp8266_callback)
* https://learn.adafruit.com/mqtt-adafruit-io-and-you?view=all

## HyGarden
The garden hydration monitor. Publishes to /environment and /status, subscribes to /control and /discovery. 

### Configuration

The endpoint uses a persistent configuration for state and behavior, which is stored in flash in JSON format. A example config is as follows (see hygardenconfig.h)

```json
{
  "topic": "/environment",
  "uuid": "07a75168-6cdf-4333-a548-aae1a9549b16",
  "active":false,
  "sensors": {
    "BME280": {
      "count":1,
      "connected":false,
      "enabled":true
    },
    "SoilMoisture": {
      "count":1,
      "enabled":true,
      "threshold":0.55,
      "operation":"avg",
      "select":0
    }
  },
  "solenoid": {
    "mode":"off",
    "state":"open",
    "min_on":0,
    "max_on":600
  },
  "interval": {
    "sample":5,
    "report":20
  }
}

```

Usage notes

* A new UUID can be set through /status. This should deactivate the endpoint, requiring re-discovery & activation (not implemented).
* sensors.BME280 is read-only
  * count is always 1
  * connected reports if the sensor was connected on power-up
* sensors.SoilMoisture is read/write
  * count must be less than 8 and should be less than the number of physically connected probes (if not, they will return 0)
  * threshold should be between 0 and 1 (0.5 typ.)
  * operation can be "none", "avg", "min", or "max". 
  * select is a probe identifier from 0 to count-1. If operation is "none", this probe value is reported/used, otherwise select is ignored. 
* solenoid
  * mode: on|off|auto
  * state: open|closed, read-only
  * min_on, max_on: time in seconds for minimum and maximum duration that the valve will be opened
* interval
  * sample: time in seconds between sensors samples (for averaging)
  * report: time in seconds between publications to /status 

### Discovery

Discovery is initiated by publishing a short (<16 chars) key to /discovery, e.g. /discovery/abcd1234. Any connected devices should respond by publishing a JSON-formatted message to /status that echoes the discover key and shares their UUID and activation status, e.g. 

```json
{
    "discovery_key":"abcd1234", 
    "uuid":"07a75168-6cdf-4333-a548-aae1a9549b16",
	"active":true
}
```

### Control
Control messages can initiate actions including "read", "write" and "store"
Each JSON-formated control message should follow the form 

```json
{
  "action": "<read|write|store>",
  "destination_uuid":"<uuid>",
  // additional config content
}
```

#### Read

A read action will trigger the endpoint to publish its current configuration (JSON) to /status including read-only properties (state).

#### Write

A write action will update the endpoint's configuration according the the remainder of the action body (e.g. force the solenoid on or off). Only fields included in the write will be updated. Example: update the soil moisture threshold for the solenoid and set the solenoid mode to "auto"

```json
{
  "action": "write",
  "destination_uuid": "07a75168-6cdf-4333-a548-aae1a9549b16",
  "sensors": {
    "SoilMoisture": {
      "threshold":0.56
    }
  },
  "solenoid" : {
  	"mode":"auto"
  }
}
```

#### Store

A store action will trigger the endpoint to write its config back to flash.

### Status
The endpoint will publish information about its configuration to /status with JSON formatted messages that include its UUID. See sections Read and Discovery.

### Environment
The endpoint will publish information about its sensors to /environment in a message that includes its UUID. The message format will be

```json
{
	"uuid" :  "07a75168-6cdf-4333-a548-aae1a9549b16",
	"temperature" : 24.23,
	"humidity" : 44.17,
	"pressure" : 1023.45,
	"moisture" : 0.423
}
```



### Todo
* (med) Implement min and max times for solenoid open
* (med) Monitor water flow/total on time
* (low) Enable topic setting for environment
* (low) add an action to reload from storage (i.e. reset settings)
* ~~(med) Implement min/max/average for soil moisture~~
* (low) deactivate on new UUID
