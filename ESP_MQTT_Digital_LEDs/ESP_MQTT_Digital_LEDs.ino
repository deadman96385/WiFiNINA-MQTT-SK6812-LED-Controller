
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

// Adafruit_NeoPixel Strip;
#define NUMSTRIPS 10
#define effectQueueSize 50

bool debugPrint = true;
/****************************************FOR JSON***************************************/
const int BUFFER_SIZE = JSON_OBJECT_SIZE(60);
const char* birthMessage = "online";
const char* lwtMessage = "offline";

/*********************************** MQTT LED variable Defintions ********************************/
// variables for values sent and received via MQTT
byte realRed = 0;
byte realGreen = 0;
byte realBlue = 0;
byte realWhite = 255;

// Previous requested values
byte previousRed = 0;
byte previousGreen = 0;
byte previousBlue = 0;
byte previousWhite = 0;

// Values as sent to strip (effected by brightness setting)
byte red = 0;
byte green = 0;
byte blue = 0;
byte white = 0;
int brightness = 255;
String previousEffect = "solid";
//String effect = "solid";
char effect[20]; // 20 is longer then longest effect name hopefully
bool effectStart = false;
bool effectEdit = false; // Should every json key edit effect queued events?
bool effectNew = false; // if effectEdit is false then if effectNew is true a new effect will be queued up.
bool effectIsOverlay = false; // Does this effect overlay and not replace currently running effects. Overlays run last.
unsigned int firstPixel = 0;
unsigned int lastPixel = ledCount;
bool stateOn = true;
unsigned int transitionTime = 50; // 1-150
unsigned int pixelLen = 1;
unsigned int pixelArray[50];
unsigned int effectParameter[4] = {5, 10, 1250, 1391};

/* Constants used by MQTT */
const char* on_cmd = "ON";
const char* off_cmd = "OFF";

/************** Wifi support variables  *********/
unsigned long nextMqttConnectTime = 0;
const unsigned long fiveSeconds = 5000;
const unsigned long tenSeconds = 10000;
WiFiClient net;
PubSubClient client(net);

/******************************** OTHER GLOBALS *******************************/
unsigned long currentMilliSeconds;

/*********************************** Physical LED strips information ******************/
// Informatin needed to present 1 large virutal strip to rest of program
Adafruit_NeoPixel pixelStrings[NUMSTRIPS] = {
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
bool stripDirty[NUMSTRIPS] = {
  false,  // strip 0
  false,  // strip 1
  false,  // strip 2
  false,  // strip 3
  false,  // strip 4
  false,  // strip 5
  false,  // strip 6
  false,  // strip 7
  false,  // strip 8
  false   // strip 9
};
#include "NeoPixel_Effects.h"

/******************* support variables for queue of running effects ************/
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
  if (debugPrint) Serial.println ();
  if (debugPrint) Serial.print (F("Initial Free memory = "));
  if (debugPrint) Serial.println (freeMemory ());
  // if analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  //randomSeed(analogRead(0));

  // Turn on power supply for LED strips. Controller runs on standby power from power supply
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  
  strcpy (effect, "solid"); // init effect variable

  for (i = 0; i < NUMSTRIPS; i++)
  {
    // End of trinket special code
    pixelStrings[i].setBrightness(maxBrightness);
    pixelStrings[i].begin();
    pixelStrings[i].show(); // Initialize all pixels to 'off'
    stripDirty[i] = false;
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
  if (debugPrint) Serial.println(F("Ready"));
  if (debugPrint) Serial.print (F("End of setup Free memory = "));
  if (debugPrint) Serial.println (freeMemory ());
}


/********************************** START SETUP WIFI *****************************************/
bool setup_wifi() {
  static unsigned long wifiDelayEnd = 0;
  static bool waitingForModule = false;
  static bool waitingForConnect = false;

  if (WiFi.status() == WL_NO_MODULE) {
    if (!waitingForModule) {
      waitingForModule = true;
      wifiDelayEnd = currentMilliSeconds + tenSeconds;
      if (debugPrint) Serial.println(F("Communication with WiFi module failed!"));
      return false;
    } else if (currentMilliSeconds >= wifiDelayEnd) {
      if (debugPrint) Serial.println(F("Communication with WiFi module failed! 10 second delay finished."));
      WiFi.init();
      wifiDelayEnd = currentMilliSeconds + tenSeconds;
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
      if (debugPrint) Serial.println(F("WiFi connected"));
      if (debugPrint) Serial.print(F("IP address: "));
      if (debugPrint) Serial.println(WiFi.localIP());
      return true;
    } else if (currentMilliSeconds >= wifiDelayEnd) {
      // 10 second timeout so attempt again
      wifiDelayEnd = currentMilliSeconds + tenSeconds;
      if (debugPrint) Serial.print(F("Attempting again to connect to WPA SSID: "));
      if (debugPrint) Serial.println(WIFI_SSID);
      // Connect to WPA/WPA2 network:
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      return false;
    }  // else we are in the 10 second wait so do nothing
  } else if (WiFi.status() != WL_CONNECTED) {
    waitingForConnect = true;
    wifiDelayEnd = currentMilliSeconds + tenSeconds;
    if (debugPrint) Serial.print(F("Attempting to connect to WPA SSID: "));
    if (debugPrint) Serial.println(WIFI_SSID);
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

void printState() {
  if (debugPrint) {
    Serial.print(F("Current param state : "));
    Serial.print(F(" R:")); Serial.print(red); Serial.print("/"); Serial.print(realRed);
    Serial.print(F(" G:")); Serial.print(green); Serial.print("/"); Serial.print(realGreen);
    Serial.print(F(" B:")); Serial.print(blue); Serial.print("/"); Serial.print(realBlue);
    Serial.print(F(" W:")); Serial.print(white); Serial.print("/"); Serial.print(realWhite);
    Serial.print(F(" E:")); Serial.print(effect);
    Serial.print(F(" f:")); Serial.print(firstPixel);
    Serial.print(F(" l:")); Serial.print(lastPixel);
    Serial.print(F(" 1:")); Serial.print(effectParameter[0]);
    Serial.print(F(" 2:")); Serial.print(effectParameter[1]);
    Serial.print(F(" 3:")); Serial.print(effectParameter[2]);
    Serial.print(F(" 4:")); Serial.print(effectParameter[3]);
    Serial.print(F(" T:")); Serial.print(transitionTime);
    Serial.print(F(" S:")); Serial.print(stateOn);
    Serial.print(F(" edit:")); Serial.print(effectEdit);
    Serial.print(F(" new:")); Serial.print(effectNew);
    Serial.print(F(" o:")); Serial.print(effectIsOverlay);
    // Serial.print(F(" T:")); Serial.print(transitionTime);
    Serial.println();
  }
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
  // setAll(0, 0, 0, 0);
  // stateOn = false;
  // previousRed = 0;
  // previousGreen = 0;
  // previousBlue = 0;
  // previousWhite = 0;
  if (digitalRead(10) == HIGH) digitalWrite(10, LOW); // Turn off LED power supply
}

void setOn() {
  if (digitalRead(10) == LOW) { // if power supply was off
    digitalWrite(10, HIGH); // turn on power supply
    delay (250); // wait 1/4 second
    FillPixels (0, lastPixel, 0, 0, 0, 50, true); // send dim white to entire virtual strip
  }
  //setAll(0, 0, 0, 0);
  //stateOn = true;
}

/********************************** START CALLBACK*****************************************/

void mqttCallback(char* topic, byte* payload, unsigned int length) {

  const char *theKey;
  if (debugPrint) {
    Serial.println();
    Serial.print(F("Message arrived ["));
    Serial.print(topic);
    Serial.print(F("] "));
    Serial.write(payload, length);
    Serial.println();
  }

  // char message[length + 1];
  // for (unsigned int i = 0; i < length; i++) {
    // message[i] = (char)payload[i];
  // }

  // message[length] = '\0';
  // if (debugPrint) Serial.println(message);

  const size_t capacity = BUFFER_SIZE + 60;
  //DynamicJsonDocument doc(capacity); // when is the heap allocation released / freed
  StaticJsonDocument<BUFFER_SIZE> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    if (debugPrint) {
      Serial.print(F("deserializeJson() failed with code "));
      Serial.println(error.c_str());
    }
    return;
  }
  JsonObject root = doc.as<JsonObject>(); // get the root object
  if (debugPrint) Serial.println(F("Payload contains these keys"));
  for (JsonPair p : root) {
    theKey = p.key().c_str();
    if (debugPrint) {
      Serial.print(theKey); Serial.print(":");
      // We only have string or number values for keys so test for string
      if (p.value().is<char*>()) { // do we have a string?
        Serial.println(p.value().as<char*>());
      } else {
        Serial.println(p.value().as<int>());
      }
    }
    if (strcmp(topic, "led/led/set") == 0) {
       if (!strcmp("state", theKey)) {
        if (!strcmp(on_cmd, p.value().as<char*>())){
          stateOn = true;
          setOn();
        } else {
          stateOn = false;
          setOff();
        }
      } 
      else if (!strcmp("color", theKey)) {
        realRed = root["color"]["r"];
        realRed = root["color"]["r"];
        realRed = root["color"]["r"];
        red = map(realRed, 0, 255, 0, brightness);
        green = map(realGreen, 0, 255, 0, brightness);
        blue = map(realBlue, 0, 255, 0, brightness);
      }
      else if (!strcmp("white_value", theKey)) {
        realWhite = p.value();
        white = map(realWhite, 0, 255, 0, brightness);
      }
      else if (!strcmp("effect", theKey)) {
        strcpy(effect, p.value().as<char*>()); 
        effectStart = true;
      }
      else if (!strcmp("firstPixel", theKey))  firstPixel = p.value();
      else if (!strcmp("lastPixel", theKey))   lastPixel = p.value();
      else if (!strcmp("parameter1", theKey))  effectParameter[0] = p.value();
      else if (!strcmp("parameter2", theKey))  effectParameter[1] = p.value();
      else if (!strcmp("parameter3", theKey))  effectParameter[2] = p.value();
      else if (!strcmp("parameter4", theKey))  effectParameter[3] = p.value();
      else if (!strcmp("edit", theKey))        effectEdit = p.value();
      else if (!strcmp("new", theKey))         effectNew = p.value();
      else if (!strcmp("overlay", theKey))     effectIsOverlay = p.value();
      else if (!strcmp("transition", theKey))  transitionTime = p.value();
      else if (!strcmp("brightness", theKey)) {
        brightness = p.value();
        red = map(realRed, 0, 255, 0, brightness);
        green = map(realGreen, 0, 255, 0, brightness);
        blue = map(realBlue, 0, 255, 0, brightness);
        white = map(realWhite, 0, 255, 0, brightness);
      }
      else if (!strcmp("debug", theKey))  debugPrint = p.value();
    }
  }
  effectStart = effectStart || effectEdit || effectNew;
  effectNew = false;
  if (debugPrint) Serial.println(F("End of key pairs"));

   //const char* j_state = root["state"];

  if (strcmp(topic, "led/led/set") == 0) {
/*
    if (j_state != nullptr) {
      if (strcmp(root["state"], on_cmd) == 0) {
        stateOn = true;
        //effectStart = true;
      } else {
      //else if (strcmp(root["state"], off_cmd) == 0) {
        stateOn = false;
      }
      // else {
        // sendState();
      // }

    }
    if (root.containsKey("color")) {
      JsonObject color = root["color"];
      realRed = color["r"]; // 255
      realGreen = color["g"]; // 0
      realBlue = color["b"]; // 0
      //realWhite = 0;        // This may have been issue with keeping white and color on at same time
      Serial.println(realRed);
      Serial.println(realGreen);
      Serial.println(realBlue);
    }
    */
    /*

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

*/

/*
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
*/
/*
    if (stateOn) {
      setOn();
    } else {
      setOff(); // NOTE: Will change transitionDone
    }
*/
    previousEffect = effect;
    previousRed = red;
    previousGreen = green;
    previousBlue = blue;
    previousWhite = white;    
    sendState();  // This will overwrite topic* and payload* buffers.
    printState();
  }

}


/********************************** START SEND STATE*****************************************/
bool sendState() {
  bool returnValue = true;
  StaticJsonDocument<BUFFER_SIZE> statedoc;
  statedoc["state"] = (stateOn) ? on_cmd : off_cmd;
  statedoc["color"]["r"] = realRed;
  statedoc["color"]["g"] = realGreen;
  statedoc["color"]["b"] = realBlue;
  // JsonObject color = statedoc.createNestedObject();
  // color["r"] = realRed;
  // color["g"] = realGreen;
  // color["b"] = realBlue;

  statedoc["white_value"] = realWhite;
  statedoc["brightness"] = brightness;
  statedoc["transition"] = transitionTime;
  statedoc["effect"] = effect; //.c_str();

  //  char buffer[measureJson(statedoc) + 1];
  char buffer[256];
  size_t payload = serializeJson(statedoc, buffer);
  if (debugPrint) serializeJsonPretty(statedoc, Serial);
  if (debugPrint) Serial.println(buffer);

  char combinedArray[sizeof(MQTT_STATE_TOPIC_PREFIX) + sizeof(deviceName)+1]; // plus 1 for terminating 0
  sprintf(combinedArray, "%s%s", MQTT_STATE_TOPIC_PREFIX, deviceName); // with word space
  if (!client.publish(combinedArray, buffer, payload)) {
    Serial.println(F("Failed to publish state to MQTT. Check if you updated your MQTT_MAX_PACKET_SIZE"));
    returnValue = false;
  }
  return returnValue;
}


/********************************** START RECONNECT *****************************************/
void attemptReconnect() {
  // Loop until we're reconnected
  //  while (!client.connected()) {
  if (debugPrint) Serial.print(F("Attempting MQTT connection..."));

  char mqttAvailTopic[sizeof(MQTT_STATE_TOPIC_PREFIX) + sizeof(deviceName) + sizeof(MQTT_AVAIL_TOPIC)];
  sprintf(mqttAvailTopic, "%s%s%s", MQTT_STATE_TOPIC_PREFIX, deviceName, MQTT_AVAIL_TOPIC); // with word space

  // Attempt to connect
  if (client.connect(deviceName, MQTT_USER, MQTT_PASSWORD, mqttAvailTopic, 0, true, lwtMessage)) {
    if (debugPrint) Serial.println(F("connected"));

    // Publish the birth message on connect/reconnect
    if (!client.publish(mqttAvailTopic, birthMessage, true)) {
      Serial.println(F("Failed to publish birth"));
    }

    char combinedArray[sizeof(MQTT_STATE_TOPIC_PREFIX) + sizeof(deviceName) + 4 + 1]; // +4 for /set +1 for terminating 0
    sprintf(combinedArray, "%s%s/set", MQTT_STATE_TOPIC_PREFIX, deviceName); // with word space
    if (!client.subscribe(combinedArray)) {
      Serial.println(F("Failed to subscribe to set"));
    }
    sendState();
  } else {
    if (debugPrint) Serial.print(F("failed, rc="));
    if (debugPrint) Serial.print(client.state());
    if (debugPrint) Serial.println(F(" try again in 5 seconds"));
    // Wait 5 seconds before retrying
    //delay(5000);
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
  bool effectRan = false;
  //static unsigned long msgDelayStart;
  //static unsigned long effectDelayStart;
  currentMilliSeconds = millis();
  unsigned int i, j, activeEffects;
  
  if ((WiFi.status() != WL_CONNECTED) || !wifiSeen) {
    //    delay(1);
    wifiSeen = setup_wifi();
  }

  if (wifiSeen) {
    // Bring MQTT on line and process messages
    if (!client.connected()) {
      if (currentMilliSeconds >= nextMqttConnectTime) { // enforce 5 second rule without stopping everything else
        attemptReconnect();
        nextMqttConnectTime = currentMilliSeconds + fiveSeconds;
      }
      if (debugPrint) Serial.println ();
      if (debugPrint) Serial.println (freeMemory ());
    } else {
      client.loop(); // Check MQTT
    }
    //ArduinoOTA.poll();
  }
  
  effectRan = false; // note if we have to ship data to strips
  if (effectStart) {
    if (!effectIsOverlay) {
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
            effectRan = true;
          }
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
        effectQueue[i].isOverlay = effectIsOverlay;
        if (!strcmp(effect, "clear"))          effectQueue[i].effectPtr = ClearEffect;
        if (!strcmp(effect, "solid"))          effectQueue[i].effectPtr = SolidEffect;
        if (!strcmp(effect, "twinkle"))        effectQueue[i].effectPtr = TwinkleEffect;
        if (!strcmp(effect, "cylon bounce"))   effectQueue[i].effectPtr = CylonBounceEffect;
        if (!strcmp(effect, "fire"))           effectQueue[i].effectPtr = FireEffect;
        if (!strcmp(effect, "fade in out"))    effectQueue[i].effectPtr = FadeInOutEffect;
        if (!strcmp(effect, "strobe"))         effectQueue[i].effectPtr = StrobeEffect;
        if (!strcmp(effect, "theater chase"))  effectQueue[i].effectPtr = TheaterChaseEffect;
        if (!strcmp(effect, "rainbow cycle"))  effectQueue[i].effectPtr = RainbowCycleEffect;
        if (!strcmp(effect, "color wipe"))     effectQueue[i].effectPtr = ColorWipeEffect;
        if (!strcmp(effect, "running lights")) effectQueue[i].effectPtr = RunningLightsEffect;
        if (!strcmp(effect, "snow sparkle"))   effectQueue[i].effectPtr = SnowSparkleEffect;
        if (!strcmp(effect, "sparkle"))        effectQueue[i].effectPtr = SparkleEffect;
        if (!strcmp(effect, "twinkle random")) effectQueue[i].effectPtr = SetOnePixelEffect;
        if (!strcmp(effect, "bouncing balls")) effectQueue[i].effectPtr = NoEffect;
        if (!strcmp(effect, "lightning"))      effectQueue[i].effectPtr = NoEffect;
        break;
      }
    }
    effectStart = false;
  }
  
  // Give all running effects in queue an itteration
  activeEffects = 0;
  for (i=0; i < effectQueueSize; ++i) {
    if (effectQueue[i].slotActive && !effectQueue[i].isOverlay) {
      effectRan = true; ++activeEffects;
      bool effectFinished;
      effectFinished = effectQueue[i].effectPtr(effectQueue[i]); // give 1 itteration to effect
      if (effectFinished) { // If true effect is done so free up slot
        effectQueue[i].slotActive = false; // indicate slot is used
       }
    }
  }
   for (i=0; i < effectQueueSize; ++i) {
    if (effectQueue[i].slotActive && effectQueue[i].isOverlay) {
      effectRan = true; ++activeEffects;
      bool effectFinished;
      effectFinished = effectQueue[i].effectPtr(effectQueue[i]); // give 1 itteration to effect
      if (effectFinished) { // If true effect is done so free up slot
        effectQueue[i].slotActive = false; // indicate slot is used
       }
    }
  } 
  if (effectRan) {
    showStrip();
    effectRan = false;
    if (debugPrint) {
      Serial.print("Active Effects:"); 
      Serial.println(activeEffects);
    }
  }
}
