#include "ricky_actions.h"

#include <cstdint>
#include <esp_log.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ricky_bark.h"
#include "ricky_action13.h"
#include "ricky_dance_music.h"
#include "ricky_motor.h"

static const char* TAG = "RickyActions";
static constexpr int EYE_REST_BETWEEN_ACTION_MS = 3000;
static constexpr int EYE_WEAK_SPEED_MIN = 1;
static constexpr int EYE_WEAK_SPEED_LOW = 2;
static constexpr int EYE_WEAK_SPEED_MID = 3;
static constexpr int EYE_WEAK_SPEED_MAX = 4;

static void eye_motor_pulse_once(RickyMotor& m, int weak_speed, uint32_t on_ms, uint32_t off_ms) {
    m.EyeMotorPulseOnce(weak_speed, on_ms, off_ms);
}

static void eye_emotion_calm(RickyMotor& m) {
    eye_motor_pulse_once(m, EYE_WEAK_SPEED_LOW, 90, 360);
    eye_motor_pulse_once(m, EYE_WEAK_SPEED_LOW, 90, 900);
}

static void eye_emotion_happy(RickyMotor& m) {
    for (int i = 0; i < 3; i++) {
        eye_motor_pulse_once(m, EYE_WEAK_SPEED_MID, 50, 170);
    }
    vTaskDelay(pdMS_TO_TICKS(500));
}

static void eye_emotion_surprised(RickyMotor& m) {
    eye_motor_pulse_once(m, EYE_WEAK_SPEED_MAX, 140, 90);
    eye_motor_pulse_once(m, EYE_WEAK_SPEED_MID, 55, 180);
    eye_motor_pulse_once(m, EYE_WEAK_SPEED_MID, 55, 600);
}

static void eye_emotion_sad(RickyMotor& m) {
    eye_motor_pulse_once(m, EYE_WEAK_SPEED_LOW, 55, 580);
    eye_motor_pulse_once(m, EYE_WEAK_SPEED_LOW, 55, 900);
    eye_motor_pulse_once(m, EYE_WEAK_SPEED_LOW, 55, 720);
}

static void eye_emotion_sleepy(RickyMotor& m) {
    eye_motor_pulse_once(m, EYE_WEAK_SPEED_MIN, 45, 720);
    eye_motor_pulse_once(m, EYE_WEAK_SPEED_MIN, 45, 720);
    eye_motor_pulse_once(m, EYE_WEAK_SPEED_MIN, 45, 920);
}

static void eye_emotion_alert(RickyMotor& m) {
    for (int i = 0; i < 4; i++) {
        eye_motor_pulse_once(m, EYE_WEAK_SPEED_MAX, 45, 110);
    }
    vTaskDelay(pdMS_TO_TICKS(450));
}

static void eye_emotion_headlight(RickyMotor& m) {
    eye_motor_pulse_once(m, EYE_WEAK_SPEED_MAX, 1200, 120);
    eye_motor_pulse_once(m, EYE_WEAK_SPEED_MID, 260, 500);
}

void RickyRunPreset(RickyMotor& m, int index_0_to_12) {
    if (index_0_to_12 < 0 || index_0_to_12 > 12) {
        ESP_LOGW(TAG, "preset id out of range: %d", index_0_to_12);
        return;
    }

    ESP_LOGI(TAG, "preset #%d start", index_0_to_12 + 1);

    /* Preset MCP 1–10 (index 0–9): phối hợp sủa ngắn hoặc dài ngẫu nhiên. */
    if (index_0_to_12 >= 0 && index_0_to_12 <= 9) {
        RickyPlayBarkRandomForPresetBasic();
    }

    switch (index_0_to_12) {
        case 0:
            ESP_LOGI(TAG, "Super Shake");
            for (int i = 0; i < 12; i++) {
                m.Forward(100);
                vTaskDelay(pdMS_TO_TICKS(90));
                m.Stop();
                vTaskDelay(pdMS_TO_TICKS(20));
                m.Backward(100);
                vTaskDelay(pdMS_TO_TICKS(90));
                m.Stop();
                vTaskDelay(pdMS_TO_TICKS(20));
            }
            break;

        case 1: {
            ESP_LOGI(TAG, "Sneaky Steps");
            static const int k_ramp[] = {52, 70, 86, 98};
            const int k_ramp_ms = 16;
            for (int i = 0; i < 6; i++) {
                for (size_t r = 0; r < sizeof(k_ramp) / sizeof(k_ramp[0]); r++) {
                    m.Forward(k_ramp[r]);
                    vTaskDelay(pdMS_TO_TICKS(k_ramp_ms));
                }
                m.Stop();
                vTaskDelay(pdMS_TO_TICKS(22));
                vTaskDelay(pdMS_TO_TICKS(72));
            }
            break;
        }

        case 2:
            ESP_LOGI(TAG, "Scaredy Cat");
            m.Backward(100);
            vTaskDelay(pdMS_TO_TICKS(400));
            for (int i = 0; i < 10; i++) {
                m.Forward(50);
                vTaskDelay(pdMS_TO_TICKS(40));
                m.Backward(50);
                vTaskDelay(pdMS_TO_TICKS(40));
            }
            break;

        case 3:
            ESP_LOGI(TAG, "Force Move");
            for (int k = 0; k < 3; k++) {
                m.Forward(100);
                vTaskDelay(pdMS_TO_TICKS(850));
                m.Stop();
                vTaskDelay(pdMS_TO_TICKS(150));
                m.Backward(100);
                vTaskDelay(pdMS_TO_TICKS(850));
                m.Stop();
                vTaskDelay(pdMS_TO_TICKS(150));
            }
            break;

        case 4:
            ESP_LOGI(TAG, "Double Hop stride");
            for (int i = 0; i < 2; i++) {
                for (int s = 72; s <= 98; s += 2) {
                    m.Forward(s);
                    vTaskDelay(pdMS_TO_TICKS(110));
                }
                m.Forward(98);
                vTaskDelay(pdMS_TO_TICKS(450));
                for (int s = 98; s >= 72; s -= 2) {
                    m.Forward(s);
                    vTaskDelay(pdMS_TO_TICKS(110));
                }
                m.Stop();
                vTaskDelay(pdMS_TO_TICKS(420));
                for (int s = 72; s <= 98; s += 2) {
                    m.Backward(s);
                    vTaskDelay(pdMS_TO_TICKS(110));
                }
                m.Backward(98);
                vTaskDelay(pdMS_TO_TICKS(450));
                for (int s = 98; s >= 72; s -= 2) {
                    m.Backward(s);
                    vTaskDelay(pdMS_TO_TICKS(110));
                }
                m.Stop();
                vTaskDelay(pdMS_TO_TICKS(420));
            }
            break;

        case 5:
            ESP_LOGI(TAG, "Elastic");
            for (int i = 0; i < 4; i++) {
                m.Forward(100);
                vTaskDelay(pdMS_TO_TICKS(250));
                m.Backward(60);
                vTaskDelay(pdMS_TO_TICKS(400));
                m.Stop();
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            break;

        case 6:
            ESP_LOGI(TAG, "Engine Start");
            for (int i = 0; i < 5; i++) {
                m.Forward(85);
                vTaskDelay(pdMS_TO_TICKS(120));
                m.Backward(85);
                vTaskDelay(pdMS_TO_TICKS(120));
                m.Stop();
                vTaskDelay(pdMS_TO_TICKS(80));
            }
            m.Forward(75);
            vTaskDelay(pdMS_TO_TICKS(350));
            m.Forward(90);
            vTaskDelay(pdMS_TO_TICKS(450));
            m.Forward(100);
            vTaskDelay(pdMS_TO_TICKS(1300));
            break;

        case 7:
            ESP_LOGI(TAG, "Confused");
            /* Tối thiểu hiệu dụng 45% (ricky_motor); 40 trước đây sẽ thành dừng. */
            m.Forward(45);
            vTaskDelay(pdMS_TO_TICKS(300));
            m.Stop();
            vTaskDelay(pdMS_TO_TICKS(400));
            m.Backward(45);
            vTaskDelay(pdMS_TO_TICKS(300));
            m.Stop();
            vTaskDelay(pdMS_TO_TICKS(400));
            m.Forward(45);
            vTaskDelay(pdMS_TO_TICKS(150));
            m.Backward(45);
            vTaskDelay(pdMS_TO_TICKS(150));
            break;

        case 8:
            ESP_LOGI(TAG, "Charge & Brake");
            m.Forward(100);
            vTaskDelay(pdMS_TO_TICKS(800));
            m.Backward(100);
            vTaskDelay(pdMS_TO_TICKS(50));
            m.Stop();
            break;

        case 9:
            ESP_LOGI(TAG, "Heartbeat");
            for (int i = 0; i < 6; i++) {
                m.Forward(60);
                vTaskDelay(pdMS_TO_TICKS(80));
                m.Stop();
                vTaskDelay(pdMS_TO_TICKS(80));
                m.Forward(90);
                vTaskDelay(pdMS_TO_TICKS(120));
                m.Stop();
                vTaskDelay(pdMS_TO_TICKS(600));
            }
            break;

        case 10:
            ESP_LOGI(TAG, "dog_dance1 sequence + dog_dance2 music");
            /* Chuỗi motor ~14s; nhạc dog_dance2 lặp trong khoảng này (PCM 24 kHz, file dog_dance2.h). */
            RickyPlayDogDance2MusicForMs(14500);
            for (int i = 0; i < 8; i++) {
                m.Backward(100);
                vTaskDelay(pdMS_TO_TICKS(320));
                m.Stop();
                vTaskDelay(pdMS_TO_TICKS(20));
                m.Backward(80);
                vTaskDelay(pdMS_TO_TICKS(140));
                m.Stop();
                vTaskDelay(pdMS_TO_TICKS(20));
            }
            for (int i = 0; i < 20; i++) {
                m.Forward(95);
                vTaskDelay(pdMS_TO_TICKS(110));
                m.Stop();
                vTaskDelay(pdMS_TO_TICKS(15));
                m.Backward(95);
                vTaskDelay(pdMS_TO_TICKS(110));
                m.Stop();
                vTaskDelay(pdMS_TO_TICKS(15));
            }
            for (int i = 0; i < 4; i++) {
                m.Forward(100);
                vTaskDelay(pdMS_TO_TICKS(1000));
                m.Stop();
                vTaskDelay(pdMS_TO_TICKS(250));
            }
            break;

        case 11:
            ESP_LOGI(TAG, "dog_dance5 (30s) + dog_dance2 music");
            RickyPlayDogDance2MusicForMs(30000);
            for (int stage = 0; stage < 5; stage++) {
                TickType_t stage_start = xTaskGetTickCount();
                TickType_t stage_end = stage_start + pdMS_TO_TICKS(6000);
                while (xTaskGetTickCount() < stage_end) {
                    switch (stage) {
                        case 0:
                            m.Forward(100);
                            vTaskDelay(pdMS_TO_TICKS(110));
                            m.Stop();
                            vTaskDelay(pdMS_TO_TICKS(20));
                            m.Backward(100);
                            vTaskDelay(pdMS_TO_TICKS(110));
                            m.Stop();
                            vTaskDelay(pdMS_TO_TICKS(20));
                            break;
                        case 1:
                            m.Backward(100);
                            vTaskDelay(pdMS_TO_TICKS(280));
                            for (int j = 0; j < 3; j++) {
                                m.Forward(72);
                                vTaskDelay(pdMS_TO_TICKS(90));
                                m.Backward(72);
                                vTaskDelay(pdMS_TO_TICKS(90));
                            }
                            break;
                        case 2:
                            m.Forward(100);
                            vTaskDelay(pdMS_TO_TICKS(700));
                            m.Stop();
                            vTaskDelay(pdMS_TO_TICKS(120));
                            m.Backward(100);
                            vTaskDelay(pdMS_TO_TICKS(700));
                            m.Stop();
                            vTaskDelay(pdMS_TO_TICKS(120));
                            break;
                        case 3:
                            m.Forward(85);
                            vTaskDelay(pdMS_TO_TICKS(140));
                            m.Backward(85);
                            vTaskDelay(pdMS_TO_TICKS(140));
                            m.Stop();
                            vTaskDelay(pdMS_TO_TICKS(80));
                            m.Forward(100);
                            vTaskDelay(pdMS_TO_TICKS(260));
                            m.Stop();
                            vTaskDelay(pdMS_TO_TICKS(120));
                            break;
                        default:
                            m.Forward(70);
                            vTaskDelay(pdMS_TO_TICKS(120));
                            m.Stop();
                            vTaskDelay(pdMS_TO_TICKS(90));
                            m.Forward(95);
                            vTaskDelay(pdMS_TO_TICKS(170));
                            m.Stop();
                            vTaskDelay(pdMS_TO_TICKS(220));
                            break;
                    }
                }
            }
            break;

        case 12:
            ESP_LOGI(TAG, "Action 13 + wholetdogout music (%d ms)", RICKY_ACTION13_VARIANT_RUN_MS);
            RickyPlayWholetdogoutMusicForMs(RICKY_ACTION13_VARIANT_RUN_MS);
            RickyRunAction13IntroBlend(m);
            break;
    }

    m.Stop();
    ESP_LOGI(TAG, "preset #%d done", index_0_to_12 + 1);
}

void RickyRunDanceBase(RickyMotor& m, int index_0_to_6) {
    if (index_0_to_6 < 0 || index_0_to_6 > 6) {
        ESP_LOGW(TAG, "dance id out of range: %d", index_0_to_6);
        return;
    }

    ESP_LOGI(TAG, "dance%d start (base/no bg-audio)", index_0_to_6 + 1);
    switch (index_0_to_6) {
        case 0:
            ESP_LOGI(TAG, "dance1 = Super Shake");
            for (int i = 0; i < 12; i++) {
                m.Forward(100);
                vTaskDelay(pdMS_TO_TICKS(90));
                m.Stop();
                vTaskDelay(pdMS_TO_TICKS(20));
                m.Backward(100);
                vTaskDelay(pdMS_TO_TICKS(90));
                m.Stop();
                vTaskDelay(pdMS_TO_TICKS(20));
            }
            break;
        case 1: {
            ESP_LOGI(TAG, "dance2 = Sneaky Steps");
            static const int k_ramp[] = {52, 70, 86, 98};
            const int k_ramp_ms = 16;
            for (int i = 0; i < 6; i++) {
                for (size_t r = 0; r < sizeof(k_ramp) / sizeof(k_ramp[0]); r++) {
                    m.Forward(k_ramp[r]);
                    vTaskDelay(pdMS_TO_TICKS(k_ramp_ms));
                }
                m.Stop();
                vTaskDelay(pdMS_TO_TICKS(22));
                vTaskDelay(pdMS_TO_TICKS(72));
            }
            break;
        }
        case 2:
            ESP_LOGI(TAG, "dance3 = Scaredy Cat");
            m.Backward(100);
            vTaskDelay(pdMS_TO_TICKS(400));
            for (int i = 0; i < 10; i++) {
                m.Forward(50);
                vTaskDelay(pdMS_TO_TICKS(40));
                m.Backward(50);
                vTaskDelay(pdMS_TO_TICKS(40));
            }
            break;
        case 3:
            ESP_LOGI(TAG, "dance4 = Force Move");
            for (int k = 0; k < 3; k++) {
                m.Forward(100);
                vTaskDelay(pdMS_TO_TICKS(850));
                m.Stop();
                vTaskDelay(pdMS_TO_TICKS(150));
                m.Backward(100);
                vTaskDelay(pdMS_TO_TICKS(850));
                m.Stop();
                vTaskDelay(pdMS_TO_TICKS(150));
            }
            break;
        case 4:
            ESP_LOGI(TAG, "dance5 = Elastic");
            for (int i = 0; i < 4; i++) {
                m.Forward(100);
                vTaskDelay(pdMS_TO_TICKS(250));
                m.Backward(60);
                vTaskDelay(pdMS_TO_TICKS(400));
                m.Stop();
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            break;
        case 5:
            ESP_LOGI(TAG, "dance6 = Confused");
            m.Forward(45);
            vTaskDelay(pdMS_TO_TICKS(300));
            m.Stop();
            vTaskDelay(pdMS_TO_TICKS(400));
            m.Backward(45);
            vTaskDelay(pdMS_TO_TICKS(300));
            m.Stop();
            vTaskDelay(pdMS_TO_TICKS(400));
            m.Forward(45);
            vTaskDelay(pdMS_TO_TICKS(150));
            m.Backward(45);
            vTaskDelay(pdMS_TO_TICKS(150));
            break;
        case 6:
            ESP_LOGI(TAG, "dance7 = Heartbeat");
            for (int i = 0; i < 6; i++) {
                m.Forward(60);
                vTaskDelay(pdMS_TO_TICKS(80));
                m.Stop();
                vTaskDelay(pdMS_TO_TICKS(80));
                m.Forward(90);
                vTaskDelay(pdMS_TO_TICKS(120));
                m.Stop();
                vTaskDelay(pdMS_TO_TICKS(600));
            }
            break;
    }

    m.Stop();
    ESP_LOGI(TAG, "dance%d done", index_0_to_6 + 1);
}

void RickyRunEyeAction(RickyMotor& m, int index_0_to_6) {
    if (index_0_to_6 < 0 || index_0_to_6 > 6) {
        ESP_LOGW(TAG, "eye action id out of range: %d", index_0_to_6);
        return;
    }

    struct EyeAction {
        const char* name;
        const char* note;
        void (*fn)(RickyMotor&);
    };
    static const EyeAction kActions[] = {
        {"1_CALM_DOUBLE", "Binh tinh: chop cham 2 nhip", eye_emotion_calm},
        {"2_HAPPY_TRIPLE", "Vui: chop nhanh 3 nhip", eye_emotion_happy},
        {"3_SURPRISED_HOLD", "Ngac nhien: mo mat lau + chop doi", eye_emotion_surprised},
        {"4_SAD_SPARSE", "Buon: chop thua, nghi dai", eye_emotion_sad},
        {"5_SLEEPY_LONG_OFF", "Buon ngu: mo ngan, tat dai", eye_emotion_sleepy},
        {"6_ALERT_STROBE", "Canh giac: nhay nhanh manh", eye_emotion_alert},
        {"7_HEADLIGHT_ON", "Bat den pha: sang lien tuc", eye_emotion_headlight},
    };

    const EyeAction& action = kActions[index_0_to_6];
    ESP_LOGI(TAG, "=== Eye Action %d/7: %s ===", index_0_to_6 + 1, action.name);
    ESP_LOGI(TAG, "Note: %s", action.note);
    action.fn(m);
    m.MotorStop();
    ESP_LOGI(TAG, "--- Nghi %dms ---", EYE_REST_BETWEEN_ACTION_MS);
    vTaskDelay(pdMS_TO_TICKS(EYE_REST_BETWEEN_ACTION_MS));
}
