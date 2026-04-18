#include "ricky_speed_settings.h"

#include <cstdio>
#include <string>

#include <esp_log.h>

#include "config.h"
#include "settings.h"

static const char* TAG = "RickySpeedNvs";
static const char* kNs = "ricky";
static const char* kFwd = "norm_fwd";
static const char* kBack = "norm_back";

static constexpr int32_t kAbsent = -999;

static int ClampPct(int v)
{
    if (v < RICKY_SPEED_MIN_EFFECTIVE) {
        return RICKY_SPEED_MIN_EFFECTIVE;
    }
    if (v > 100) {
        return 100;
    }
    return v;
}

int RickySpeedGetStoredForwardNormal()
{
    Settings s(kNs, false);
    int v = (int)s.GetInt(kFwd, RICKY_SPEED_DEFAULT);
    return ClampPct(v);
}

int RickySpeedGetStoredBackwardNormal()
{
    Settings s(kNs, false);
    int v = (int)s.GetInt(kBack, RICKY_SPEED_DEFAULT);
    return ClampPct(v);
}

bool RickySpeedStoreForwardNormal(int percent)
{
    if (percent < RICKY_SPEED_MIN_EFFECTIVE || percent > 100) {
        return false;
    }
    Settings s(kNs, true);
    s.SetInt(kFwd, percent);
    ESP_LOGI(TAG, "saved forward normal = %d%%", percent);
    return true;
}

bool RickySpeedStoreBackwardNormal(int percent)
{
    if (percent < RICKY_SPEED_MIN_EFFECTIVE || percent > 100) {
        return false;
    }
    Settings s(kNs, true);
    s.SetInt(kBack, percent);
    ESP_LOGI(TAG, "saved backward normal = %d%%", percent);
    return true;
}

void RickySpeedResetToFactoryDefaults()
{
    Settings s(kNs, true);
    s.EraseKey(kFwd);
    s.EraseKey(kBack);
    ESP_LOGI(TAG, "reset motor speed NVS → firmware default %d%%", RICKY_SPEED_DEFAULT);
}

std::string RickySpeedGetStatusJson()
{
    Settings s(kNs, false);
    const int raw_f = (int)s.GetInt(kFwd, kAbsent);
    const int raw_b = (int)s.GetInt(kBack, kAbsent);
    const int f = (raw_f == kAbsent) ? RICKY_SPEED_DEFAULT : ClampPct(raw_f);
    const int b = (raw_b == kAbsent) ? RICKY_SPEED_DEFAULT : ClampPct(raw_b);
    const bool nvs_f = (raw_f != kAbsent);
    const bool nvs_b = (raw_b != kAbsent);

    char buf[192];
    snprintf(buf, sizeof(buf),
             "{\"forward\":%d,\"backward\":%d,\"nvs_forward\":%s,\"nvs_backward\":%s,"
             "\"factory_default\":%d}",
             f, b, nvs_f ? "true" : "false", nvs_b ? "true" : "false", RICKY_SPEED_DEFAULT);
    return std::string(buf);
}
