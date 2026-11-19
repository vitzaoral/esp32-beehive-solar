// Pojistka po OTA: nový firmware se po prvním startu musí sám potvrdit,
// jinak bootloader vrátí předchozí verzi (rollback).
//
// Jak to funguje (Arduino-ESP32 2.0.x má CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y):
//  1. Update.end() po OTA označí nový oddíl jako NEW, bootloader ho při startu
//     přepne na PENDING_VERIFY.
//  2. main.cpp přepisuje verifyRollbackLater() = true, takže Arduino ho samo
//     neschválí (výchozí chování schvaluje hned po startu, ochrana pak nic nedělá).
//  3. Po prvním cyklu: spojení s Blynkem (tj. jde poslat další OTA) → confirm(true)
//     → firmware platí. Jinak confirm(false) → návrat k předchozí verzi a restart.
//  4. Pád, watchdog nebo restart před potvrzením → bootloader vrátí předchozí verzi sám.
//     POZOR: probuzení z hlubokého spánku jde přes bootloader, proto se musí
//     rozhodnout ještě před prvním spánkem.
//  5. Hlídač (esp_timer): když se cyklus zasekne (déle než timeout), u nepotvrzeného
//     firmware spustí rollback, u potvrzeného jen uspí desku, ať nevybije baterii.
#ifndef __OtaGuard_H
#define __OtaGuard_H

#include <Arduino.h>

namespace OtaGuard
{
    // Spustí hlídače cyklu. sleepSeconds = na jak dlouho uspat, když se cyklus zasekne.
    void begin(uint32_t timeoutSeconds, uint32_t sleepSeconds);
    // Prodlouží hlídače (např. během stahování OTA).
    void extend(uint32_t timeoutSeconds);
    void stop();
    // Běží firmware čerstvě po OTA a čeká na potvrzení?
    bool pendingVerify();
    // healthy = true: potvrdit; false: vrátit předchozí verzi (restartuje, nevrací se).
    void confirm(bool healthy);
    // Krátký popis pro Blynk/log: "ok", "potvrzeno", "bez rollbacku"
    const char *stateText();
}

#endif
