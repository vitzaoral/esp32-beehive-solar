// Sken BLE teploměrů (NimBLE, jen pasivní poslech). Wi-Fi při skenu NESMÍ běžet:
// scan() zapne BLE, poslouchá, a BLE zase úplně vypne a uvolní paměť.
#ifndef __BleGateway_H
#define __BleGateway_H

#include <Arduino.h>
#include "BleAdvParser.h"

class BleGateway
{
public:
    void begin(const BleSensorConfig *config, size_t count);
    // Blokuje `seconds` sekund. Po návratu je BLE vypnuté.
    void scan(uint32_t seconds);
    void printResults() const;

    BleSensorStore store;
    bool scanned = false;
    uint32_t scanMillis = 0; // jak dlouho trval sken včetně zapnutí a vypnutí BLE
    uint32_t packetsSeen = 0;
};

#endif
