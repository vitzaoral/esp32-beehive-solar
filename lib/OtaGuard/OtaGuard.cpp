#include "OtaGuard.h"

#include <esp_ota_ops.h>
#include <esp_sleep.h>
#include <esp_timer.h>

static esp_timer_handle_t guardTimer = nullptr;
static uint32_t guardSleepSeconds = 60;
static const char *state = "ok";

static void onGuardTimeout(void *)
{
    if (OtaGuard::pendingVerify())
    {
        Serial.println("OtaGuard: cyklus se zasekl u noveho firmware, vracim predchozi verzi");
        esp_ota_mark_app_invalid_rollback_and_reboot();
        return; // sem se nedostane, pokud rollback jde
    }
    Serial.println("OtaGuard: cyklus se zasekl, uspavam");
    esp_sleep_enable_timer_wakeup((uint64_t)guardSleepSeconds * 1000000ULL);
    esp_deep_sleep_start();
}

void OtaGuard::begin(uint32_t timeoutSeconds, uint32_t sleepSeconds)
{
    guardSleepSeconds = sleepSeconds;
    if (guardTimer == nullptr)
    {
        esp_timer_create_args_t args = {};
        args.callback = &onGuardTimeout;
        args.name = "ota_guard";
        if (esp_timer_create(&args, &guardTimer) != ESP_OK)
        {
            guardTimer = nullptr;
            return;
        }
    }
    extend(timeoutSeconds);
}

void OtaGuard::extend(uint32_t timeoutSeconds)
{
    if (guardTimer == nullptr)
        return;
    esp_timer_stop(guardTimer); // vrací chybu, když neběží; nevadí
    esp_timer_start_once(guardTimer, (uint64_t)timeoutSeconds * 1000000ULL);
}

void OtaGuard::stop()
{
    if (guardTimer != nullptr)
        esp_timer_stop(guardTimer);
}

bool OtaGuard::pendingVerify()
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t otaState;
    if (running == nullptr || esp_ota_get_state_partition(running, &otaState) != ESP_OK)
        return false;
    return otaState == ESP_OTA_IMG_PENDING_VERIFY;
}

void OtaGuard::confirm(bool healthy)
{
    if (!pendingVerify())
        return;
    if (healthy)
    {
        esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
        state = err == ESP_OK ? "potvrzeno" : "potvrzeni selhalo";
        Serial.printf("OtaGuard: novy firmware potvrzen (%d)\n", err);
        return;
    }
    Serial.println("OtaGuard: novy firmware se nespojil s Blynkem, vracim predchozi verzi");
    esp_ota_mark_app_invalid_rollback_and_reboot();
    state = "rollback selhal";
}

const char *OtaGuard::stateText()
{
    return state;
}
