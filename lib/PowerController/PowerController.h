#ifndef __PowerController_H
#define __PowerController_H

#include <Wire.h>
#include <INA3221.h>

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
  PowerController();
  PowerData sensor_solar;
  PowerData sensor_battery;
  void setData();

private:
   void setData(INA3221, ina3221_ch_t, PowerData *data);
};

#endif