#include "ricky_bark.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

#include <esp_log.h>
#include <esp_random.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "audio_codec.h"
#include "board.h"
#include "ricky_robot_audio_gate.h"
#include "chosua1s_long.h"
#include "chosua1s_short.h"

static const char* TAG = "RickyBark";

static constexpr int kBarkSampleRateHz = 24000;

static void BarkPlaybackTask(void* pvParameters) {
    const bool long_bark = (reinterpret_cast<intptr_t>(pvParameters) != 0);
    const unsigned char* raw = long_bark ? chosua1s_long_raw : chosua1s_short_raw;
    const unsigned int len = long_bark ? chosua1s_long_raw_len : chosua1s_short_raw_len;

    AudioCodec* codec = Board::GetInstance().GetAudioCodec();
    if (codec == nullptr) {
        ESP_LOGW(TAG, "no codec");
        vTaskDelete(nullptr);
        return;
    }

    RickyRobotAudioGateEnter();
    codec->EnableOutput(true);

    if (codec->output_sample_rate() != kBarkSampleRateHz) {
        if (!codec->SetOutputSampleRate(kBarkSampleRateHz)) {
            ESP_LOGW(TAG, "SetOutputSampleRate %d failed, playing at %d Hz", kBarkSampleRateHz,
                     codec->output_sample_rate());
        }
    }

    constexpr size_t kChunkBytes = 1024;
    for (size_t offset = 0; offset < len; offset += kChunkBytes) {
        size_t n = std::min(kChunkBytes, static_cast<size_t>(len - offset));
        if (n < sizeof(int16_t)) {
            break;
        }
        n -= n % sizeof(int16_t);
        const size_t samples = n / sizeof(int16_t);
        std::vector<int16_t> pcm(samples);
        std::memcpy(pcm.data(), raw + offset, n);
        codec->OutputData(pcm);
    }

    (void)codec->SetOutputSampleRate(-1);
    RickyRobotAudioGateLeave();

    vTaskDelete(nullptr);
}

static void StartBarkTask(bool long_bark) {
    const BaseType_t ok =
        xTaskCreate(BarkPlaybackTask, "ricky_bark", 6144,
                    reinterpret_cast<void*>(static_cast<intptr_t>(long_bark ? 1 : 0)), 3, nullptr);
    if (ok != pdPASS) {
        ESP_LOGW(TAG, "xTaskCreate bark failed");
    }
}

void RickyPlayBarkShortAsync() {
    ESP_LOGI(TAG, "bark short (async)");
    StartBarkTask(false);
}

void RickyPlayBarkLongAsync() {
    ESP_LOGI(TAG, "bark long (async)");
    StartBarkTask(true);
}

void RickyPlayBarkRandomForPresetBasic() {
    const bool long_bark = (esp_random() & 1u) != 0;
    ESP_LOGI(TAG, "preset bark: %s", long_bark ? "long" : "short");
    StartBarkTask(long_bark);
}
