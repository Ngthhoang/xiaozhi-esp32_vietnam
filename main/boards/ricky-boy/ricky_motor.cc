#include "ricky_motor.h"

#include <cmath>
#include <esp_err.h>
#include <esp_log.h>

#include "config.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "RickyMotor";

#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_CH_IN1 LEDC_CHANNEL_0
#define LEDC_CH_IN2 LEDC_CHANNEL_1
#define LEDC_DUTY_RES LEDC_TIMER_10_BIT
#define LEDC_MAX_DUTY ((1 << 10) - 1)
#define LEDC_FREQ_HZ 2000

static inline uint32_t PercentToLeDcDuty(int percent) {
    if (percent <= 0) {
        return 0;
    }
    if (percent > 100) {
        percent = 100;
    }
    uint32_t duty = (uint32_t)((percent * LEDC_MAX_DUTY) / 100);
    return duty ? duty : 1u;
}

/* Raw 1..100 -> duty 1..1023, không ép sàn chạy motor (dùng cho mắt). */
static inline uint32_t speed_to_duty_raw(int speed) {
    if (speed <= 0) {
        return 0;
    }
    if (speed > 100) {
        speed = 100;
    }
    uint32_t duty = (uint32_t)((speed * LEDC_MAX_DUTY) / 100);
    return duty ? duty : 1u;
}

/** 0 hoặc < MIN: dừng; không kéo lên nữa (theo cấu hình robot). */
static inline int ClampUserSpeedOrZero(int speed) {
    if (speed <= 0) {
        return 0;
    }
    if (speed < RICKY_SPEED_MIN_EFFECTIVE) {
        return 0;
    }
    if (speed > 100) {
        return 100;
    }
    return speed;
}

/** Tiến: % người dùng → PWM tuyến tính trên [MIN..100]. */
static uint32_t SpeedToDutyForward(int speed) {
    const int s = ClampUserSpeedOrZero(speed);
    if (s == 0) {
        return 0;
    }
    return PercentToLeDcDuty(s);
}

/**
 * Lùi: cùng thang % người dùng nhưng ánh xạ phi tuyến (trượt cơ khí).
 * Hiệu dụng eff% trong [MIN..100] rồi đổi sang duty.
 */
static uint32_t SpeedToDutyBackward(int speed) {
    const int s = ClampUserSpeedOrZero(speed);
    if (s == 0) {
        return 0;
    }
    const float span = static_cast<float>(100 - RICKY_SPEED_MIN_EFFECTIVE);
    const float t = static_cast<float>(s - RICKY_SPEED_MIN_EFFECTIVE) / span;
    const float shaped = powf(t, RICKY_BACKWARD_SPEED_CURVE_EXP);
    int eff_pct = RICKY_SPEED_MIN_EFFECTIVE +
                  static_cast<int>(lroundf(span * shaped));
    if (eff_pct < RICKY_SPEED_MIN_EFFECTIVE) {
        eff_pct = RICKY_SPEED_MIN_EFFECTIVE;
    }
    if (eff_pct > 100) {
        eff_pct = 100;
    }
    return PercentToLeDcDuty(eff_pct);
}

void RickyMotor::Init(gpio_num_t in1, gpio_num_t in2) {
    in1_ = in1;
    in2_ = in2;

    ledc_timer_config_t timer = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = false,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t ch1 = {
        .gpio_num = in1_,
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CH_IN1,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
        .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
        .flags = {.output_invert = 0},
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch1));

    ledc_channel_config_t ch2 = {
        .gpio_num = in2_,
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CH_IN2,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
        .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
        .flags = {.output_invert = 0},
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch2));

    inited_ = true;
    ESP_LOGI(TAG, "LEDC %d Hz, IN1=%d IN2=%d", LEDC_FREQ_HZ, (int)in1_, (int)in2_);
}

void RickyMotor::Stop() {
    if (!inited_) {
        return;
    }
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CH_IN1, 0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CH_IN1));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CH_IN2, 0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CH_IN2));
}

void RickyMotor::Forward(int speed_percent) {
    if (!inited_) {
        return;
    }
    uint32_t d = SpeedToDutyForward(speed_percent);
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CH_IN2, 0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CH_IN2));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CH_IN1, d));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CH_IN1));
}

void RickyMotor::Backward(int speed_percent) {
    if (!inited_) {
        return;
    }
    uint32_t d = SpeedToDutyBackward(speed_percent);
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CH_IN1, 0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CH_IN1));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CH_IN2, d));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CH_IN2));
}

void RickyMotor::EyeMotorPulseOnce(int weak_speed, uint32_t on_ms, uint32_t off_ms) {
    if (!inited_) {
        return;
    }
    const uint32_t d = speed_to_duty_raw(weak_speed);
    /* Chỉ kích 1 chiều, duty rất thấp để đèn sáng mà hạn chế lực kéo cơ khí. */
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CH_IN2, 0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CH_IN2));
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CH_IN1, d));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CH_IN1));
    vTaskDelay(pdMS_TO_TICKS(on_ms));
    MotorStop();
    vTaskDelay(pdMS_TO_TICKS(off_ms));
}
