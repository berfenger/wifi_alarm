# MQTT Tuya Wifi Alarm siren

The purpose of this esphome based firmware is to provide a replacement of the official ESP8266 firmware of Tuya Wifi Alarm sirens. This software works fully offline and can be controlled via MQTT instead of official Tuya Cloud compatible apps.

This devices can be bought on some chinese retailers. [Example](https://es.aliexpress.com/item/1005001431986619.html)

A Tuya wifi alarm siren has 2 modules:
  * Tuya MCU: manages all device behavior.
  * ESP8266: act as a gateway between Tuya MCU and Tuya Cloud.

Both modules communicate via serial port (UART).

This software is a replacement for the ESP8266 and allows you to control the MCU trough MQTT.

## Advantages
  * Can be controlled offline without internet connection.
  * Can be integrated with other home automation controllers through MQTT (e.g. Home Assistant).
  * Improved security. Are there any secure cloud alarms?
  * You have the control.

## Disadvantages
  * You have to flash the ESP8266, and it's not as easy as I expected. Doable, but a little tricky.

## Alarm features not supported
  * Notifications from the MCU are not supported. The MCU has the ability to send text messages designed to end up on your mobile device. But this notifications are just ignored.
  * Accesories on zone "24 hours silent", because this accessory trigger just generate a notification, wich is ignored.
  * Accessory/remote list does not work reliably. If devices are not listed, a factory reset should fix it.

## How to flash the ESP8266
[Flash guide](./doc/flash_guide.md)

## MQTT Protocol
A description of the MQTT protocol can be found [here](./doc/mqtt_protocol.md).

## Licensing

Arturo Casal

Shield: [![CC BY 4.0][cc-by-shield]][cc-by]

This work is licensed under a
[Creative Commons Attribution 4.0 International License][cc-by].

[![CC BY 4.0][cc-by-image]][cc-by]

[cc-by]: http://creativecommons.org/licenses/by/4.0/
[cc-by-image]: https://i.creativecommons.org/l/by/4.0/88x31.png
[cc-by-shield]: https://img.shields.io/badge/License-CC%20BY%204.0-lightgrey.svg

#### This software uses ESPHome

Copyright (c) 2019 ESPHome