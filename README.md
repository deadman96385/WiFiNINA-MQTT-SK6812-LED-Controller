# WiFiNINA MQTT JSON SK6812RGBW HomeAssistant

This project shows a super easy way to get started using Digital LED strips with [Home Assistant](https://home-assistant.io/), a sick, open-source Home Automation platform that can do just about anything. 

The code covered in this repository utilizes [Home Assistant's MQTT JSON Light Component](https://home-assistant.io/components/light.mqtt_json/) and any microcontroller that supports WiFiNINA. 

This project can run standalone. When turned on the white LEDs will light.  This means you can use the project as a normal light source. If the device can connect to WiFi and MQTT, then you are able to control remotely.

#### Supported Features Include
- RGB Color Selection
- White Selection
- Brightness 
- Effects with Animation Speed
- Over-the-Air (OTA) Upload from the ArduinoIDE!

Most of the effects incorporate the currently selected color while other effects use pre-defined colors. The input_number and automation in the HA configuration example allow you to easily set a transition speed from HA's user interface without needing to use the Services tool. 

The default speed for the effects is hard coded and is set when the light is first turned on. When changing between effects, the previously used transition speed will take over. If the effects don't look great, play around with the slider to adjust the transition speed (AKA the effect's animation speed). 

#### OTA Uploading
ArduionoOTA has native support for WiFiNINA devices and its been implemented here

#### Parts List
- [Digital RGBW Leds (SK6812RGBW)](https://www.adafruit.com/product/2842)
- More to come

#### Sample MQTT commands
Listen to MQTT commands
> mosquitto_sub -h 172.17.0.1 -t '#'

Make the full string white
> mosquitto_pub -h 172.17.0.1 -t led/kitchen/set -m "{'state': 'ON', 'white_value': 0, 'effect': 'solid', 'transition': 0, 'brightness': 255}"

Make the full string blue
> mosquitto_pub -h 172.17.0.1 -t led/kitchen/set -m "{'state': 'ON', 'color': {'r':0, 'g':0, 'b':255}, 'effect': 'solid', 'transition': 0, 'brightness': 255}"

Wipe the current effect with a color and return
> mosquitto_pub -h 172.17.0.1 -t led/kitchen/set -m "{'state': 'ON', 'color': {'r':255, 'g':0, 'b':0}, 'effect': 'color wipe once', 'transition': 10}"

Turn a specific pixel green
> mosquitto_pub -h 172.17.0.1 -t led/kitchen/set -m "{'state': 'ON', 'color': {'r':0, 'g':255, 'b':0}, 'effect': 'pixel', 'pixel': [10]}"
> mosquitto_pub -h 172.17.0.1 -t led/kitchen/set -m "{'state': 'ON', 'color': {'r':0, 'g':255, 'b':0}, 'effect': 'pixel', 'pixel': [1, 10, 12]}"

Turn on a section of the strip
> mosquitto_pub -h 172.17.0.1 -t led/kitchen/set -m "{'state': 'ON', 'color': {'r':0, 'g':255, 'b':0}, 'effect': 'pixel', 'pixel': [0, 50]}"
