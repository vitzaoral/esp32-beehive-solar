#ifndef __MeteoData_H
#define __MeteoData_H

#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>


struct TempAndHumidity
{
    float temperature;
    float humidity;
    float pressure;
};

class MeteoData
{
  public:
    TempAndHumidity sensorOutdoor;
    void initializeSensors();
    void setData();

  private:
    void printSensorData(TempAndHumidity *data);
};

#endif
