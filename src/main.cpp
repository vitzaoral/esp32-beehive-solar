#include <Arduino.h>
#include <InternetConnection.h>

InternetConnection connection;
MeteoData meteoData;
PowerController powerController;

void sendDataToInternet();

void setup()
{
  Serial.begin(115200);

  meteoData.initializeSensors();

  Serial.println("Setup done");

  sendDataToInternet();
  Serial.println("Data sended, BYE");
  esp_deep_sleep(connection.deepSleepInterval * 1000000);
}

void loop()
{
}

void sendDataToInternet()
{
  Serial.println("Start initialize Blynk connection");
  if (connection.initializeConnection())
  {
    Serial.println("Setting sensors data");
    meteoData.setData();
    powerController.setData();

    Serial.println("Sending data to Blynk");
    connection.sendDataToBlynk(meteoData, powerController);
    connection.disconnect();
  }
  else
  {
    Serial.println("No internet/blynk connection");
    connection.disconnect();
  }
}