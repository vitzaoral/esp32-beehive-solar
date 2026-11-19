#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPLalHw40y_"
#define BLYNK_TEMPLATE_NAME "Vcely baterky"
#define BLYNK_FIRMWARE_VERSION "3.1.0"

#include "InternetConnection.h"
#include <BlynkSimpleEsp32.h>
#include "../../src/settings.cpp"
#include "../../src/ble_config.h"
#include "BeesysCa.h"
#include <WiFiClientSecure.h>
#include <OtaGuard.h>

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

    // stahování může na hotspotu trvat; hlídač cyklu nesmí OTA přerušit
    OtaGuard::extend(600);

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
int blePeriodMinutesStatic = -1;

// BLE teploměry: perioda poslechu v minutách (0 = vypnuto)
BLYNK_WRITE(BLYNK_PIN_BLE_PERIOD)
{
    int minutes = param.asInt();
    if (minutes >= 0 && minutes <= 1440)
    {
        blePeriodMinutesStatic = minutes;
        Serial.println("BLE scan period was set to: " + String(minutes) + " min");
    }
}

int InternetConnection::blePeriodFromBlynk()
{
    return blePeriodMinutesStatic;
}

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
        Blynk.virtualWrite(V41, powerController.sensor_solar.loadVoltage);
        Blynk.virtualWrite(V42, powerController.sensor_solar.current_mA);
        Blynk.virtualWrite(V43, powerController.sensor_solar.power_mW / 1000.0);

        // 12V battery power data
        Blynk.virtualWrite(V8, powerController.sensor_battery.loadVoltage); // v8
        Blynk.virtualWrite(V9, powerController.sensor_battery.current_mA); // v9
        Blynk.virtualWrite(V10, powerController.sensor_battery.power_mW / 1000.0); // v10

        // powerbank power data
        Blynk.virtualWrite(V44, powerController.sensor_powerbank.loadVoltage);
        Blynk.virtualWrite(V45, powerController.sensor_powerbank.current_mA);
        Blynk.virtualWrite(V46, powerController.sensor_powerbank.power_mW / 1000.0);

        // powerbanks leds
        led1.on();
        led2.on();
        led1.setColor(powerController.powerBank1Connected == 1 ? LED_GREEN : LED_RED);
        led2.setColor(powerController.powerBank2Connected == 1 ? LED_GREEN : LED_RED);

        // powebanks isOn / isOff
        Blynk.virtualWrite(V37, powerController.powerBank1Connected == 1);
        Blynk.virtualWrite(V38, powerController.powerBank2Connected == 1);

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

static void appendJsonString(String &out, const char *text)
{
    out += '"';
    for (const char *p = text; *p; p++)
    {
        char c = *p;
        if (c == '"' || c == '\\')
        {
            out += '\\';
            out += c;
        }
        else if ((uint8_t)c < 0x20)
            out += ' ';
        else
            out += c;
    }
    out += '"';
}

int InternetConnection::sendBleToBeesys(const BleSensorStore &store, uint32_t scanMillis)
{
    if (!WiFi.isConnected())
        return 0;

    char mac[18];
    char num[24];
    String body;
    body.reserve(220 + store.sensorCount * 200);
    body += "{\"gateway\":";
    appendJsonString(body, BEESYS_GATEWAY_NAME);
    body += ",\"firmware\":";
    appendJsonString(body, BLYNK_FIRMWARE_VERSION);
    snprintf(num, sizeof(num), "%lu", (unsigned long)scanMillis);
    body += ",\"scan_ms\":";
    body += num;
    body += ",\"sensors\":[";
    bool first = true;
    for (int i = 0; i < store.sensorCount; i++)
    {
        const BleSensorSlot &s = store.sensors[i];
        if (!s.seen)
            continue;
        if (!first)
            body += ',';
        first = false;
        bleFormatMac(s.mac, mac);
        body += "{\"mac\":\"";
        body += mac;
        body += "\",\"name\":";
        appendJsonString(body, s.name);
        body += ",\"format\":\"";
        body += bleFormatName(s.reading.format);
        body += '"';
        if (s.reading.hasTemperature)
        {
            snprintf(num, sizeof(num), "%.2f", s.reading.temperature);
            body += ",\"temperature\":";
            body += num;
        }
        if (s.reading.hasHumidity)
        {
            snprintf(num, sizeof(num), "%.2f", s.reading.humidity);
            body += ",\"humidity\":";
            body += num;
        }
        if (s.reading.hasBatteryPct)
        {
            body += ",\"battery_pct\":";
            body += String(s.reading.batteryPct);
        }
        if (s.reading.hasBatteryMv)
        {
            body += ",\"battery_mv\":";
            body += String(s.reading.batteryMv);
        }
        body += ",\"rssi\":";
        body += String(s.rssi);
        body += ",\"packets\":";
        body += String(s.packets);
        body += '}';
    }
    body += "],\"unknown\":[";
    for (int i = 0; i < store.unknownCount; i++)
    {
        if (i > 0)
            body += ',';
        bleFormatMac(store.unknown[i], mac);
        body += '"';
        body += mac;
        body += '"';
    }
    body += "],\"encrypted_packets\":";
    body += String(store.encryptedPackets);
    body += '}';

    WiFiClientSecure client;
    client.setCACert(BEESYS_CA_PEM);
    client.setHandshakeTimeout(10);
    HTTPClient http;
    http.setConnectTimeout(8000);
    http.setTimeout(8000);
    if (!http.begin(client, BEESYS_BLE_URL))
    {
        Serial.println("beeSys: http.begin selhal");
        return -100;
    }
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Gateway-Token", settings.beesysToken);
    int code = http.POST(body);
    Serial.printf("beeSys: POST %d (%u B)\n", code, body.length());
    if (code > 0 && code != 200)
        Serial.println(http.getString());
    http.end();
    return code;
}

void InternetConnection::sendBleToBlynk(const BleSensorStore &store, int beesysCode, const char *otaState)
{
    if (!Blynk.connected())
        return;
    for (int i = 0; i < store.sensorCount; i++)
    {
        const BleSensorSlot &s = store.sensors[i];
        if (!s.seen)
            continue;
        int pin = BLYNK_PIN_BLE_FIRST + 3 * i;
        if (s.reading.hasTemperature)
            Blynk.virtualWrite(pin, s.reading.temperature);
        if (s.reading.hasHumidity)
            Blynk.virtualWrite(pin + 1, s.reading.humidity);
        if (s.reading.hasBatteryPct)
            Blynk.virtualWrite(pin + 2, s.reading.batteryPct);
    }

    String status = "BLE " + String(store.seenCount()) + "/" + String(store.sensorCount);
    if (store.unknownCount > 0)
    {
        char mac[18];
        status += " | nezname:";
        for (int i = 0; i < store.unknownCount && i < 3; i++)
        {
            bleFormatMac(store.unknown[i], mac);
            status += " ";
            status += mac;
        }
    }
    if (store.encryptedPackets > 0)
        status += " | sifrovane!";
    status += " | beeSys " + String(beesysCode);
    status += " | OTA " + String(otaState);
    Blynk.virtualWrite(BLYNK_PIN_BLE_STATUS, status);

    for (int i = 0; i < 5; i++)
    {
        Blynk.run();
        delay(100);
    }
}
