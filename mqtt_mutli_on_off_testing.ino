#include <PubSubClient.h>
#include <WiFiNINA.h>
#include <Adafruit_NeoPixel.h>

#include "arduino_secrets.h"
//#define BROKER_IP    "192.168.1.172"
#define BROKER_IP    "172.20.10.3"
#define DEV_NAME     "arduiono"
#define MQTT_USER    "arduiono"
#define MQTT_PW      "arduionopass"
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;

#define LED_COUNT 300
#define BRIGHTNESS 255

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
  Serial.println(WiFi.localIP());

  client.setServer(BROKER_IP, 1883);
  client.setCallback(callback);

  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");

    if (client.connect("ESP8266Client", MQTT_USER, MQTT_PW )) {
      Serial.println("connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  client.subscribe("/hello");

  for (int i = 0; i < NUMSTRIPS; i++)
  {
    pixelStrings[i].begin();
    pixelStrings[i].show(); // Initialize all pixels to 'off'
    pixelStrings[i].setBrightness(BRIGHTNESS);
  }
}

void loop() {
  client.loop();
}

void callback(char* topic, byte* payload, unsigned int length) {

  Adafruit_NeoPixel Strip;

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
        Serial.print(i);
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
