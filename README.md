# ESP32 beehive solar controler (WiFi version)
Online solar charger and temperature monitoring based on the ESP32. PWM solar charger charge 12v lead acid battery. YX850 switches two big 5v powerbanks (60Ah) and is measured with INA3221.

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
* measuring the solar charger and 12V battery
* measuring powerbanks

### Currents list:

* ESP32 WROOM-32
* SHT31 temperature & humidity sensor
* INA3221 sensor
* 2x 18V 40W Solar panel
* PWM solar charger 10A
* 12V 60Ah lead acid battery
* YX850 5V Automatic Switching UPS

### Schema:
![Schema](https://raw.githubusercontent.com/vitzaoral/esp32-beehive-solar/master/schema/schema.png)
