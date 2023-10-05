#include "MeteoData.h"

// BME/BMP280 on address
Adafruit_BME280 bme;

void MeteoData::initializeSensors()
{
    if (!bme.begin(0x76))
    {
        Serial.println("Could not find a valid BME280 sensor on address 0x76!");
    }
    else
    {
        Serial.println("Outdoor BME280 OK");
    }
}

void MeteoData::setData()
{
    // int delayTime = 300;

    Serial.print("Outdoor sensor: ");
    sensorOutdoor.temperature = bme.readTemperature();
    sensorOutdoor.humidity = bme.readHumidity();
    sensorOutdoor.pressure = bme.readPressure() / 100.0;
    MeteoData::printSensorData(&sensorOutdoor);
}

void MeteoData::printSensorData(TempAndHumidity *data)
{
    Serial.print("temperature: " + String(data->temperature) + "°C ");
    Serial.print("humidity: " + String(data->humidity) + "% ");
    Serial.println("pressure: " + String(data->pressure) + "hPa");
}