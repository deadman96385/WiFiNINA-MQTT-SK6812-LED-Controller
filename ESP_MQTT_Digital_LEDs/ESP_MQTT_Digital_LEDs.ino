
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
#define MQTT_SOCKET_TIMEOUT 3

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
byte realWhite = 50;

// Previous requested values
byte previousRed = 0;
byte previousGreen = 0;
byte previousBlue = 0;
byte previousWhite = 50;

// Values as sent to strip (effected by brightness setting)
byte red = 0;
byte green = 0;
byte blue = 0;
byte white = 50;
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
int activeEffects = 0;

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
   while (!Serial) {
     ; // wait for serial port to connect. Needed for native USB port only
   }
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

  // client.setServer(MQTT_SERVER, MQTT_PORT);
  // client.setCallback(mqttCallback);
  FillPixels(0,lastPixel, red, green, blue, white, true);
  if (debugPrint) {
    Serial.println();
    Serial.println(F("Ready"));
    Serial.print (F("End of setup Free memory = "));
    Serial.println (freeMemory ());
  }
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
    Serial.print(F("Current:"));
    Serial.print(F(" r:")); Serial.print(red); Serial.print("/"); Serial.print(realRed);
    Serial.print(F(" g:")); Serial.print(green); Serial.print("/"); Serial.print(realGreen);
    Serial.print(F(" b:")); Serial.print(blue); Serial.print("/"); Serial.print(realBlue);
    Serial.print(F(" w:")); Serial.print(white); Serial.print("/"); Serial.print(realWhite);
    Serial.print(F(" e:'")); Serial.print(effect);
    Serial.print(F("' fp:")); Serial.print(firstPixel);
    Serial.print(F(" lp:")); Serial.print(lastPixel);
    Serial.print(F(" p1:")); Serial.print(effectParameter[0]);
    Serial.print(F(" p2:")); Serial.print(effectParameter[1]);
    Serial.print(F(" p3:")); Serial.print(effectParameter[2]);
    Serial.print(F(" p4:")); Serial.print(effectParameter[3]);
    Serial.print(F(" tr:")); Serial.print(transitionTime);
    Serial.print(F(" st:")); Serial.print(stateOn);
    Serial.print(F(" ed:")); Serial.print(effectEdit);
    Serial.print(F(" ne:")); Serial.print(effectStart);
    Serial.print(F(" ov:")); Serial.print(effectIsOverlay);
    Serial.print(F(" ae:")); Serial.print(activeEffects);
    Serial.println();
  }
}

void printQueueState() {
  int i;
  if (debugPrint) {
    Serial.print(F("Active Effects:"));
    Serial.println(activeEffects);
    for (i=0; i < effectQueueSize; ++i) {
      if (effectQueue[i].slotActive) {
        Serial.print("["); Serial.print(i); Serial.print("] ");
        Serial.print(F(" r:")); Serial.print(effectQueue[i].r);
        Serial.print(F(" g:")); Serial.print(effectQueue[i].g);
        Serial.print(F(" b:")); Serial.print(effectQueue[i].b);
        Serial.print(F(" w:")); Serial.print(effectQueue[i].w);
        Serial.print(F(" e:'")); Serial.print(effectQueue[i].effectName);
        Serial.print(F("' fp:")); Serial.print(effectQueue[i].firstPixel);
        Serial.print(F(" lp:")); Serial.print(effectQueue[i].lastPixel);
        Serial.print(F(" p1:")); Serial.print(effectQueue[i].intParam[0]);
        Serial.print(F(" p2:")); Serial.print(effectQueue[i].intParam[1]);
        Serial.print(F(" p3:")); Serial.print(effectQueue[i].intParam[2]);
        Serial.print(F(" p4:")); Serial.print(effectQueue[i].intParam[3]);
        Serial.print(F(" v1:")); Serial.print(effectQueue[i].effectVar[0]);
        Serial.print(F(" v2:")); Serial.print(effectQueue[i].effectVar[1]);
        Serial.print(F(" v3:")); Serial.print(effectQueue[i].effectVar[2]);
        Serial.print(F(" v4:")); Serial.print(effectQueue[i].effectVar[3]);
        Serial.print(F(" v5:")); Serial.print(effectQueue[i].effectVar[4]);
        Serial.print(F(" effectMemory==NULL:")); Serial.print(effectQueue[i].effectMemory == NULL);

        Serial.print(F(" ov:")); Serial.print(effectQueue[i].isOverlay);
        Serial.print(F(" state:")); Serial.print(effectQueue[i].effectState);
        Serial.println();  
      }
    }
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
        realGreen = root["color"]["g"];
        realBlue = root["color"]["b"];
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
//  if (debugPrint) serializeJsonPretty(statedoc, Serial);
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

  char mqttAvailTopic[1+sizeof(MQTT_STATE_TOPIC_PREFIX) + sizeof(deviceName) + sizeof(MQTT_AVAIL_TOPIC)];
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

void printHelp() {
  Serial.println(F("Commands available"));
  Serial.println(F("fp# First Pixel\t\tlp# Last Pixel"));
  Serial.println(F("p1#\tp2#\tp3#\tp4#\tparameter n"));
  Serial.println(F("r# Red\tg# Green\tb# Blue\tw# White"));
  Serial.println(F("co#,#,#,#  ColorR,G,B,W"));
  Serial.println(F("br# Brightness"));
  Serial.println(F("ed Toggle edit flags"));
  Serial.println(F("ne New instance of current effect"));
  Serial.println(F("ov Toggle overlay setting"));
  Serial.println(F("e:<effectName>"));
  Serial.println(F("tr# Transation time"));
  Serial.println(F("st toggle the on/off state"));
  Serial.println(F("qu print state of effect queue"));
  Serial.print  (F("\t'clear'\t"));
  Serial.print  (F("\t'solid'\t"));
  Serial.print  (F("\t'twinkle'"));
  Serial.print  (F("\t'cylon bounce'\t"));
  Serial.print  (F("\t'fire'\t\t"));
  Serial.println(F("\t'fade in out'"));
  Serial.print  (F("\t'strobe'"));
  Serial.print  (F("\t'theater chase'"));
  Serial.print  (F("\t'rainbow cycle'"));
  Serial.print  (F("\t'color wipe'\t"));
  Serial.println(F("\t'running lights'"));
  Serial.print  (F("\t'snow sparkle'"));
  Serial.print  (F("\t'sparkle'"));
  Serial.print  (F("\t'set one pixel'"));
  Serial.print  (F("\t'twinkle random'"));
  Serial.print  (F("\t'bouncing balls'"));
  Serial.println(F("\t'lightning'"));
  Serial.println(F("help/? print this help"));
}



/********************************** START MAIN LOOP *****************************************/
void loop() {
  static bool wifiSeen = false;
  bool effectRan = false;
  //static unsigned long msgDelayStart;
  //static unsigned long effectDelayStart;
  currentMilliSeconds = millis();
  unsigned int i, j;
  static int cmdState = 0;
  static char cmdBuffer[50];
  static int cmdIndex = 0;

  /*
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
  */
  switch (cmdState) {
    default:
    case 0: // Send cmd prompt
      cmdIndex = 0;
      cmdState = 1; // poll for input
      Serial.print(F("CMD: "));
      break;

    case 1:
      if( Serial.available()) {
        char rcvd = Serial.read();
        if (rcvd == 10) { // new line
          cmdBuffer[cmdIndex] = 0; // terminating zero for string
          cmdState = 2; // decode command
        } else if (rcvd == 13) { // new line or carrage return
          cmdBuffer[cmdIndex] = 0; // terminating zero for string
          cmdState = 2; // decode command
        } else if (cmdIndex == 48) { // Buffer is going to overflow. Try and decode
          cmdBuffer[cmdIndex++] = rcvd;
          cmdBuffer[cmdIndex] = 0; // terminating zero for string
          cmdState = 2; // decode command
        } else {
          cmdBuffer[cmdIndex++] = rcvd;
        }
      //} else {
      //  Serial.print(".");
      }
      break;

    case 2: // decode command
      Serial.print("Got:");
      Serial.println(cmdBuffer);
      cmdState = 0;
      cmdIndex = 0;
      if (0 == strncmp("fp", cmdBuffer, 2)) { // First Pixel
        sscanf (cmdBuffer+2, "%d", &firstPixel);
      }
      else if (0 == strncmp("lp", cmdBuffer, 2)) { //
        sscanf (cmdBuffer+2, "%d", &lastPixel);
      }
      else if (0 == strncmp("p1", cmdBuffer, 2)) { //
        sscanf (cmdBuffer+2, "%d", &effectParameter[0]);
      }
      else if (0 == strncmp("p2", cmdBuffer, 2)) { //
        sscanf (cmdBuffer+2, "%d", &effectParameter[1]);
      }
      else if (0 == strncmp("p3", cmdBuffer, 2)) { //
        sscanf (cmdBuffer+2, "%d", &effectParameter[2]);
      }
      else if (0 == strncmp("p4", cmdBuffer, 2)) { //
        sscanf (cmdBuffer+2, "%d", &effectParameter[3]);
      }
      else if (0 == strncmp("col", cmdBuffer, 3)) { //
        int ri, gi, bi, wi;
        sscanf (cmdBuffer+3, "%d,%d,%d,%d", &ri, &gi, &bi, &wi);
        realRed = ri;  realGreen = gi; realBlue = bi; realWhite = wi;
        red = map(realRed, 0, 255, 0, brightness);
        green = map(realGreen, 0, 255, 0, brightness);
        blue = map(realBlue, 0, 255, 0, brightness);
        white = map(realWhite, 0, 255, 0, brightness);
      }
      else if (0 == strncmp("co", cmdBuffer, 2)) { //
        int ri, gi, bi, wi;
        sscanf (cmdBuffer+2, "%d,%d,%d,%d", &ri, &gi, &bi, &wi);
        realRed = ri;  realGreen = gi; realBlue = bi; realWhite = wi;
        red = map(realRed, 0, 255, 0, brightness);
        green = map(realGreen, 0, 255, 0, brightness);
        blue = map(realBlue, 0, 255, 0, brightness);
        white = map(realWhite, 0, 255, 0, brightness);
      }
      else if (0 == strncmp("br", cmdBuffer, 2)) { //
        sscanf (cmdBuffer+2, "%d", &brightness);
      }
      else if (0 == strncmp("r", cmdBuffer, 1)) { //
        int ri;
        sscanf (cmdBuffer+1, "%d", &ri);
        realRed = ri;
        red = map(realRed, 0, 255, 0, brightness);
      }
      else if (0 == strncmp("g", cmdBuffer, 1)) { //
        int gi;
        sscanf (cmdBuffer+1, "%d", &gi);
        realGreen = gi;
        green = map(realGreen, 0, 255, 0, brightness);
      }
      else if (0 == strncmp("b", cmdBuffer, 1)) { //
        int bi, wi;
        sscanf (cmdBuffer+1, "%d", &bi);
        realBlue = bi;
        blue = map(realBlue, 0, 255, 0, brightness);
      }
      else if (0 == strncmp("w", cmdBuffer, 1)) { //
        int wi;
        sscanf (cmdBuffer+1, "%d", &wi);
        realWhite = wi;
        white = map(realWhite, 0, 255, 0, brightness);
      }
      else if (0 == strncmp("ed", cmdBuffer, 2)) { //
        effectEdit = !effectEdit;
        Serial.print (F("Effect edit is now ")); Serial.println(effectEdit);
      }
      else if (0 == strncmp("ne", cmdBuffer, 2)) { //
        effectNew = true;
      }
      else if (0 == strncmp("ov", cmdBuffer, 2)) { //
        effectIsOverlay = !effectIsOverlay;
        Serial.print (F("Effect overlay is now ")); Serial.println(effectIsOverlay);
      }
      else if (0 == strncmp("e:", cmdBuffer, 2)) { //
        strcpy (effect, cmdBuffer+2);
      }
      else if (0 == strncmp("tr", cmdBuffer, 2)) { //
        sscanf (cmdBuffer+2, "%d", &transitionTime);
      }
      else if (0 == strncmp("st", cmdBuffer, 2)) { //
        stateOn = !stateOn;
        if (stateOn) setOn();
        else setOff();
      }
      else if (0 == strncmp("qu", cmdBuffer, 2)) { //
        printQueueState();
      }
      else if (0 == strncmp("he", cmdBuffer, 2)) { //
        printHelp();
      }
      else if (0 == strncmp("?", cmdBuffer, 1)) { //
        //printHelp();
      }
      else if (0 == strncmp("z", cmdBuffer, 1)) { // zone macros for areas of strip
        int zone = 0;
        sscanf (cmdBuffer+1, "%d", &zone);
        switch (zone) {
          default:
          case 0: // default do nothing
            break;
            
          case 1: // Main Room
            firstPixel = vStartMr;
            lastPixel  = vEndMr;
            break;
            
          case 2: // Kitchen
            firstPixel = vStartKt;
            lastPixel  = vEndKt;
            break;
            
          case 3: // Bed Room
            firstPixel = vStartBr;
            lastPixel  = vEndBr;
            break;
            
          case 4: // Above Computer
            firstPixel = 450;
            lastPixel  = 550;
            break;
            
          case 5: // Main Room
            firstPixel = 10;
            lastPixel  = 20;
            break;
            
          case 6: // Main Room
            firstPixel = 30;
            lastPixel  = 40;
            break;
            
          case 7: // Main Room
            firstPixel = 12;
            lastPixel  = 18;
            break;
            
          case 8: // Main Room
            firstPixel = 32;
            lastPixel  = 38;
            break;
        }
      } else {
        Serial.println("???");
      }
      effectStart = effectStart || effectEdit || effectNew;
      effectNew = false;
      printState();
      break;
  }
  if ((!strcmp(effect, "no effect")) && effectStart){
    // special handling to enable removal of base and overlay effects from the queue
    for (i=0; i < effectQueueSize; ++i) {
      if (effectQueue[i].slotActive) {
        // See if new effect overlaps any running effects. If so terminate them gracefully.
        if (insideRange (firstPixel, effectQueue[i].firstPixel, effectQueue[i].lastPixel) ||
           insideRange (lastPixel, effectQueue[i].firstPixel, effectQueue[i].lastPixel) ||
           insideRange (effectQueue[i].firstPixel,firstPixel, lastPixel) ||
           insideRange (effectQueue[i].lastPixel, firstPixel, lastPixel) )
        {
          if (effectQueue[i].isOverlay == effectIsOverlay) {
            // only remove effects that match current effectIsOverlay, this is how to remove overlay effects.
            // It overlaps so request it to terminate on next itteration.
            effectQueue[i].effectState = 1; // request termination
            Serial.print ("Request effect slot ["); Serial.print(i); Serial.print("] stop: ");
            Serial.println(effectQueue[i].effectName);
          }
        }
      }
    }
    effectStart = false; // Don't fill an effect slot with noEffect effect.
  }

  effectRan = false; // note if we have to ship data to strips
  if (effectStart) {
    effectStart = false;
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
        effectQueue[i].r = red;                   // What color effect should use if selectable
        effectQueue[i].g = green;                 // What color effect should use if selectable
        effectQueue[i].b = blue;                  // What color effect should use if selectable
        effectQueue[i].w = white;                 // What color effect should use if selectable
        for (j=0; j < 4; ++j) {;
          effectQueue[i].intParam[j] = effectParameter[j];  // defined per effect
        }
        effectQueue[i].applyBrightness = false;
        effectQueue[i].effectState = 0;  // State 0 is alway init, allocate memory, set defaults
        effectQueue[i].effectPtr = NULL;
        effectQueue[i].effectMemory = NULL;
        effectQueue[i].isOverlay = effectIsOverlay;
        if (!strcmp(effect, "clear"))          {
          effectQueue[i].effectPtr = ClearEffect;
          effectQueue[i].effectName = "clear";
        } else
        if (!strcmp(effect, "solid"))          {
          effectQueue[i].effectPtr = SolidEffect;
          effectQueue[i].effectName = "solid";
        } else
        if (!strcmp(effect, "twinkle"))        {
          effectQueue[i].effectPtr = TwinkleEffect;
          effectQueue[i].effectName = "twinkle";
        } else
        if (!strcmp(effect, "cylon bounce"))   {
          effectQueue[i].effectPtr = CylonBounceEffect;
          effectQueue[i].effectName = "cylon bounce";
        } else
        if (!strcmp(effect, "fire"))           {
          effectQueue[i].effectPtr = FireEffect;
          effectQueue[i].effectName = "fire";
        } else
        if (!strcmp(effect, "fade in out"))    {
          effectQueue[i].effectPtr = FadeInOutEffect;
          effectQueue[i].effectName = "fade in out";
        } else
        if (!strcmp(effect, "strobe"))         {
          effectQueue[i].effectPtr = StrobeEffect;
          effectQueue[i].effectName = "strobe";
        } else
        if (!strcmp(effect, "theater chase"))  {
          effectQueue[i].effectPtr = TheaterChaseEffect;
          effectQueue[i].effectName = "theater chase";
        } else
        if (!strcmp(effect, "rainbow cycle"))  {
          effectQueue[i].effectPtr = RainbowCycleEffect;
          effectQueue[i].effectName = "rainbow cycle";
        } else
        if (!strcmp(effect, "color wipe"))     {
          effectQueue[i].effectPtr = ColorWipeEffect;
          effectQueue[i].effectName = "color wipe";
        } else
        if (!strcmp(effect, "running lights")) {
          effectQueue[i].effectPtr = RunningLightsEffect;
          effectQueue[i].effectName = "running lights";
        } else
        if (!strcmp(effect, "snow sparkle"))   {
          effectQueue[i].effectPtr = SnowSparkleEffect;
          effectQueue[i].effectName = "snow sparkle";
        } else
        if (!strcmp(effect, "sparkle"))        {
          effectQueue[i].effectPtr = SparkleEffect;
          effectQueue[i].effectName = "sparkle";
        } else
        if (!strcmp(effect, "set one pixel"))  {
          effectQueue[i].effectPtr = SetOnePixelEffect;
          effectQueue[i].effectName = "set one pixel";
        } else
        if (!strcmp(effect, "twinkle random")) {
          effectQueue[i].effectPtr = TwinkleRandomEffect;
          effectQueue[i].effectName = "twinkle random";
        } else
        if (!strcmp(effect, "bouncing balls")) {
          effectQueue[i].effectPtr = BouncingBallsEffect;
          effectQueue[i].effectName = "bouncing balls";
        } else
        if (!strcmp(effect, "lightning"))      {
          effectQueue[i].effectPtr = LightingingEffect;
          effectQueue[i].effectName = "lightning";
        } else
        if (!strcmp(effect, "no effect"))      {
          effectQueue[i].effectPtr = NoEffect;
          effectQueue[i].effectName = "no effect";
        } else {
          Serial.print("Unknown effect: "); Serial.println(effect);
          effectQueue[i].slotActive = false; // release slot
        }

        break;
      }
    }
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
  }
}
