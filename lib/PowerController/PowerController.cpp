#include "PowerController.h"

INA3221 ina3221(INA3221_ADDR40_GND);

PowerController::PowerController()
{
    // https://github.com/Tinyu-Zhao/INA3221/blob/main/examples/get_started/get_started.ino
    // https://navody.dratek.cz/navody-k-produktum/senzor-napeti-a-proudu-ina3221.html
    ina3221.begin();
    ina3221.reset();

    // Set shunt resistors to 10 mOhm for all channels
    ina3221.setShuntRes(10, 10, 10);

    // Set series filter resistors to 10 Ohm for all channels.
    // Series filter resistors introduce error to the current measurement.
    // The error can be estimated and depends on the resitor values and the bus
    // voltage.
    ina3221.setFilterRes(10, 10, 10);
}

void PowerController::setData()
{
    Serial.println("Solar sensor:");
    setData(ina3221, INA3221_CH1, &sensor_solar);

    Serial.println("12V battery sensor:");
    setData(ina3221, INA3221_CH2, &sensor_battery);
}

void PowerController::setData(INA3221 sensor, ina3221_ch_t channel, PowerData *data)
{
    data->shuntVoltage = sensor.getShuntVoltage(channel);
    data->busVoltage = 0;
    data->current_mA = sensor.getCurrent(channel);
    data->loadVoltage = sensor.getVoltage(channel);
    data->power_mW = data->loadVoltage * data->current_mA;

    Serial.print("Bus Voltage:   ");
    Serial.print(data->busVoltage);
    Serial.println(" V");
    Serial.print("Shunt Voltage: ");
    Serial.print(data->shuntVoltage);
    Serial.println(" mV");
    Serial.print("Load Voltage:  ");
    Serial.print(data->loadVoltage);
    Serial.println(" V");
    Serial.print("Current:       ");
    Serial.print(data->current_mA);
    Serial.println(" mA");
    Serial.print("Power:         ");
    Serial.print(data->power_mW);
    Serial.println(" mW");
}
