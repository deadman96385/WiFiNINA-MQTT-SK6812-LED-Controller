/* Add your keys & rename this file to auth.h */

#ifndef _AUTH_DETAILS
#define _AUTH_DETAILS

#define WIFI_SSID "ssid"
#define WIFI_PASSWORD "password"
int status = WL_IDLE_STATUS;

#define MQTT_SERVER "mqtt server ip"
#define MQTT_USER ""
#define MQTT_PASSWORD ""
#define MQTT_PORT 1883
const char* MQTT_STATE_TOPIC_PREFIX = "led/"; // e.g. led/<deviceName> and led/<deviceName>/set
#define MQTT_AVAIL_TOPIC "/availability"

#define DATA_PIN_LEDS   15  // D8 on ESP8266

/******************************** CONFIG SETUP *******************************/
#define LED_COUNT_MAXIMUM 2323 // Default number of leds per strip
const int ledCount = LED_COUNT_MAXIMUM;
const char* deviceName = "led";
byte maxBrightness = 255;

#endif
