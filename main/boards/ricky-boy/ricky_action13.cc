#include "ricky_action13.h"

#include <esp_log.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ricky_motor.h"

static const char* TAG = "RickyAction13";

/* Thin adapters: semantics giống test_rb motor_forward / motor_backward / motor_stop */
static inline void motor_forward(RickyMotor& m, int speed) {
    m.Forward(speed);
}
static inline void motor_backward(RickyMotor& m, int speed) {
    m.Backward(speed);
}
static inline void motor_stop(RickyMotor& m) {
    m.Stop();
}

void RickyRunAction13IntroBlend(RickyMotor& m) {
    /* Port nguyên khối test_rb blink_example_main.c case 12 (13a Intro blend). */
    ESP_LOGI(TAG, "Action 13a: Intro blend (A1 shake + A6 elastic)");
    TickType_t end = xTaskGetTickCount() + pdMS_TO_TICKS(RICKY_ACTION13_VARIANT_RUN_MS);
    while (xTaskGetTickCount() < end) {
        /* from Action 1: super shake */
        for (int i = 0; i < 5 && xTaskGetTickCount() < end; i++) {
            motor_forward(m, 100);
            vTaskDelay(pdMS_TO_TICKS(90));
            motor_stop(m);
            vTaskDelay(pdMS_TO_TICKS(20));
            motor_backward(m, 100);
            vTaskDelay(pdMS_TO_TICKS(90));
            motor_stop(m);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        /* from Action 6: elastic pull-back */
        for (int i = 0; i < 2 && xTaskGetTickCount() < end; i++) {
            motor_forward(m, 100);
            vTaskDelay(pdMS_TO_TICKS(220));
            motor_backward(m, 60);
            vTaskDelay(pdMS_TO_TICKS(360));
            motor_stop(m);
            vTaskDelay(pdMS_TO_TICKS(80));
        }
    }
}
