// Parser BLE vysílání teploměrů s firmwarem pvvx (Xiaomi LYWSD03MMC a další).
//
// Čisté C++ bez Arduina a bez NimBLE, aby šel testovat nativně (pio test -e native).
// Podporované formáty (všechny nešifrované):
//   - pvvx custom:  service data UUID 0x181A, 15 bajtů, little-endian, 0,01 °C a 0,01 %
//   - ATC1441:      service data UUID 0x181A, 13 bajtů, big-endian, 0,1 °C a 1 %
//   - BTHome v2:    service data UUID 0xFCD2 (výchozí formát pvvx; od pvvx 6.0 jediný),
//                   teplota/vlhkost/baterie chodí střídavě ve dvou paketech, proto merge.
// Šifrované varianty se jen rozpoznají (BLE_PARSE_ENCRYPTED), dešifrování neumíme.
#ifndef __BleAdvParser_H
#define __BleAdvParser_H

#include <stddef.h>
#include <stdint.h>

enum BleFormat : uint8_t
{
    BLE_FMT_NONE = 0,
    BLE_FMT_PVVX = 1,
    BLE_FMT_ATC1441 = 2,
    BLE_FMT_BTHOME = 3,
};

enum BleParseResult : uint8_t
{
    BLE_PARSE_OK = 0,
    BLE_PARSE_NOT_THERMO,  // jiné zařízení (telefon, sluchátka...)
    BLE_PARSE_ENCRYPTED,   // náš formát, ale šifrovaný: v TelinkMiFlasher vypnout šifrování
    BLE_PARSE_MALFORMED,   // UUID sedí, délka ne
};

struct BleThermoReading
{
    uint8_t mac[6]; // v pořadí zápisu: mac[0] je první bajt z "A4:C1:38:..."
    BleFormat format;
    bool hasTemperature;
    bool hasHumidity;
    bool hasBatteryPct;
    bool hasBatteryMv;
    bool hasCounter;
    float temperature; // °C
    float humidity;    // %
    uint8_t batteryPct;
    uint16_t batteryMv;
    uint8_t counter; // čítač měření (custom/ATC) nebo packet id (BTHome)
};

void bleReadingClear(BleThermoReading &r);

// payload = surová data vysílání (posloupnost AD struktur: délka, typ, data),
// advMac = adresa vysílajícího v pořadí zápisu (pro BTHome, který MAC v datech nemá).
BleParseResult bleParseAdvertisement(const uint8_t *payload, size_t len, const uint8_t advMac[6], BleThermoReading &out);

// Jedna service data s 16bit UUID; data = bajty ZA UUID.
BleParseResult bleParseServiceData(uint16_t uuid, const uint8_t *data, size_t len, const uint8_t advMac[6], BleThermoReading &out);

// Doplní do `into` hodnoty, které `from` má (BTHome posílá data ve dvou paketech).
void bleMergeReading(BleThermoReading &into, const BleThermoReading &from);

// "A4:C1:38:EE:C8:5C" (i malá písmena) → 6 bajtů. false při špatném formátu.
bool bleParseMac(const char *text, uint8_t mac[6]);
// 6 bajtů → "A4:C1:38:EE:C8:5C" (out musí mít aspoň 18 znaků)
void bleFormatMac(const uint8_t mac[6], char out[18]);
const char *bleFormatName(BleFormat f);

// ---------------------------------------------------------------------------
// Úložiště jednoho skenu: povolená čidla (sloty) + neznámá čidla do logu.
// Také čisté C++, volá ho callback NimBLE a testy.

struct BleSensorConfig
{
    const char *mac;  // "A4:C1:38:EE:C8:5C"
    const char *name; // název úlu, jde do Blynku i do beeSysu
};

struct BleSensorSlot
{
    const char *name;
    uint8_t mac[6];
    bool seen;
    int8_t rssi;      // nejsilnější signál během skenu (dBm)
    uint16_t packets; // kolik platných paketů přišlo
    BleThermoReading reading;
};

class BleSensorStore
{
public:
    static const int MAX_SENSORS = 8;
    static const int MAX_UNKNOWN = 8;

    // Vrátí počet přijatých čidel z configu (špatná MAC se přeskočí).
    int configure(const BleSensorConfig *config, size_t count);
    void resetScan();
    // Jedno přijaté vysílání. Vrací výsledek parseru.
    BleParseResult onAdvertisement(const uint8_t advMac[6], int rssi, const uint8_t *payload, size_t len);
    int findSlot(const uint8_t mac[6]) const;
    int seenCount() const;

    int sensorCount = 0;
    BleSensorSlot sensors[MAX_SENSORS];

    // Čidla, která vysílají náš formát, ale nejsou v seznamu (pro zjištění MAC).
    int unknownCount = 0;
    uint8_t unknown[MAX_UNKNOWN][6];
    int8_t unknownRssi[MAX_UNKNOWN];
    BleThermoReading unknownReading[MAX_UNKNOWN];

    uint16_t encryptedPackets = 0;
};

#endif
