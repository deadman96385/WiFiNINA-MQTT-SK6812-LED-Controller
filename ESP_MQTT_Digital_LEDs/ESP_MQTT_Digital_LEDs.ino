
/*
  To use this code you will need the following dependancies:

  - Support for the MKR boards.
        - Enable it under the Board Manager

  - You will also need to download the follow libraries by going to Sketch -> Include Libraries -> Manage Libraries
      - WiFiNINA
      - Adafruit NeoPixel
      - PubSubClient
      - ArduinoJSON
*/
// ------------------------------
// ---- all config in auth.h ----
// ------------------------------

// The maximum mqtt message size, included via header, is 256 bytes by default.
#define MQTT_MAX_PACKET_SIZE 1024
#define MQTT_KEEPALIVE 60
#define MQTT_SOCKET_TIMEOUT 60

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <WiFiNINA.h>
#include <ArduinoOTA.h>
#include "auth-template.h"
#include <InternalStorage.h>


/****************************************FOR JSON***************************************/
const int BUFFER_SIZE = JSON_OBJECT_SIZE(60);

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
int brightness = 255;

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

// Adafruit_NeoPixel Strip;


#include "NeoPixel_Effects.h"

#define effectQueueSize 50
effectData effectQueue[effectQueueSize];

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

  unsigned int i, j;
  Serial.begin(115200);
  //  while (!Serial) {
  //    ; // wait for serial port to connect. Needed for native USB port only
  //  }
  Serial.println ();
  Serial.print (F("Initial Free memory = "));
  Serial.println (freeMemory ());
  // if analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  randomSeed(analogRead(0));

  // Turn on power supply for LED strips. Controller runs on standby power from power supply
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);

  for (i = 0; i < NUMSTRIPS; i++)
  {
    // End of trinket special code
    pixelStrings[i].setBrightness(maxBrightness);
    pixelStrings[i].begin();
    pixelStrings[i].show(); // Initialize all pixels to 'off'
    strip_dirty[i] = false;
  }
  // Initialize the effect queue
  for (i=0; i < effectQueueSize; ++i) {
    effectQueue[i].slotActive = false; // indicate slot is unused
    effectQueue[i].effectPtr = NULL;
    effectQueue[i].firstPixel = 0;  // first pixel involved in effect
    effectQueue[i].lastPixel = 0;   // last pixel involved in effect
    effectQueue[i].effectDelay = 0; // When effect wants time again
    effectQueue[i].r = 0;           // What color effect should use if selectable
    effectQueue[i].g = 0;           // What color effect should use if selectable
    effectQueue[i].b = 0;;          // What color effect should use if selectable
    effectQueue[i].w = 0;;          // What color effect should use if selectable
    for (j=0; j < 5; ++j) effectQueue[i].effectVar[j] = 0;// 5 integers to play with before having to allocate memory
    effectQueue[i].effectMemory = NULL; // receiver of pointer from memory allocation
    for (j=0; j < 4; ++j) {
      effectQueue[i].intParam[j] = 0;  // defined per effect
    }
    effectQueue[i].applyBrightness = false;
    effectQueue[i].effectState = 0;  // State 0 is alway init, allocate memory, set defaults
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

  const size_t capacity = BUFFER_SIZE + 60;
  DynamicJsonDocument doc(capacity);
  //  StaticJsonDocument<BUFFER_SIZE> doc;
  auto error = deserializeJson(doc, payload, length);
  JsonObject root = doc.as<JsonObject>(); // get the root object

  if (error) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(error.c_str());
    return;
  }

  const char* j_state = root["state"];
  unsigned int j_transition = root["transition"];
//  JsonObject j_color = root["color"];
  unsigned int j_white_value = root["white_value"];
  unsigned int j_brightness = root["brightness"];
  const char* j_pixel = root["pixel"];
  const char* j_effect = root["effect"];
  unsigned int j_firstPixel = root["firstPixel"];
  unsigned int j_lastPixel = root["lastPixel"];
  unsigned int j_parameter1 = root["parameter1"];
  unsigned int j_parameter2 = root["parameter2"];
  unsigned int j_parameter3 = root["parameter3"];
  unsigned int j_parameter4 = root["parameter4"];
  Serial.print(F("First Pixel"));
  Serial.println(F(""));
  Serial.println(j_firstPixel);  
  Serial.print(F("Last Pixel"));
  Serial.println(F(""));
  Serial.println(j_lastPixel);
  Serial.print(F("Param 1"));
  Serial.println(F(""));
  Serial.println(j_parameter1);

  if (strcmp(topic, "led/led/set") == 0) {
    previousEffect = effect;

    if (j_state != nullptr) {
      if (strcmp(root["state"], on_cmd) == 0) {
        stateOn = true;
        effectStart = true;
      }
      else if (strcmp(root["state"], off_cmd) == 0) {
        stateOn = false;
      }
      else {
        sendState();
      }

    }

    if (j_transition != 0) {
      transitionTime = root["transition"];
    }

    if (root.containsKey("color")) {
      JsonObject color = root["color"];
      realRed = color["r"]; // 255
      realGreen = color["g"]; // 0
      realBlue = color["b"]; // 0
      realWhite = 0;
      Serial.println(realRed);
      Serial.println(realGreen);
      Serial.println(realBlue);
    }

    if (j_white_value != 0) {
      // To prevent our power supply from having a cow. Only RGB OR White
      // Disabled because defaulting to it limits the total brightness if someone has a good power supply
      //    realRed = 0;
      //    realGreen = 0;
      //    realBlue = 0;
      realWhite = doc["white_value"];
    }

    if (j_brightness != 0) {
      brightness = doc["brightness"];
    }

    if (j_pixel != nullptr) {
      pixelLen = doc["pixel"].size();
      if (pixelLen > sizeof(pixelArray)) {
        pixelLen = sizeof(pixelArray);
      }
      for (int i = 0; i < pixelLen; i++) {
        pixelArray[i] = doc["pixel"][i];
      }
      effectStart = true;
    }

    if (j_effect != nullptr) {
      effectString = root["effect"];
      effect = effectString;
      effectStart = true;
    }

    if (j_firstPixel != 0) {
      firstPixel = doc["firstPixel"];
    }

    if (j_lastPixel != 0) {
      lastPixel = doc["lastPixel"];
    }

    if (j_parameter1 != 0) {
      effectParameter[0] = doc["parameter1"];
    }

    if (j_parameter2 != 0) {
      effectParameter[1] = doc["parameter2"];
    }

    if (j_parameter3 != 0) {
      effectParameter[2] = doc["parameter3"];
    }

    if (j_parameter4 != 0) {
      effectParameter[3] = doc["parameter4"];
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


/********************************** START SEND STATE*****************************************/
void sendState() {
  StaticJsonDocument<BUFFER_SIZE> statedoc;
  statedoc["state"] = (stateOn) ? on_cmd : off_cmd;
  JsonObject color = statedoc.createNestedObject();
  color["r"] = realRed;
  color["g"] = realGreen;
  color["b"] = realBlue;

  statedoc["white_value"] = realWhite;
  statedoc["brightness"] = brightness;
  statedoc["transition"] = transitionTime;
  statedoc["effect"] = effect.c_str();

  //  char buffer[measureJson(statedoc) + 1];
  char buffer[256];
  size_t payload = serializeJson(statedoc, buffer);

  char combinedArray[sizeof(MQTT_STATE_TOPIC_PREFIX) + sizeof(deviceName)];
  sprintf(combinedArray, "%s%s", MQTT_STATE_TOPIC_PREFIX, deviceName); // with word space
  if (!client.publish(combinedArray, buffer, payload)) {
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

// see if one pixel is between or overlaps 2 other pixels
bool insideRange (unsigned int check, unsigned int first, unsigned int last) {
  return ((first <= check) && (check <= last));
}

/********************************** START MAIN LOOP *****************************************/
void loop() {
  static bool wifiSeen = false;
  static unsigned long msgDelayStart;
  static unsigned long effectDelayStart;
  currentMilliSeconds = millis();
  unsigned int i, j;
  
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
  
  if (effectStart) {
    for (i=0; i < effectQueueSize; ++i) {
      if (effectQueue[i].slotActive) {
        // See if new effect overlaps any running effects. If so terminate them gracefully.
        if (insideRange (firstPixel, effectQueue[i].firstPixel, effectQueue[i].lastPixel) ||
           insideRange (lastPixel, effectQueue[i].firstPixel, effectQueue[i].lastPixel) ||
           insideRange (effectQueue[i].firstPixel,firstPixel, lastPixel) ||
           insideRange (effectQueue[i].lastPixel, firstPixel, lastPixel) ) 
        {
             bool finished;
          // It overlaps so terminate it.
          effectQueue[i].effectState = 1; // request termination
          finished = effectQueue[i].effectPtr(effectQueue[i]); // give effect an itteration to cleanup and finish
          effectQueue[i].slotActive = false; // Free up slot.
        }
      }
    }
    for (i=0; i < effectQueueSize; ++i) {
      if (effectQueue[i].slotActive == false) { // find a slot to put new effect in
        effectQueue[i].slotActive = true; // indicate slot is used
        effectQueue[i].firstPixel = firstPixel;   // first pixel involved in effect
        effectQueue[i].lastPixel  = lastPixel;    // last pixel involved in effect
        effectQueue[i].r = realRed;            // What color effect should use if selectable
        effectQueue[i].g = realGreen;            // What color effect should use if selectable
        effectQueue[i].b = realBlue;            // What color effect should use if selectable
        effectQueue[i].w = realWhite;            // What color effect should use if selectable
        for (j=0; j < 4; ++j) {;
          effectQueue[i].intParam[j] = effectParameter[i];  // defined per effect
        }
        effectQueue[i].applyBrightness = false;
        effectQueue[i].effectState = 0;  // State 0 is alway init, allocate memory, set defaults
        effectQueue[i].effectPtr = NULL;
        if (effect == "clear")          effectQueue[i].effectPtr = ClearEffect;
        if (effect == "solid")          effectQueue[i].effectPtr = SolidEffect;
        if (effect == "twinkle")        effectQueue[i].effectPtr = TwinkleEffect;
        if (effect == "cylon bounce")   effectQueue[i].effectPtr = CylonBounceEffect;
        if (effect == "fire")           effectQueue[i].effectPtr = FireEffect;
        if (effect == "fade in out")    effectQueue[i].effectPtr = FadeInOutEffect;
        if (effect == "strobe")         effectQueue[i].effectPtr = StrobeEffect;
        if (effect == "theater chase")  effectQueue[i].effectPtr = TheaterChaseEffect;
        if (effect == "rainbow cycle")  effectQueue[i].effectPtr = RainbowCycleEffect;
        if (effect == "color wipe")     effectQueue[i].effectPtr = ColorWipeEffect;
        if (effect == "running lights") effectQueue[i].effectPtr = RunningLightsEffect;
        if (effect == "snow sparkle")   effectQueue[i].effectPtr = SnowSparkleEffect;
        if (effect == "sparkle")        effectQueue[i].effectPtr = SparkleEffect;
        if (effect == "twinkle random") effectQueue[i].effectPtr = SetOnePixelEffect;
        if (effect == "bouncing balls") effectQueue[i].effectPtr = NoEffect;
        if (effect == "lightning")      effectQueue[i].effectPtr = NoEffect;
        break;
      }
    }
    effectStart = false;
  }
  
  // Give all running effects in queue an itteration
  for (i=0; i < effectQueueSize; ++i) {
    if (effectQueue[i].slotActive && !effectQueue[i].isOverlay) {
      bool effectFinished;
      effectFinished = effectQueue[i].effectPtr(effectQueue[i]); // give 1 itteration to effect
      if (effectFinished) { // If true effect is done so free up slot
        effectQueue[i].slotActive = false; // indicate slot is used
       }
    }
  }
   for (i=0; i < effectQueueSize; ++i) {
    if (effectQueue[i].slotActive && effectQueue[i].isOverlay) {
      bool effectFinished;
      effectFinished = effectQueue[i].effectPtr(effectQueue[i]); // give 1 itteration to effect
      if (effectFinished) { // If true effect is done so free up slot
        effectQueue[i].slotActive = false; // indicate slot is used
       }
    }
  } showStrip();
       
/*
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
  */
}
