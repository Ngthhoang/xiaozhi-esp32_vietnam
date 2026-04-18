#include "ricky_dance_music.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <vector>

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "audio_codec.h"
#include "board.h"
#include "ricky_robot_audio_gate.h"
#include "dog_dance2.h"
#include "wholetdogout.h"

static const char* TAG = "RickyDanceMusic";

static constexpr int kMusicSampleRateHz = 24000;
static std::atomic<bool> g_stop_requested{false};

static void DanceMusicTask(void* pvParameters) {
    const uint32_t duration_ms = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pvParameters));
    const TickType_t end_tick = xTaskGetTickCount() + pdMS_TO_TICKS(duration_ms);

    AudioCodec* codec = Board::GetInstance().GetAudioCodec();
    if (codec == nullptr) {
        ESP_LOGW(TAG, "no codec");
        vTaskDelete(nullptr);
        return;
    }

    RickyRobotAudioGateEnter();
    codec->EnableOutput(true);
    if (codec->output_sample_rate() != kMusicSampleRateHz) {
        if (!codec->SetOutputSampleRate(kMusicSampleRateHz)) {
            ESP_LOGW(TAG, "SetOutputSampleRate %d failed, playing at %d Hz", kMusicSampleRateHz,
                     codec->output_sample_rate());
        }
    }

    const unsigned char* raw = dog_dance2_raw;
    const unsigned int len = dog_dance2_raw_len;
    constexpr size_t kChunkBytes = 1024;

    while (!g_stop_requested.load() && xTaskGetTickCount() < end_tick) {
        for (size_t offset = 0;
             offset < len && !g_stop_requested.load() && xTaskGetTickCount() < end_tick;
             offset += kChunkBytes) {
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
    }

    (void)codec->SetOutputSampleRate(-1);
    RickyRobotAudioGateLeave();
    ESP_LOGI(TAG, "dog_dance2 music %s (%lums)", g_stop_requested.load() ? "stopped" : "done",
             static_cast<unsigned long>(duration_ms));
    vTaskDelete(nullptr);
}

static void WholetdogoutMusicTask(void* pvParameters) {
    const uint32_t duration_ms = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pvParameters));
    const TickType_t end_tick = xTaskGetTickCount() + pdMS_TO_TICKS(duration_ms);

    AudioCodec* codec = Board::GetInstance().GetAudioCodec();
    if (codec == nullptr) {
        ESP_LOGW(TAG, "no codec");
        vTaskDelete(nullptr);
        return;
    }

    RickyRobotAudioGateEnter();
    codec->EnableOutput(true);
    if (codec->output_sample_rate() != kMusicSampleRateHz) {
        if (!codec->SetOutputSampleRate(kMusicSampleRateHz)) {
            ESP_LOGW(TAG, "SetOutputSampleRate %d failed, playing at %d Hz", kMusicSampleRateHz,
                     codec->output_sample_rate());
        }
    }

    const unsigned char* raw = wholetdogout_raw;
    const unsigned int len = wholetdogout_raw_len;
    constexpr size_t kChunkBytes = 1024;

    while (!g_stop_requested.load() && xTaskGetTickCount() < end_tick) {
        for (size_t offset = 0;
             offset < len && !g_stop_requested.load() && xTaskGetTickCount() < end_tick;
             offset += kChunkBytes) {
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
    }

    (void)codec->SetOutputSampleRate(-1);
    RickyRobotAudioGateLeave();
    ESP_LOGI(TAG, "wholetdogout music %s (%lums)", g_stop_requested.load() ? "stopped" : "done",
             static_cast<unsigned long>(duration_ms));
    vTaskDelete(nullptr);
}

void RickyPlayDogDance2MusicForMs(uint32_t duration_ms) {
    if (duration_ms == 0) {
        return;
    }
    g_stop_requested.store(false);
    ESP_LOGI(TAG, "dog_dance2 music start %lums", static_cast<unsigned long>(duration_ms));
    const BaseType_t ok =
        xTaskCreate(DanceMusicTask, "ricky_dance2", 8192,
                    reinterpret_cast<void*>(static_cast<uintptr_t>(duration_ms)), 2, nullptr);
    if (ok != pdPASS) {
        ESP_LOGW(TAG, "xTaskCreate dance music failed");
    }
}

void RickyPlayWholetdogoutMusicForMs(uint32_t duration_ms) {
    if (duration_ms == 0) {
        return;
    }
    g_stop_requested.store(false);
    ESP_LOGI(TAG, "wholetdogout music start %lums", static_cast<unsigned long>(duration_ms));
    const BaseType_t ok =
        xTaskCreate(WholetdogoutMusicTask, "ricky_who13", 8192,
                    reinterpret_cast<void*>(static_cast<uintptr_t>(duration_ms)), 2, nullptr);
    if (ok != pdPASS) {
        ESP_LOGW(TAG, "xTaskCreate wholetdogout music failed");
    }
}

void RickyStopDanceMusicNow() {
    g_stop_requested.store(true);
}
