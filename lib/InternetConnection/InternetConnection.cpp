#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPLalHw40y_"
#define BLYNK_TEMPLATE_NAME "Vcely baterky"
#define BLYNK_FIRMWARE_VERSION "3.0.0"

#include "InternetConnection.h"
#include <BlynkSimpleEsp32.h>
#include "../../src/settings.cpp"

// hard reset
#include <esp_int_wdt.h>
#include <esp_task_wdt.h>

Settings settings;

#define SDA 21
#define SCL 22
#define LED_GREEN "#10b34e"
#define LED_RED "#FF0000"

WidgetLED led1(V47);
WidgetLED led2(V48);

// Synchronize settings from Blynk server with device when internet is connected
BLYNK_CONNECTED()
{
    Blynk.syncAll();
}

String overTheAirURL = "";

BLYNK_WRITE(InternalPinOTA)
{
    overTheAirURL = param.asString();

    Serial.println("OTA Started");
    overTheAirURL = param.asString();
    Serial.print("overTheAirURL = ");
    Serial.println(overTheAirURL);

    HTTPClient http;
    http.begin(overTheAirURL);
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK)
    {
        Serial.println("Bad httpCode");
        return;
    }
    int contentLength = http.getSize();
    if (contentLength <= 0)
    {
        Serial.println("No contentLength");
        return;
    }
    bool canBegin = Update.begin(contentLength);
    if (!canBegin)
    {
        Serial.println("Can't begin update");
        return;
    }
    Client &client = http.getStream();
    int written = Update.writeStream(client);
    if (written != contentLength)
    {
        Serial.println("Bad contentLength");
        return;
    }
    if (!Update.end())
    {
        Serial.println("Update not ended");
        return;
    }
    if (!Update.isFinished())
    {
        Serial.println("Update not finished");
        return;
    }

    Serial.println("Update OK");
    ESP.restart();
}

// Restart ESP
BLYNK_WRITE(V0)
{
    if (param.asInt())
    {
        Blynk.virtualWrite(V0, false);

        // TODO: refactor, odstranit? https://github.com/espressif/arduino-esp32/issues/1563#issuecomment-401560601
        Serial.println("Check I2C");
        if (!digitalRead(SDA) || !digitalRead(SCL))
        { // bus in busy state
            log_w("invalid state sda=%d, scl=%d\n", digitalRead(SDA), digitalRead(SCL));
            Serial.print("invalid state SDA: ");
            Serial.println(digitalRead(SDA));
            Serial.print("invalid state SCL: ");
            Serial.println(digitalRead(SCL));
            digitalWrite(SDA, HIGH);
            digitalWrite(SCL, HIGH);
            delayMicroseconds(5);
            digitalWrite(SDA, HIGH);
            for (uint8_t a = 0; a < 9; a++)
            {
                delayMicroseconds(5);
                digitalWrite(SCL, LOW);
                delayMicroseconds(5);
                digitalWrite(SCL, HIGH);
                if (digitalRead(SDA))
                { // bus recovered, all done. resync'd with slave
                    break;
                }
            }
        }
        else
        {
            Serial.println("I2C OK");
        }
        Serial.println("Restarting, bye..");
        ESP.restart();
    }
}

int deepSleepIntervalStatic = 0;

// deep sleep interval in seconds
BLYNK_WRITE(V3)
{
    if (param.asInt())
    {
        deepSleepIntervalStatic = param.asInt();
        Serial.println("Deep sleep interval was set to: " + String(deepSleepIntervalStatic));
    }
}

bool InternetConnection::initializeConnection()
{
    int connAttempts = 0;
    Serial.println("\r\nTry connecting to: " + String(settings.wifiSSID));

    // save battery power, set lowest WiFi power
    WiFi.setTxPower(WIFI_POWER_MINUS_1dBm);
    delay(1);

    // try config - quicker for WiFi connection
    // WiFi.config(settings.ip, settings.gateway, settings.subnet, settings.gateway);

    WiFi.begin(settings.wifiSSID, settings.wifiPassword);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        if (connAttempts > 20)
        {
            Serial.println("Error - couldn't connect to WIFI");
            return false;
        }

        connAttempts++;
    }

    delay(1000);

    Serial.println("\r\nConnecting to WIFI OK, connnecting to Blynk");

    if (!Blynk.connected())
    {
        Blynk.config(settings.blynkAuth);
        // timeout 6sec
        Blynk.connect(2000);
    }

    return Blynk.connected();
}

void InternetConnection::disconnect()
{
    if (WiFi.isConnected())
    {
        Blynk.disconnect();
        WiFi.disconnect(true);

        Serial.println("Disconnected OK");
    }
    else
    {
        Serial.println("Already disconnected");
    }
}

void InternetConnection::sendDataToBlynk(MeteoData meteoData, PowerController powerController)
{
    // create data to send to Blynk
    if (Blynk.connected())
    {
        // sync Blynk in 3 sec
        for (int loop_count = 0; loop_count < 30; loop_count++)
        {
            Blynk.run();
            delay(100);
            Serial.print(".");
        }

        // solar power data
        Blynk.virtualWrite(V8, powerController.sensor_solar.loadVoltage);
        Blynk.virtualWrite(V9, powerController.sensor_solar.current_mA);
        Blynk.virtualWrite(V10, powerController.sensor_solar.power_mW / 1000.0);

        // 12V battery power data
        Blynk.virtualWrite(V41, powerController.sensor_battery.loadVoltage);
        Blynk.virtualWrite(V42, powerController.sensor_battery.current_mA);
        Blynk.virtualWrite(V43, powerController.sensor_battery.power_mW / 1000.0);

        // powerbank power data
        Blynk.virtualWrite(V44, powerController.sensor_powerbank.loadVoltage);
        Blynk.virtualWrite(V45, powerController.sensor_powerbank.current_mA);
        Blynk.virtualWrite(V46, powerController.sensor_powerbank.power_mW / 1000.0);

        // powerbanks leds
        led1.on();
        led2.on();
        led1.setColor(powerController.powerBank1Connected == 1 ? LED_GREEN : LED_RED);
        led2.setColor(powerController.powerBank2Connected == 1 ? LED_GREEN : LED_RED);

        // set SDA/SCL status
        setI2CStatusVersion();

        // outdoor temperature sensor
        Blynk.virtualWrite(V22, meteoData.sensorOutdoor.humidity);
        Blynk.virtualWrite(V23, meteoData.sensorOutdoor.temperature);

        // WIFI info
        Blynk.virtualWrite(V39, "IP: " + WiFi.localIP().toString() + "|G: " + WiFi.gatewayIP().toString() + "|S: " + WiFi.subnetMask().toString() + "|DNS: " + WiFi.dnsIP().toString());
        Blynk.virtualWrite(V40, WiFi.RSSI());

        if (deepSleepIntervalStatic > 0)
        {
            deepSleepInterval = deepSleepIntervalStatic;
        }
        Blynk.virtualWrite(V4, String(deepSleepInterval));
        Serial.println("Sending data to Blynk - DONE");

        // send data...
        delay(500);
    }
    else
    {
        Serial.println("Blynk is not connected");
    }
}

void InternetConnection::setI2CStatusVersion()
{
    // I2C status - SDA and SCL
    bool SDAisOK = digitalRead(SDA);
    bool SCLsOK = digitalRead(SCL);

    String message = (SDAisOK ? String("SDA OK, ") : String("SDA error, ")) +
                     (SCLsOK ? String("SCL OK, ") : String("SCL error, ")) +
                     String("Version: ") + settings.version;

    Blynk.virtualWrite(V17, message);
}