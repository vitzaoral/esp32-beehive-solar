// Nastavení brány pro BLE teploměry (Xiaomi LYWSD03MMC s firmwarem pvvx).
// Nic tajného tu není; heslo k Wi-Fi, Blynk a token do beeSysu jsou v src/settings.cpp.
#ifndef __BleConfig_H
#define __BleConfig_H

#include "BleAdvParser.h"

// Jak často poslouchat: každých 5 minut (při probouzení po 60 s = každé 5. probuzení).
// Jde přepsat z Blynku pinem V58 (v minutách, 0 = vypnout BLE úplně).
#define BLE_SCAN_PERIOD_MINUTES 5
// Jak dlouho poslouchat. pvvx vysílá ve výchozím nastavení každých 2,5 s,
// za 10 s tedy přijdou 3–4 pakety z každého teploměru.
#define BLE_SCAN_SECONDS 10

// Povolené teploměry: MAC (jak ji ukazuje TelinkMiFlasher) a název úlu.
// Nejvýš 8. Neznámé teploměry se jen vypíšou do sériové linky a do Blynku V59.
static const BleSensorConfig BLE_SENSORS[] = {
    // Zkušební kus Xiaomi Monitor 3 Mini (MJWSD06MMC) s pvvx 5.9, formát PVVX (Custom).
    // MAC podle flasheru (Get MAC) i cloudu Xiaomi; tovární firmware vysílal z jiné adresy.
    {"A4:C1:38:52:A7:D4", "Zkouska Xiaomi"},
};

// beeSys: příjem dat z brány (token v settings.cpp jako beesysToken)
#define BEESYS_BLE_URL "https://sys.zaoral.cz/api/public/ble/readings"
#define BEESYS_GATEWAY_NAME "vcely-solar"

// Blynk: nové virtuální piny (stávající V0–V48 se nemění)
//   V58 = perioda poslechu v minutách (zápis z aplikace, 0 = vypnuto)
//   V59 = stav brány (text): kolik čidel se ozvalo, neznámá, výsledek beeSys, OTA
//   V60 + 3*i = teplota čidla i (°C), V61 + 3*i = vlhkost (%), V62 + 3*i = baterie (%)
//   i = pořadí v BLE_SENSORS (0..7) → V60–V83
#define BLYNK_PIN_BLE_PERIOD V58
#define BLYNK_PIN_BLE_STATUS V59
#define BLYNK_PIN_BLE_FIRST 60

#endif
