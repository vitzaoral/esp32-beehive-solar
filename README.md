# ESP32 beehive solar controler (WiFi version)
Online solar charger and temperature monitoring based on the ESP32 and others sensors. WiFi version.

## 🚀 BLYNK 2.0 🚀
Project is devepoled using new version of [Blynk 2.0](https://docs.blynk.io/en/)

> To build a project, you need to download all the necessary libraries and create the *settings.cpp* file in the *src* folder:
```c++
// Project settings
#include "IPAddress.h"

struct Settings
{
    const char *wifiSSID = "YYY";
    const char *wifiPassword = "ZZZ";
    const char *blynkAuth = "XXX";
    const char *version = "3.0.0"
};
```
### Features
* measuring the temperature and humidity
measuring the temperature, humidity and pressure outside the hive
* measuring the solar charger and 12V battery

### Currents list:

* [ESP32 WROOM-32](https://www.aliexpress.com/item/ESP32-ESP-32-ESP32S-ESP-32S-CP2102-Wireless-WiFi-Bluetooth-Development-Board-Micro-USB-Dual-Core/32867696371.html)
* [BMP280 digital temperature/humidity/pressure sensor](https://www.aliexpress.com/item/32849462236.html)
* Solar panel 18V with battery and solar charger or other power supply

### Save battery power:
* Set CPU frequency to 30% (80Mhz) - *setCpuFrequencyMhz(80);*
* Setup lowest WiFi power - *WiFi.setTxPower(WIFI_POWER_MINUS_1dBm);*
* Setup static IP, Gateway and DNS (quicker WiFi connection) - *WiFi.config(settings.ip, settings.gateway, settings.subnet, settings.gateway);*
* When disconnect WiFi, turn off WiFi modem - *WiFi.disconnect(true);*

### Schema:
TODO:
![Schema](https://github.com/vitzaoral/esp32-beehive-wifi/blob/master/schema/schema.png)

### Powering 5V -> 3.3V:
3v3 regulator - https://randomnerdtutorials.com/esp8266-voltage-regulator-lipo-and-li-ion-batteries/