#define SECRET_SSID "SSID"
#define SECRET_PASS PASS
#define BROKER_IP    "192.168.1.172"
#define DEV_NAME     "porch"
#define MQTT_USER    "USERNAME"
#define MQTT_PW      "PASSWORD"
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;

char* MQTT_STATE_TOPIC_PREFIX = "led/"; // e.g. led/<deviceName> and led/<deviceName>/set
#define MQTT_AVAIL_TOPIC "/availability"

#define LED_COUNT 300
int ledCount = LED_COUNT;
#define BRIGHTNESS 255
byte maxBrightness = BRIGHTNESS;
