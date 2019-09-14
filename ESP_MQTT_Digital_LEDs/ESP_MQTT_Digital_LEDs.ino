/*
  To use this code you will need the following dependancies:

  - Support for the MKR boards.
        - Enable it under the Board Manager

  - You will also need to download the follow libraries by going to Sketch -> Include Libraries -> Manage Libraries
      - WiFiNINA
      - Adafruit NeoPixel
      - PubSubClient
      - ArduinoJSON V5.3.14
*/
// ------------------------------
// ---- all config in auth.h ----
// ------------------------------

// The maximum mqtt message size, including header, is 128 bytes by default.
// You must update your PubSubClient.h file manually.......
#define MQTT_MAX_PACKET_SIZE 512

#include <ArduinoJson.h> //Not beta version. Tested with v5.3.14
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <WiFiNINA.h>
#include <ArduinoOTA.h>
#include "auth-template.h"
#include <InternalStorage.h>

/****************************************FOR JSON***************************************/
const int BUFFER_SIZE = JSON_OBJECT_SIZE(60);

char* birthMessage = "online";
const char* lwtMessage = "offline";

/*********************************** LED Defintions ********************************/
// Real values as requested from the MQTT server
byte realRed = 0;
byte realGreen = 0;
byte realBlue = 0;
byte realWhite = 255;

// Previous requested values
byte previousRed = 0;
byte previousGreen = 0;
byte previousBlue = 0;
byte previousWhite = 0;

// Values as set to strip
byte red = 0;
byte green = 0;
byte blue = 0;
byte white = 0;
byte brightness = 255;


/******************************** OTHER GLOBALS *******************************/
const char* on_cmd = "ON";
const char* off_cmd = "OFF";
const char* effectString = "solid";
String previousEffect = "solid";
String effect = "solid";
bool stateOn = true;
bool transitionDone = true;
bool transitionAbort = false;
int transitionTime = 50; // 1-150
int pixelLen = 1;
int pixelArray[50];

int stripStart0 ;
int stripStart1 ;
int stripStart2 ;
int stripStart3 ;
int stripStart4 ;
int stripStart5 ;
int stripStart6 ;
int stripStart7 ;
int stripStart8 ;
int stripStart9 ;
int stripEnd0 ;
int stripEnd1 ;
int stripEnd2 ;
int stripEnd3 ;
int stripEnd4 ;
int stripEnd5 ;
int stripEnd6 ;
int stripEnd7 ;
int stripEnd8 ;
int stripEnd9 ;

bool strip_dirty[10];

WiFiClient net;
PubSubClient client(net);

Adafruit_NeoPixel Strip;

Adafruit_NeoPixel pixelStrings[] = {
  Adafruit_NeoPixel(1, 0, NEO_GRBW),
  Adafruit_NeoPixel(300, 1, NEO_GRBW),
  Adafruit_NeoPixel(122, 2, NEO_GRBW),
  Adafruit_NeoPixel(288, 3, NEO_GRBW),
  Adafruit_NeoPixel(300, 4, NEO_GRBW),
  Adafruit_NeoPixel(257, 5, NEO_GRBW),
  Adafruit_NeoPixel(300, 6, NEO_GRBW),
  Adafruit_NeoPixel(300, 7, NEO_GRBW),
  Adafruit_NeoPixel(243, 8, NEO_GRBW),
  Adafruit_NeoPixel(213, 9, NEO_GRBW)
};

#define NUMSTRIPS (sizeof(pixelStrings)/sizeof(pixelStrings[0]))

#include "NeoPixel_Effects.h"

/********************************** START SETUP*****************************************/
void setup() {
  Serial.begin(115200);

  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);

  for (int i = 0; i < NUMSTRIPS; i++)
  {
    // End of trinket special code
    pixelStrings[i].setBrightness(maxBrightness);
    pixelStrings[i].begin();
    pixelStrings[i].show(); // Initialize all pixels to 'off'
    strip_dirty[i] = false;
  }

  //
  //  for (int y = 0; y < NUMSTRIPS; y++)
  //  {
  //    Strip = pixelStrings[y];
  //    for (int j = 0; j < Strip.numPixels(); j++) {
  //      Strip.setPixelColor(j, Strip.Color(0, 0, 0, 255));
  //      Strip.show();
  //      delay(1); // Need delay to be like a yield so it will not restart
  //    }
  //  }

  //  setup_wifi();
  //
  //  ArduinoOTA.begin(WiFi.localIP(), "Arduino", "password", InternalStorage);

  stripStart1 = 0;
  stripEnd1 = stripStart1 + 299;
  stripStart2 = stripEnd1 + 1;
  stripEnd2 = stripStart2 + 121;
  stripStart9 = stripEnd2 + 1; // 9 is here due to moving a strip to another pin
  stripEnd9 = stripStart9 + 212;
  stripStart3 = stripEnd9 + 1;
  stripEnd3 = stripStart3 + 287;
  stripStart4 = stripEnd3 + 1;
  stripEnd4 = stripStart4 + 299;
  stripStart5 = stripEnd4 + 1;
  stripEnd5 = stripStart5 + 256;
  stripStart6 = stripEnd5 + 1;
  stripEnd6 = stripStart6 + 299;
  stripStart7 = stripEnd6 + 1;
  stripEnd7 = stripStart7 + 299;
  stripStart8 = stripEnd7 + 1;
  stripEnd8 = stripStart8 + 242;


//  setPixel(stripStart1, 0, 0, 255, 0, false);
//  setPixel(stripEnd1, 0, 0, 255, 0, false);
//  Strip.show();
//  setPixel(stripStart2, 0, 255, 0, 0, false);
//  setPixel(stripEnd2, 0, 255, 0, 0, false);
//  Strip.show();
//  setPixel(stripStart3, 255, 0, 0, 0, false);
//  setPixel(stripEnd3, 255, 0, 0, 0, false);
//  Strip.show();
//  setPixel(stripStart4, 0, 0, 0, 255, false);
//  setPixel(stripEnd4, 0, 0, 0, 255, false);
//  Strip.show();
//  setPixel(stripStart5, 0, 0, 0, 255, false);
//  setPixel(stripEnd5, 0, 0, 0, 255, false);
//  Strip.show();
//  setPixel(stripStart6, 0, 0, 0, 255, false);
//  setPixel(stripEnd6, 0, 0, 0, 255, false);
//  Strip.show();
//  setPixel(stripStart7, 0, 0, 0, 255, false);
//  setPixel(stripEnd7, 0, 0, 0, 255, false);
//  Strip.show();
//  setPixel(stripStart8, 0, 0, 0, 255, false);
//  setPixel(stripEnd8, 0, 0, 0, 255, false);
//  Strip.show();
//  setPixel(stripStart9, 0, 0, 0, 255, false);
//  setPixel(stripEnd9, 0, 0, 0, 255, false);
//  Strip.show();
  //
 // for (int y = stripStart1; y <= stripEnd8; y++)
//  {
    //    Serial.println(y, DEC);
//    setPixel(y, 0, 0, 0, 255, false);
//    show_dirty();
//    delay(1); // Need delay to be like a yield so it will not restart
//    setPixel(y, 0, 0, 0, 0, false);
     setAll(0, 0, 0, 127);
//  }

  //  client.setServer(MQTT_SERVER, MQTT_PORT);
  //  client.setCallback(callback);

  Serial.println(F("Ready"));

  //  for (int y = 0; y < NUMSTRIPS; y++)
  //  {
  //    Strip = pixelStrings[y];
  //    for (int j = 0; j < Strip.numPixels(); j++) {
  //      Strip.setPixelColor(j, Strip.Color(0, 0, 255, 0));
  //      Strip.show();
  //      delay(1);
  //    }
  //    delay(500);
  //    for (int j = 0; j < Strip.numPixels(); j++) {
  //      Strip.setPixelColor(j, Strip.Color(0, 0, 0, 0));
  //      Strip.show();
  //      delay(1);
  //    }
  //  }
}


/********************************** START SETUP WIFI *****************************************/
void setup_wifi() {
  while (status == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    delay(10000);
  }

  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(WIFI_SSID);
    WiFi.disconnect();
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    delay(10000);
  }

  Serial.println(F(""));
  Serial.println(F("WiFi connected"));
  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());
}

/*
  SAMPLE PAYLOAD:
  {
    "brightness": 120,
    "color": {
      "r": 255,
      "g": 100,
      "b": 100
    },
    "flash": 2,
    "transition": 5,
    "state": "ON"
  }
*/


/********************************** START LED POWER STATE *****************************************/
void setOff() {
  setAll(0, 0, 0, 0);
  stateOn = false;
  transitionDone = true; // Ensure we dont run the loop
  transitionAbort = true; // Ensure we about any current effect
  previousRed = 0;
  previousGreen = 0;
  previousBlue = 0;
  previousWhite = 0;
  digitalWrite(10, LOW);
}

void setOn() {
  digitalWrite(10, HIGH);
  stateOn = true;
}

/********************************** START CALLBACK*****************************************/
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println(F(""));
  Serial.print(F("Message arrived ["));
  Serial.print(topic);
  Serial.print(F("] "));

  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);
  //Serial.println(topic);

  if (strcmp(topic, "led/led/num") == 0) {
    ledCount0 = atoi(message);

    Serial.print(F("Number of leds:"));
    Serial.println(ledCount0);

  } else if (strcmp(topic, "led/led/set") == 0) {

    previousEffect = effect;

    if (!processJson(message)) {
      return;
    }

    previousRed = red;
    previousGreen = green;
    previousBlue = blue;
    previousWhite = white;

    if (stateOn) {
      red = map(realRed, 0, 255, 0, brightness);
      green = map(realGreen, 0, 255, 0, brightness);
      blue = map(realBlue, 0, 255, 0, brightness);
      white = map(realWhite, 0, 255, 0, brightness);
    } else {
      red = 0;
      green = 0;
      blue = 0;
      white = 0;
    }

    Serial.println(effect);

    transitionAbort = true; // Kill the current effect
    transitionDone = false; // Start a new transition

    if (stateOn) {
      setOn();
    } else {
      setOff(); // NOTE: Will change transitionDone
    }

    sendState();
  }
}


/********************************** START PROCESS JSON*****************************************/
bool processJson(char* message) {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(message);

  if (!root.success()) {
    Serial.println("parseObject() failed");
    return false;
  }

  if (root.containsKey("state")) {
    if (strcmp(root["state"], on_cmd) == 0) {
      stateOn = true;
    }
    else if (strcmp(root["state"], off_cmd) == 0) {
      stateOn = false;
    }
    else {
      sendState();
      return false;
    }
  }

  if (root.containsKey("transition")) {
    transitionTime = root["transition"];
  }

  if (root.containsKey("color")) {
    realRed = root["color"]["r"];
    realGreen = root["color"]["g"];
    realBlue = root["color"]["b"];
    realWhite = 0;
  }

  // To prevent our power supply from having a cow. Only RGB OR White
  // Disabled because defaulting to it limits the total brightness if someone has a good power supply
  if (root.containsKey("white_value")) {
    //    realRed = 0;
    //    realGreen = 0;
    //    realBlue = 0;
    realWhite = root["white_value"];
  }

  if (root.containsKey("brightness")) {
    brightness = root["brightness"];
  }

  if (root.containsKey("pixel")) {
    pixelLen = root["pixel"].size();
    if (pixelLen > sizeof(pixelArray)) {
      pixelLen = sizeof(pixelArray);
    }
    for (int i = 0; i < pixelLen; i++) {
      pixelArray[i] = root["pixel"][i];
    }
  }

  if (root.containsKey("effect")) {
    effectString = root["effect"];
    effect = effectString;
  }

  return true;
}


/********************************** START SEND STATE*****************************************/
void sendState() {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();

  root["state"] = (stateOn) ? on_cmd : off_cmd;
  JsonObject& color = root.createNestedObject("color");
  color["r"] = realRed;
  color["g"] = realGreen;
  color["b"] = realBlue;

  root["white_value"] = realWhite;
  root["brightness"] = brightness;
  root["transition"] = transitionTime;
  root["effect"] = effect.c_str();

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));

  char combinedArray[sizeof(MQTT_STATE_TOPIC_PREFIX) + sizeof(deviceName)];
  sprintf(combinedArray, "%s%s", MQTT_STATE_TOPIC_PREFIX, deviceName); // with word space
  if (!client.publish(combinedArray, buffer, true)) {
    Serial.println(F("Failed to publish to MQTT. Check you updated your MQTT_MAX_PACKET_SIZE"));
  }
}


/********************************** START RECONNECT *****************************************/
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print(F("Attempting MQTT connection..."));

    char mqttAvailTopic[sizeof(MQTT_STATE_TOPIC_PREFIX) + sizeof(deviceName) + sizeof(MQTT_AVAIL_TOPIC)];
    sprintf(mqttAvailTopic, "%s%s%s", MQTT_STATE_TOPIC_PREFIX, deviceName, MQTT_AVAIL_TOPIC); // with word space

    // Attempt to connect
    if (client.connect(deviceName, MQTT_USER, MQTT_PASSWORD, mqttAvailTopic, 0, true, lwtMessage)) {
      Serial.println(F("connected"));

      // Publish the birth message on connect/reconnect
      client.publish(mqttAvailTopic, birthMessage, true);

      char combinedArray[sizeof(MQTT_STATE_TOPIC_PREFIX) + sizeof(deviceName) + 4];
      sprintf(combinedArray, "%s%s/set", MQTT_STATE_TOPIC_PREFIX, deviceName); // with word space
      client.subscribe(combinedArray);

      char combinedArray2[sizeof(MQTT_STATE_TOPIC_PREFIX) + sizeof(deviceName) + 4];
      sprintf(combinedArray2, "%s%s/num", MQTT_STATE_TOPIC_PREFIX, deviceName); // with word space
      client.subscribe(combinedArray2);

      setOff();
      sendState();
    } else {
      Serial.print(F("failed, rc="));
      Serial.print(client.state());
      Serial.println(F(" try again in 5 seconds"));
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


/********************************** START MAIN LOOP *****************************************/
void loop() {

  //  if (!client.connected()) {
  //    reconnect();
  //  }

  //  if (WiFi.status() != WL_CONNECTED) {
  //    delay(1);
  //    Serial.print(F("WIFI Disconnected. Attempting reconnection."));
  //    setup_wifi();
  //    return;
  //  }

  //  client.loop(); // Check MQTT

  //  ArduinoOTA.poll();

  transitionAbort = false; // Because we came from the loop and not 1/2 way though a transition

  if (!transitionDone) {  // Once we have completed the transition, No point to keep going though the process
    if (stateOn) {   // if the light is turned on

      //EFFECTS
      if (effect == "clear") {
        setAll(0, 0, 0, 0);
        transitionDone = true;
      }
      if (effect == "solid") {
        //        if (transitionTime <= 1) {
        setAll(red, green, blue, white);
        transitionDone = true;
        //        } else {
        //          Fade(transitionTime);
        //        }
      }
      if (effect == "pixel") {
        ShowPixels();
      }
      if (effect == "twinkle") {
        Twinkle(10, (2 * transitionTime), false);
      }
      if (effect == "cylon bounce") {
        CylonBounce(4, transitionTime / 10, 50);
      }
      if (effect == "fire") {
        Fire(55, 120, (2 * transitionTime / 2));
      }
      if (effect == "fade in out") {
        FadeInOut();
      }
      if (effect == "strobe") {
        Strobe(10, transitionTime);
      }
      if (effect == "theater chase") {
        theaterChase(transitionTime);
      }
      if (effect == "rainbow cycle") {
        rainbowCycle(transitionTime / 5);
      }
      if (effect == "color wipe") {
        colorWipe(transitionTime / 20);
      }
      if (effect == "running lights") {
        RunningLights(transitionTime);
      }
      if (effect == "snow sparkle") {
        SnowSparkle(20, random(transitionTime, (10 * transitionTime)));
      }
      if (effect == "sparkle") {
        Sparkle(transitionTime);
      }
      if (effect == "twinkle random") {
        TwinkleRandom(20, (2 * transitionTime), false);
      }
      if (effect == "bouncing balls") {
        BouncingBalls(3);
      }
      if (effect == "lightning") {
        Lightning(transitionTime);
      }





      // Run once notification effects
      // Reverts color and effect after run
      if (effect == "color wipe once") {
        colorWipeOnce(transitionTime);

        if (effect != "color wipe once") {
          effect = previousEffect;
        }

        if (red == 0 && green == 0 && blue == 0 && white == 0) {
          setOff();
        } else {
          transitionDone = false; // Run the old effect again
        }
        sendState();
      }



      //      if (effect == "bpm") {
      //      }
      //      if (effect == "candy cane") {
      //      }
      //      if (effect == "confetti" ) {
      //      }
      //      if (effect == "dots") {
      //      }
      //      if (effect == "glitter") {
      //      }
      //      if (effect == "juggle" ) {                           // eight colored dots, weaving in and out of sync with each other
      //      }
      //      if (effect == "lightning") {
      //      }
      //      if (effect == "police all") {                 //POLICE LIGHTS (TWO COLOR SOLID)
      //      }
      //      if (effect == "police one") {
      //      }
      //      if (effect == "rainbow with glitter") {               // FastLED's built-in rainbow generator with Glitter
      //      }

    } else {
      setAll(0, 0, 0, 0);
      transitionDone = true;
    }
  } else {
    delay(600); // Save some power? (from 0.9w to 0.4w when off with ESP8266)
  }
}
