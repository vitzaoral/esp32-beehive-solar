#ifndef __PowerController_H
#define __PowerController_H

#include <Wire.h>
#include <INA3221.h>
// #include <SDL_Arduino_INA3221.h>

struct PowerData
{
  float busVoltage;
  float shuntVoltage;
  float loadVoltage;
  float current_mA;
  float power_mW;
};

class PowerController
{
public:
  PowerData sensor_solar;
  PowerData sensor_battery;
  PowerData sensor_powerbank;
  void initializeSensors();
  void setData();
  bool powerBank1Connected;
  bool powerBank2Connected;

private:
   void setData(INA3221, ina3221_ch_t, PowerData *data);
};

#endif