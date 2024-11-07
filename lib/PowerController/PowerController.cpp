#include "PowerController.h"

INA3221 ina3221(INA3221_ADDR40_GND);

#define POWER_BANK_PIN 36

// PowerController::PowerController()
// {
//     // https://github.com/Tinyu-Zhao/INA3221/blob/main/examples/get_started/get_started.ino
//     // https://navody.dratek.cz/navody-k-produktum/senzor-napeti-a-proudu-ina3221.html
//     // ina3221.begin();
// }

void PowerController::initializeSensors()
{
     ina3221.begin(&Wire);
     ina3221.reset();
     ina3221.setShuntRes(100, 100, 100);
}

void PowerController::setData()
{
    Serial.println("Solar sensor:");
    setData(ina3221, INA3221_CH1, &sensor_solar);

    Serial.println("12V battery sensor:");
    setData(ina3221, INA3221_CH2, &sensor_battery);

    Serial.println("5V powerbank:");
    setData(ina3221, INA3221_CH3, &sensor_powerbank);

    int sensorValue = digitalRead(POWER_BANK_PIN);

    powerBank1Connected = (sensorValue == HIGH);
    Serial.println(powerBank1Connected ? "Powerbank 1 CONNECTED" : "Powerbank 1 DISCONECTED");

    powerBank2Connected = (sensorValue == LOW && sensor_powerbank.loadVoltage > 4.0);
    Serial.println(powerBank2Connected ? "Powerbank 2 CONNECTED" : "Powerbank 2 DISCONECTED");
}

void PowerController::setData(INA3221 sensor, ina3221_ch_t channel, PowerData *data)
{
    data->shuntVoltage = 0;
    data->busVoltage = 0;
    data->current_mA = sensor.getCurrent(channel) * 1000.0;
    data->loadVoltage = sensor.getVoltage(channel);
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
