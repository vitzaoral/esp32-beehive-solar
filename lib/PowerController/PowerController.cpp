#include "PowerController.h"

SDL_Arduino_INA3221 ina3221;

#define POWER_BANK_PIN 36

// PowerController::PowerController()
// {
//     // https://github.com/Tinyu-Zhao/INA3221/blob/main/examples/get_started/get_started.ino
//     // https://navody.dratek.cz/navody-k-produktum/senzor-napeti-a-proudu-ina3221.html
//     // ina3221.begin();
// }

void PowerController::initializeSensors()
{
    ina3221.begin();
    Serial.println("ina3221 begin");
}

void PowerController::setData()
{    
    Serial.println("Solar sensor:");
    setData(ina3221, 1, &sensor_solar);

    Serial.println("12V battery sensor:");
    setData(ina3221, 2, &sensor_battery);

    Serial.println("5V powerbank:");
    setData(ina3221, 3, &sensor_powerbank);

    int sensorValue = digitalRead(POWER_BANK_PIN);

    if (sensorValue == HIGH)
    {
        Serial.println("Powerbank 1 is connected (has energy)");
        powerBank1Connected = true;
    }
    else
    {
        Serial.println("Powerbank 1 is not connected (has no energy)");
        powerBank1Connected = false;
    }

    if (sensorValue == LOW && sensor_powerbank.loadVoltage > 0.0)
    {
        Serial.println("Powerbank 2 is connected (has energy)");
        powerBank2Connected = true;
    }
    else
    {
        Serial.println("Powerbank 2 is not connected (has no energy)");
        powerBank2Connected = false;
    }
}

void PowerController::setData(SDL_Arduino_INA3221 sensor, int channel, PowerData *data)
{
    data->shuntVoltage = sensor.getShuntVoltage_mV(channel);
    data->busVoltage = sensor.getBusVoltage_V(channel);
    data->current_mA = sensor.getCurrent_mA(channel);
    data->loadVoltage = data->busVoltage + (data->shuntVoltage / 1000.0);
    data->power_mW = data->loadVoltage * data->current_mA;

    Serial.print("Zdroj:        ");
    Serial.print(data->busVoltage);
    Serial.println(" V");
    Serial.print("Bočník:       ");
    Serial.print(data->shuntVoltage);
    Serial.println(" mV");
    Serial.print("Napětí zátěž: ");
    Serial.print(data->loadVoltage);
    Serial.println(" V");
    Serial.print("Proud zátěž:  ");
    Serial.print(data->current_mA);
    Serial.println(" mA");
    Serial.print("Výkon zátěž:  ");
    Serial.print(data->power_mW);
    Serial.println(" mW");
}
