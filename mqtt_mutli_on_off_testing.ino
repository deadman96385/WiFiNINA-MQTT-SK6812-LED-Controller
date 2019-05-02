#include <PubSubClient.h>
#include <WiFiNINA.h>
#include <Adafruit_NeoPixel.h>

#include "arduino_secrets.h"
#define BROKER_IP    "192.168.1.172"
//#define BROKER_IP    "172.20.10.3"
#define DEV_NAME     "arduiono"
#define MQTT_USER    "arduiono"
#define MQTT_PW      "arduionopass"
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;

char* MQTT_STATE_TOPIC_PREFIX = "led/"; // e.g. led/<deviceName> and led/<deviceName>/set
#define MQTT_AVAIL_TOPIC "/availability"

char* birthMessage = "online";
const char* lwtMessage = "offline";

#define LED_COUNT 300
#define BRIGHTNESS 255

Adafruit_NeoPixel Strip;

Adafruit_NeoPixel pixelStrings[] = {
  Adafruit_NeoPixel(LED_COUNT, 0, NEO_GRBW),
  Adafruit_NeoPixel(LED_COUNT, 1, NEO_GRBW),
  Adafruit_NeoPixel(LED_COUNT, 2, NEO_GRBW),
  Adafruit_NeoPixel(LED_COUNT, 3, NEO_GRBW),
  Adafruit_NeoPixel(LED_COUNT, 4, NEO_GRBW),
  Adafruit_NeoPixel(LED_COUNT, 5, NEO_GRBW),
  Adafruit_NeoPixel(LED_COUNT, 6, NEO_GRBW),
  Adafruit_NeoPixel(LED_COUNT, 7, NEO_GRBW),
  Adafruit_NeoPixel(LED_COUNT, 8, NEO_GRBW),
  Adafruit_NeoPixel(LED_COUNT, 9, NEO_GRBW)
};

#define NUMSTRIPS (sizeof(pixelStrings)/sizeof(pixelStrings[0]))

WiFiClient net;
PubSubClient client(net);

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(500);
  }

  for (int i = 0; i < NUMSTRIPS; i++)
  {
    pixelStrings[i].begin();
    pixelStrings[i].show(); // Initialize all pixels to 'off'
    pixelStrings[i].setBrightness(BRIGHTNESS);
  }

  for (int y = 0; y < NUMSTRIPS; y++)
  {
    Strip = pixelStrings[y];
    for (int j = 0; j < Strip.numPixels(); j++) {
      Strip.setPixelColor(j, Strip.Color(0, 0, 0, 255));
      Strip.show();
      delay(1);
    }
  }

  wifi_connect();

  for (int y = 0; y < NUMSTRIPS; y++)
  {
    Strip = pixelStrings[y];
    for (int j = 0; j < Strip.numPixels(); j++) {
      Strip.setPixelColor(j, Strip.Color(255, 0, 0, 0));
      Strip.show();
      delay(1);
    }
  }

  client.setServer(BROKER_IP, 1883);
  client.setCallback(callback);

  mqtt_connect();

  for (int y = 0; y < NUMSTRIPS; y++)
  {
    Strip = pixelStrings[y];
    for (int j = 0; j < Strip.numPixels(); j++) {
      Strip.setPixelColor(j, Strip.Color(0, 0, 255, 0));
      Strip.show();
      delay(1);
    }
    delay(500);
    for (int j = 0; j < Strip.numPixels(); j++) {
      Strip.setPixelColor(j, Strip.Color(0, 0, 0, 0));
      Strip.show();
      delay(1);
    }
  }

}

void loop() {
  if (!client.connected()) {
    mqtt_connect();
  }

  if (WiFi.status() != WL_CONNECTED) {
    delay(1);
    Serial.print(F("WIFI Disconnected. Attempting reconnection."));
    wifi_connect();
    return;
  }

  client.loop();
}

void callback(char* topic, byte* payload, unsigned int length) {

  Serial.print("Message arrived in topic: ");
  Serial.println(topic);

  Serial.print("Message:");
  for (int k = 0; k < length; k++) {
    char receivedChar = (char)payload[k];
    Serial.print(receivedChar);

    if (receivedChar == '1') {
      for (int i = 0; i < NUMSTRIPS; i++)
      {
        Strip = pixelStrings[i];
        for (int j = 0; j < Strip.numPixels(); j++) {
          Strip.setPixelColor(j, Strip.Color(0, 0, 0, 255));
        }
        Strip.show();
      }
    } else {
      for (int i = 0; i < NUMSTRIPS; i++)
      {
        Strip = pixelStrings[i];
        for (int j = 0; j < Strip.numPixels(); j++) {
          Strip.setPixelColor(j, Strip.Color(0, 0, 0, 0));
        }
        Strip.show();
      }
    }
  }
  Serial.println();
  Serial.println("-----------------------");
}

void wifi_connect() {
  while (status == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    delay(10000);
  }

  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    WiFi.disconnect();
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);
    delay(10000);
  }

  Serial.println("Connected to the WiFi network");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void mqtt_connect() {
  while (!client.connected()) {
    Serial.print(F("Attempting MQTT connection...\n"));

    char mqttAvailTopic[sizeof(MQTT_STATE_TOPIC_PREFIX) + sizeof(DEV_NAME) + sizeof(MQTT_AVAIL_TOPIC)];
    sprintf(mqttAvailTopic, "%s%s%s", MQTT_STATE_TOPIC_PREFIX, DEV_NAME, MQTT_AVAIL_TOPIC);

    if (client.connect(DEV_NAME, MQTT_USER, MQTT_PW, mqttAvailTopic, 0, true, lwtMessage)) {
      Serial.println(F("connected"));

      client.publish(mqttAvailTopic, birthMessage, true);

      client.subscribe("/hello");

    } else {
      Serial.print(F("failed, rc="));
      Serial.print(client.state());
      Serial.println(F(" try again in 5 seconds"));
      delay(5000);
    }
  }
}
