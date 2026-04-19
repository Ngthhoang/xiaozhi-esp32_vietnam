#include <esp_log.h>

#include "application.h"
#include "button.h"
#include "codecs/no_audio_codec.h"
#include "config.h"
#include "display/display.h"
#include "led/led.h"
#include "mcp_server.h"
#include "ricky_status_led.h"
#include "wifi_board.h"

#include <wifi_station.h>

#define TAG "RickyBoy"

extern void InitializeRickyController();

class RickyBoyBoard : public WifiBoard {
private:
    Button boot_button_;

    void InitializeTools() {
        auto& mcp_server = McpServer::GetInstance();
        mcp_server.AddTool(
            "self.system.reconfigure_wifi",
            "Reboot once and enter WiFi configuration mode (captive portal / AP). "
            "Use when the user asks to change WiFi, reconfigure network, or enter provisioning. "
            "**CAUTION** Confirm with the user when possible. One restart is normal before the config hotspot appears.",
            PropertyList(),
            [this](const PropertyList& /*properties*/) -> ReturnValue {
                ResetWifiConfiguration();
                return true;
            });
    }

    void InitializeButtons() {
        // Headless: không có màn hình hướng dẫn — 5 lần nhấn BOOT = xóa WiFi, reboot, phát AP cấu hình.
        boot_button_.OnMultipleClick([this]() { ResetWifiConfiguration(); }, 5);
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting &&
                !WifiStation::GetInstance().IsConnected()) {
                ResetWifiConfiguration();
            }
            app.ToggleChatState();
        });
    }

public:
    RickyBoyBoard() : boot_button_(BOOT_BUTTON_GPIO) {
        InitializeButtons();
        InitializeRickyController();
        InitializeTools();
        ESP_LOGI(TAG, "Ricky_boy board (ESP32-S3 mini target), fw %s", RICKY_BOY_VERSION);
    }

    virtual AudioCodec* GetAudioCodec() override {
        static NoAudioCodecSimplex audio_codec(
            AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE, AUDIO_I2S_SPK_GPIO_BCLK,
            AUDIO_I2S_SPK_GPIO_LRCK, AUDIO_I2S_SPK_GPIO_DOUT, AUDIO_I2S_MIC_GPIO_SCK,
            AUDIO_I2S_MIC_GPIO_WS, AUDIO_I2S_MIC_GPIO_DIN);
        return &audio_codec;
    }

    virtual Display* GetDisplay() override {
        static NoDisplay display;
        return &display;
    }

    Led* GetLed() override {
        static RickyStatusLed led(RICKY_STATUS_LED_GPIO);
        return &led;
    }
};

DECLARE_BOARD(RickyBoyBoard);
