#include "BleGateway.h"

#include <NimBLEDevice.h>

static BleGateway *activeGateway = nullptr;

class AdvCallbacks : public NimBLEAdvertisedDeviceCallbacks
{
    void onResult(NimBLEAdvertisedDevice *device) override
    {
        if (activeGateway == nullptr)
            return;
        activeGateway->packetsSeen++;
        // NimBLE drží adresu little-endian, my v pořadí zápisu (A4:C1:38:...)
        const uint8_t *native = device->getAddress().getNative();
        uint8_t mac[6];
        for (int i = 0; i < 6; i++)
            mac[i] = native[5 - i];
        activeGateway->store.onAdvertisement(mac, device->getRSSI(), device->getPayload(), device->getPayloadLength());
    }
};

static AdvCallbacks advCallbacks;

void BleGateway::begin(const BleSensorConfig *config, size_t count)
{
    int ok = store.configure(config, count);
    Serial.printf("BLE: %d povolenych cidel\n", ok);
}

void BleGateway::scan(uint32_t seconds)
{
    uint32_t started = millis();
    store.resetScan();
    packetsSeen = 0;
    activeGateway = this;

    // BLE stačí 80 MHz, ušetří to proud po dobu poslechu
    uint32_t cpuMhz = getCpuFrequencyMhz();
    setCpuFrequencyMhz(80);

    NimBLEDevice::init("");
    NimBLEScan *scanner = NimBLEDevice::getScan();
    // wantDuplicates = true: BTHome posílá teplotu a napětí střídavě ve dvou paketech
    scanner->setAdvertisedDeviceCallbacks(&advCallbacks, true);
    scanner->setActiveScan(false); // pasivně: nic nevysíláme, teploměry nic navíc neplatí
    scanner->setInterval(100);     // ms
    scanner->setWindow(100);       // = poslouchá se nepřetržitě
    scanner->setDuplicateFilter(false);
    scanner->setMaxResults(0); // výsledky neukládat, stačí callback
    // Neblokující start a vlastní limit času. Blokující start(seconds) z NimBLE 1.4.3
    // se na ESP32-D0WDQ6 nevrátil: příjem po 10 s skončil, ale událost konce skenu
    // nepřišla, isScanning() zůstalo true a cyklus visel až do hlídače (OtaGuard).
    if (scanner->start(seconds, nullptr, false))
    {
        uint32_t deadline = millis() + seconds * 1000UL + 500UL;
        while (scanner->isScanning() && (int32_t)(deadline - millis()) > 0)
            delay(50);
    }
    else
    {
        Serial.println("BLE: sken se nespustil");
    }
    scanner->stop();
    NimBLEDevice::deinit(true);

    activeGateway = nullptr;
    setCpuFrequencyMhz(cpuMhz);
    scanned = true;
    scanMillis = millis() - started;
}

void BleGateway::printResults() const
{
    Serial.printf("BLE sken %lu ms, paketu %lu, cidel %d/%d, neznamych %d, sifrovanych paketu %u\n",
                  (unsigned long)scanMillis, (unsigned long)packetsSeen, store.seenCount(), store.sensorCount,
                  store.unknownCount, store.encryptedPackets);
    char mac[18];
    for (int i = 0; i < store.sensorCount; i++)
    {
        const BleSensorSlot &s = store.sensors[i];
        bleFormatMac(s.mac, mac);
        if (!s.seen)
        {
            Serial.printf("  %s (%s): NEOZVALO SE\n", s.name, mac);
            continue;
        }
        Serial.printf("  %s (%s): %.2f C, %.1f %%, baterie %u %% %u mV, RSSI %d, paketu %u, format %s\n",
                      s.name, mac, s.reading.temperature, s.reading.humidity, s.reading.batteryPct,
                      s.reading.batteryMv, s.rssi, s.packets, bleFormatName(s.reading.format));
    }
    for (int i = 0; i < store.unknownCount; i++)
    {
        bleFormatMac(store.unknown[i], mac);
        const BleThermoReading &r = store.unknownReading[i];
        Serial.printf("  NEZNAME cidlo %s: %.2f C, RSSI %d, format %s (pridej MAC do src/ble_config.h)\n",
                      mac, r.temperature, store.unknownRssi[i], bleFormatName(r.format));
    }
    if (store.encryptedPackets > 0)
        Serial.println("  Nektere teplomery vysilaji sifrovane: v TelinkMiFlasher vypni sifrovani (bindkey).");
}
