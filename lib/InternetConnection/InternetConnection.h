#ifndef __InternetConnection_H
#define __InternetConnection_H

#include <Update.h>
#include <HTTPClient.h>
#include <MeteoData.h>
#include <PowerController.h>

class InternetConnection
{
public:
  int deepSleepInterval = 60;
  bool initializeConnection();
  void disconnect();
  void sendDataToBlynk(MeteoData, PowerController);

private:
  void setI2CStatusVersion();
};

#endif