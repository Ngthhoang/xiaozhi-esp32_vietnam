#include "ricky_status_led.h"

#include "application.h"
#include "device_state.h"

#include <esp_log.h>
#include <wifi_station.h>

#define TAG "RickyStatusLed"

// Cùng thang độ sáng với SingleLed (WS2812 một pixel).
static constexpr uint8_t kDim = 4;
static constexpr uint8_t kLow = 2;
static constexpr uint8_t kHigh = 16;

RickyStatusLed::RickyStatusLed(gpio_num_t gpio) : SingleLed(gpio) {}

void RickyStatusLed::OnStateChanged() {
    auto& app = Application::GetInstance();
    const DeviceState device_state = app.GetDeviceState();

    switch (device_state) {
        case kDeviceStateStarting:
            SetColor(0, 0, kDim);
            StartContinuousBlink(100);
            break;
        case kDeviceStateWifiConfiguring:
            SetColor(0, 0, kDim);
            StartContinuousBlink(500);
            break;
        case kDeviceStateIdle:
            if (WifiStation::GetInstance().IsConnected()) {
                SetColor(0, kHigh, 0);
                TurnOn();
            } else {
                TurnOff();
            }
            break;
        case kDeviceStateConnecting:
            SetColor(0, 0, kDim);
            TurnOn();
            break;
        case kDeviceStateListening:
        case kDeviceStateAudioTesting:
            if (app.IsVoiceDetected()) {
                SetColor(kHigh, 0, 0);
            } else {
                SetColor(kLow, 0, 0);
            }
            TurnOn();
            break;
        case kDeviceStateSpeaking:
            SetColor(kHigh, kHigh, 0);
            TurnOn();
            break;
        case kDeviceStateUpgrading:
            SetColor(0, kDim, 0);
            StartContinuousBlink(100);
            break;
        case kDeviceStateActivating:
            SetColor(0, kDim, 0);
            StartContinuousBlink(500);
            break;
        case kDeviceStateUnknown:
        case kDeviceStateFatalError:
            TurnOff();
            break;
        default:
            ESP_LOGW(TAG, "Unhandled device state: %d", (int)device_state);
            TurnOff();
            break;
    }
}
