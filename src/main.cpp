#include <Arduino.h>
#include <InternetConnection.h>
#include <BleGateway.h>
#include <OtaGuard.h>
#include "ble_config.h"

InternetConnection connection;
MeteoData meteoData;
PowerController powerController;
BleGateway bleGateway;

#define POWER_BANK_PIN 36

// Hlídač cyklu: běžný cyklus trvá do 40 s (BLE 10 s, Wi-Fi do 11 s, Blynk, HTTPS).
#define CYCLE_GUARD_SECONDS 90
// Nepotvrzený firmware po OTA: dva další pokusy o Blynk po 30 s, pak rollback.
#define OTA_VERIFY_RETRIES 2
#define OTA_VERIFY_RETRY_DELAY_MS 30000
#define OTA_VERIFY_GUARD_SECONDS 240

// Přežije hluboký spánek, po zapnutí napájení se nastaví znovu.
RTC_DATA_ATTR uint32_t bleSecondsSinceScan = UINT32_MAX; // po zapnutí poslouchat hned
RTC_DATA_ATTR uint32_t lastCycleSeconds = 0;             // spánek + bdění minulého cyklu
RTC_DATA_ATTR int32_t blePeriodMinutes = BLE_SCAN_PERIOD_MINUTES;
RTC_DATA_ATTR uint32_t lastSleepSeconds = 60;

// Arduino jádro jinak nový firmware po OTA schválí hned po startu (viz OtaGuard.h).
extern "C" bool verifyRollbackLater()
{
    return true;
}

bool sendDataToInternet();
bool bleScanDue();

void setup()
{
    Serial.begin(115200);
    OtaGuard::begin(OtaGuard::pendingVerify() ? OTA_VERIFY_GUARD_SECONDS : CYCLE_GUARD_SECONDS, lastSleepSeconds);
    if (OtaGuard::pendingVerify())
        Serial.println("Novy firmware po OTA, ceka na potvrzeni");

    pinMode(POWER_BANK_PIN, INPUT); // 36 = GPIO36 = ADC1_CH0 read if powerbank 1 is connected (has energy)

    meteoData.initializeSensors();
    powerController.initializeSensors();

    Serial.println("Setup done");

    // BLE před Wi-Fi: obě rádia nikdy nejedou současně
    bleGateway.begin(BLE_SENSORS, sizeof(BLE_SENSORS) / sizeof(BLE_SENSORS[0]));
    if (bleScanDue())
    {
        bleGateway.scan(BLE_SCAN_SECONDS);
        bleGateway.printResults();
        bleSecondsSinceScan = 0;
    }

    bool blynkOk = sendDataToInternet();
    for (int i = 0; !blynkOk && OtaGuard::pendingVerify() && i < OTA_VERIFY_RETRIES; i++)
    {
        Serial.println("Novy firmware se nespojil, zkousim znovu za 30 s");
        delay(OTA_VERIFY_RETRY_DELAY_MS);
        blynkOk = connection.initializeConnection();
        connection.disconnect();
    }
    // Rollback, když se nový firmware nespojil (jinak by už nešel opravit přes OTA)
    OtaGuard::confirm(blynkOk);

    Serial.println("Data sended, BYE");
    lastSleepSeconds = connection.deepSleepInterval;
    lastCycleSeconds = connection.deepSleepInterval + millis() / 1000;
    OtaGuard::stop();
    esp_deep_sleep(connection.deepSleepInterval * 1000000ULL);
}

void loop()
{
  // delay(1000);
  // sendDataToInternet();
}

bool bleScanDue()
{
    if (blePeriodMinutes <= 0)
    {
        Serial.println("BLE vypnuto (V58 = 0)");
        return false;
    }
    if (bleSecondsSinceScan != UINT32_MAX)
        bleSecondsSinceScan += lastCycleSeconds;
    uint32_t period = (uint32_t)blePeriodMinutes * 60;
    // polovina cyklu tolerance, aby se při probouzení po 60 s neposlouchalo jen každých 6 min
    bool due = bleSecondsSinceScan >= period || period - bleSecondsSinceScan <= lastCycleSeconds / 2;
    Serial.printf("BLE: od posledniho poslechu %lu s, perioda %lu s -> %s\n",
                  (unsigned long)(bleSecondsSinceScan == UINT32_MAX ? 0 : bleSecondsSinceScan),
                  (unsigned long)period, due ? "poslouchat" : "ne");
    return due;
}

// Vrací true, když se podařilo spojit s Blynkem.
bool sendDataToInternet()
{
  Serial.println("Start initialize Blynk connection");
  bool blynkOk = connection.initializeConnection();
  if (blynkOk)
  {
    Serial.println("Setting sensors data");
    meteoData.setData();
    powerController.setData();

    Serial.println("Sending data to Blynk");
    connection.sendDataToBlynk(meteoData, powerController);
  }
  else
  {
    Serial.println("No internet/blynk connection");
  }

  // BLE data: do beeSysu i bez Blynku (stačí Wi-Fi), stav do Blynku
  if (bleGateway.scanned)
  {
    int beesysCode = connection.sendBleToBeesys(bleGateway.store, bleGateway.scanMillis);
    if (blynkOk)
      connection.sendBleToBlynk(bleGateway.store, beesysCode,
                                OtaGuard::pendingVerify() ? "ceka" : OtaGuard::stateText());
  }

  int period = connection.blePeriodFromBlynk();
  if (period >= 0)
    blePeriodMinutes = period;

  connection.disconnect();
  return blynkOk;
}
