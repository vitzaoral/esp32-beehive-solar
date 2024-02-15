#include <Arduino.h>
#include <InternetConnection.h>

InternetConnection connection;
MeteoData meteoData;
PowerController powerController;

#define POWER_BANK_PIN 36

void sendDataToInternet();

void setup()
{
  Serial.begin(115200);

  pinMode(POWER_BANK_PIN, INPUT); // 36 = GPIO36 = ADC1_CH0 read if powerbank 1 is connected (has energy)

   meteoData.initializeSensors();
   powerController.initializeSensors();

  Serial.println("Setup done");

 sendDataToInternet();
  Serial.println("Data sended, BYE");
  esp_deep_sleep(connection.deepSleepInterval * 1000000);
}

void loop()
{
  // delay(1000);
  // sendDataToInternet();
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