#include "ricky_robot_audio_gate.h"

#include <atomic>

#include <esp_log.h>
#include "sdkconfig.h"

#if CONFIG_BOARD_TYPE_RICKY_BOY
#include "application.h"
#endif

static const char* TAG = "RickyAudioGate";

#if CONFIG_BOARD_TYPE_RICKY_BOY

static std::atomic<int> s_hold_depth{0};

void RickyRobotAudioGateEnter(void)
{
    const int was = s_hold_depth.fetch_add(1, std::memory_order_acq_rel);
    if (was == 0) {
        Application::GetInstance().GetAudioService().ResetDecoder();
        ESP_LOGI(TAG, "robot sound: ResetDecoder + hold TTS until robot audio done");
    }
}

void RickyRobotAudioGateLeave(void)
{
    const int prev = s_hold_depth.fetch_sub(1, std::memory_order_acq_rel);
    if (prev <= 0) {
        s_hold_depth.store(0, std::memory_order_release);
        ESP_LOGW(TAG, "Leave underflow (ignored)");
    } else if (prev == 1) {
        ESP_LOGI(TAG, "robot sound done, TTS allowed");
    }
}

bool RickyRobotAudioGateIsHeld(void)
{
    return s_hold_depth.load(std::memory_order_acquire) > 0;
}

#else  // !CONFIG_BOARD_TYPE_RICKY_BOY

void RickyRobotAudioGateEnter(void) {}
void RickyRobotAudioGateLeave(void) {}
bool RickyRobotAudioGateIsHeld(void)
{
    return false;
}

#endif
