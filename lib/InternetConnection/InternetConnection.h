#ifndef __InternetConnection_H
#define __InternetConnection_H

#include <Update.h>
#include <HTTPClient.h>
#include <MeteoData.h>
#include <PowerController.h>
#include <BleAdvParser.h>

class InternetConnection
{
public:
  int deepSleepInterval = 60;
  bool initializeConnection();
  void disconnect();
  void sendDataToBlynk(MeteoData, PowerController);
  // BLE teploměry: HTTP kód z beeSysu (záporný = chyba spojení, 0 = neposláno)
  int sendBleToBeesys(const BleSensorStore &store, uint32_t scanMillis);
  void sendBleToBlynk(const BleSensorStore &store, int beesysCode, const char *otaState);
  // Perioda poslechu BLE z Blynku (V58) v minutách, -1 = Blynk nic neposlal
  int blePeriodFromBlynk();

private:
  void setI2CStatusVersion();
};

#endif