
/*
  To use this code you will need the following dependancies:

  - Support for the MKR boards.
        - Enable it under the Board Manager

  - You will also need to download the follow libraries by going to Sketch -> Include Libraries -> Manage Libraries
      - WiFiNINA
      - Adafruit NeoPixel
      - PubSubClient
      - ArduinoJSON V5.13.5
*/
// ------------------------------
// ---- all config in auth.h ----
// ------------------------------

// The maximum mqtt message size, including header, is 128 bytes by default.
// You must update your PubSubClient.h file manually.......
#define MQTT_MAX_PACKET_SIZE 1024

#include <ArduinoJson.h> //Not beta version. Tested with v5.13.5
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <WiFiNINA.h>
#include <ArduinoOTA.h>
#include "auth-template.h"
#include <InternalStorage.h>


/****************************************FOR JSON***************************************/
const int BUFFER_SIZE = JSON_OBJECT_SIZE(15);

const char* birthMessage = "online";
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


/******************************** OTHER GLOBALS *******************************/
unsigned long currentMilliSeconds;
unsigned long effectDelayStart;
const unsigned long fiveSeconds = 5000;
const unsigned long tenSeconds = 10000;

const char* on_cmd = "ON";
const char* off_cmd = "OFF";
const char* effectString = "solid";
String previousEffect = "solid";
String effect = "solid";
bool effectStart = false;
unsigned int effectState = 0;
unsigned int effectMemory[500];
const unsigned int effectMemoryLen = 499;
unsigned int firstPixel = 0;
unsigned int lastPixel = ledCount;
bool stateOn = true;
bool transitionDone = true;
bool transitionAbort = false;
unsigned int transitionTime = 50; // 1-150
unsigned int pixelLen = 1;
unsigned int pixelArray[50];
unsigned int effectParameter[4] = {5, 10, 1250, 1391};

const unsigned int stripStart[NUMSTRIPS] = {
  0,    // strip 0 which is not used
  0,    // strip 1
  300,  // strip 2
  635,  // strip 3 which comes after strip 9
  923,  // strip 4
  1223, // strip 5
  1480, // strip 6
  1780, // strip 7
  2080, // strip 8
  422   // strip 9 which comes after strip 2
};

const unsigned int stripEnd[NUMSTRIPS] = {
  0,
  299,
  421,
  922,
  1222,
  1479,
  1779,
  2079,
  2322,
  634
};

const bool stripReversed[NUMSTRIPS] = {
  false,  // strip 0
  true,   // strip 1
  true,   // strip 2
  false,  // strip 3
  false,  // strip 4
  false,  // strip 5
  false,  // strip 6
  false,  // strip 7
  false,  // strip 8
  false   // strip 9
};

bool strip_dirty[NUMSTRIPS];

WiFiClient net;
PubSubClient client(net);

Adafruit_NeoPixel Strip;



#include "NeoPixel_Effects.h"

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}

/********************************** START SETUP*****************************************/
void setup() {

  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println ();
  Serial.print (F("Initial Free memory = "));
  Serial.println (freeMemory ());

  // Turn on power supply for LED strips. Controller runs on standby power from power supply
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);

  for (unsigned int i = 0; i < NUMSTRIPS; i++)
  {
    // End of trinket special code
    pixelStrings[i].setBrightness(maxBrightness);
    pixelStrings[i].begin();
    pixelStrings[i].show(); // Initialize all pixels to 'off'
    strip_dirty[i] = false;
  }

  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(mqttCallback);
  Serial.println(F("Ready"));
  Serial.print (F("End of setup Free memory = "));
  Serial.println (freeMemory ());
}


/********************************** START SETUP WIFI *****************************************/
bool setup_wifi() {
  static unsigned long wifiDelayStart = 0;
  static bool waitingForModule = false;
  static bool waitingForConnect = false;

  if (WiFi.status() == WL_NO_MODULE) {
    if (!waitingForModule) {
      waitingForModule = true;
      wifiDelayStart = currentMilliSeconds;
      Serial.println(F("Communication with WiFi module failed!"));
      return false;
    } else if ((currentMilliSeconds - wifiDelayStart) > tenSeconds) {
      Serial.println(F("Communication with WiFi module failed! 10 second delay finished."));
      WiFi.init();
      wifiDelayStart = currentMilliSeconds;
      return false;
    }
  } else  {
    waitingForModule = false;
  }

  //  while (WiFi.status() == WL_NO_MODULE) {
  //    Serial.println("Communication with WiFi module failed!");
  //    delay(10000);
  //  }

  if (waitingForConnect) {
    if (WiFi.status() == WL_CONNECTED) {
      waitingForConnect = false;
      Serial.println(F(""));
      Serial.println(F("WiFi connected"));
      Serial.print(F("IP address: "));
      Serial.println(WiFi.localIP());
      return true;
    } else if ((currentMilliSeconds - wifiDelayStart) > tenSeconds) {
      // 10 second timeout so attempt again
      wifiDelayStart = currentMilliSeconds;
      Serial.print(F("Attempting again to connect to WPA SSID: "));
      Serial.println(WIFI_SSID);
      // Connect to WPA/WPA2 network:
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      return false;
    }  // else we are in the 10 second wait so do nothing
  } else if (WiFi.status() != WL_CONNECTED) {
    waitingForConnect = true;
    wifiDelayStart = currentMilliSeconds;
    Serial.print(F("Attempting to connect to WPA SSID: "));
    Serial.println(WIFI_SSID);
    // Connect to WPA/WPA2 network:
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }
  //  while (WiFi.status() != WL_CONNECTED) {
  //    Serial.print("Attempting to connect to WPA SSID: ");
  //    Serial.println(WIFI_SSID);
  //    WiFi.disconnect();
  //    // Connect to WPA/WPA2 network:
  //    status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  //    delay(10000);
  //  }
  //
  //  Serial.println(F(""));
  //  Serial.println(F("WiFi connected"));
  //  Serial.print(F("IP address: "));
  //  Serial.println(WiFi.localIP());
  return false;
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
  transitionAbort = true; // Ensure we abort any current effect
  previousRed = 0;
  previousGreen = 0;
  previousBlue = 0;
  previousWhite = 0;
  digitalWrite(10, LOW); // Turn off LED power supply
}

void setOn() {
  digitalWrite(10, HIGH);
  setAll(0, 0, 0, 0);
  stateOn = true;
}

/********************************** START CALLBACK*****************************************/
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.println(F(""));
  Serial.print(F("Message arrived ["));
  Serial.print(topic);
  Serial.print(F("] "));

  char message[length + 1];
  for (unsigned int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);
  //Serial.println(topic);

  if (strcmp(topic, "led/led/set") == 0) {
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
    Serial.println(F("parseObject() failed"));
    return false;
  }

  if (root.containsKey("state")) {
    if (strcmp(root["state"], on_cmd) == 0) {
      stateOn = true;
      effectStart = true;
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
    effectStart = true;
  }

  if (root.containsKey("effect")) {
    effectString = root["effect"];
    effect = effectString;
    effectStart = true;
  }

  if (root.containsKey("firstPixel")) {
    firstPixel = root["firstPixel"];
  }

  if (root.containsKey("lastPixel")) {
    lastPixel = root["lastPixel"];
  }

  if (root.containsKey("parameter1")) {
    effectParameter[0] = root["parameter1"];
  }
  if (root.containsKey("parameter2")) {
    effectParameter[1] = root["parameter2"];
  }
  if (root.containsKey("parameter3")) {
    effectParameter[2] = root["parameter3"];
  }
  if (root.containsKey("parameter4")) {
    effectParameter[3] = root["parameter4"];
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
void attemptReconnect() {
  // Loop until we're reconnected
  //  while (!client.connected()) {
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

    sendState();
  } else {
    Serial.print(F("failed, rc="));
    Serial.print(client.state());
    Serial.println(F(" try again in 5 seconds"));
    // Wait 5 seconds before retrying
    delay(5000);
  }
  //  }
}


/********************************** START MAIN LOOP *****************************************/
void loop() {
  static bool wifiSeen = false;
  static unsigned long msgDelayStart;
  static unsigned long effectDelayStart;
  currentMilliSeconds = millis();
  if ((WiFi.status() != WL_CONNECTED) || !wifiSeen) {
    //    delay(1);
    wifiSeen = setup_wifi();
  }

  if (wifiSeen) {
    // Bring MQTT on line and process messages
    if (!client.connected()) {
      attemptReconnect();
      Serial.println ();
      Serial.println (freeMemory ());
    } else {
      client.loop(); // Check MQTT
    }

    //ArduinoOTA.poll();
  }

  // This var will go away when all effects are state machine based
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
      //      if (effect == "pixel") {
      //        ShowPixels();
      //        // transitionDone = true; // done inside ShowPixels()
      //      }
      if (effect == "twinkle") {
        Twinkle(200, (2 * transitionTime), true);
      }
      if (effect == "cylon bounce") {
        //        CylonBounce(4, transitionTime / 10, 50);
        CylonBounce(effectParameter[0], effectParameter[1], effectParameter[1] * 10);
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
    //  } else {
    //    delay(600); // Save some power? (from 0.9w to 0.4w when off with ESP8266)
  }
  effectStart = false;
}
