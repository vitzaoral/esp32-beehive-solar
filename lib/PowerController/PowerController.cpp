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

    powerBank1Connected = (sensorValue == HIGH);
    Serial.println(powerBank1Connected ? "Powerbank 1 CONNECTED" : "Powerbank 1 DISCONECTED");

    powerBank2Connected = (sensorValue == LOW && sensor_powerbank.loadVoltage > 4.0);
    Serial.println(powerBank2Connected ? "Powerbank 2 CONNECTED" : "Powerbank 2 DISCONECTED");
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
    Serial.println("");
}
