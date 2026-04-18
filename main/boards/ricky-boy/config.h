#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

/*
 * Nguồn logic motor / preset: examples/test_rb/main/blink_example_main.c (LEDC 2 kHz, 10 bit,
 * tiến IN1 PWM + IN2=0, lùi IN2 PWM + IN1=0, speed_to_duty tối thiểu 70%).
 * Board Ricky_boy: main/boards/ricky-boy/ricky_motor.cc, ricky_actions.cc.
 *
 * Hiện tại: không màn hình (GetDisplay = NoDisplay). Flash nhẹ: CMake không gói DEFAULT_EMOJI_COLLECTION.
 * Sau này có LCD/OLED: sửa ricky_boy.cc khởi tạo panel + thêm DEFAULT_EMOJI_COLLECTION trong CMakeLists.
 */

/** Điều khiển cầu H: PWM tiến trên IN1, lùi trên IN2 (giống test_rb). Đèn mắt nối chung driver sáng theo motor. */
#define RICKY_MOTOR_IN1_GPIO GPIO_NUM_41
#define RICKY_MOTOR_IN2_GPIO GPIO_NUM_42

/**
 * Tốc độ MCP (%) — tiến và lùi có thể khác nhau (pace slow/fast/normal + custom).
 * Dưới RICKY_SPEED_MIN_EFFECTIVE: coi như 0 (robot yếu/không chạy ổn).
 * Mặc định 55: mức thực tế ổn định; chỉnh trong code hoặc pace=custom + speed.
 */
#define RICKY_SPEED_MIN_EFFECTIVE 45
#define RICKY_SPEED_DEFAULT 55
#define RICKY_FORWARD_SPEED_SLOW 48
#define RICKY_FORWARD_SPEED_FAST 72
#define RICKY_BACKWARD_SPEED_SLOW 48
#define RICKY_BACKWARD_SPEED_FAST 70

/**
 * Lùi: cơ khí trượt — cùng số % người dùng không tương ứng tuyến tính với quãng đường.
 * Ánh xạ hiệu dụng: eff = MIN + (100-MIN) * pow(t, EXP) với t = (speed-MIN)/(100-MIN).
 * EXP > 1: tăng PWM chậm hơn ở vùng tốc cao (giảm trượt khi max). Chỉnh thực nghiệm.
 */
#define RICKY_BACKWARD_SPEED_CURVE_EXP 1.2f

#define AUDIO_INPUT_SAMPLE_RATE 16000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000
#define AUDIO_I2S_METHOD_SIMPLEX

/** Mic + loa: chỉnh theo mạch thực tế (tham khảo Otto / test_rb I2S loa). */
#define AUDIO_I2S_MIC_GPIO_WS GPIO_NUM_4
#define AUDIO_I2S_MIC_GPIO_SCK GPIO_NUM_5
#define AUDIO_I2S_MIC_GPIO_DIN GPIO_NUM_6
#define AUDIO_I2S_SPK_GPIO_DOUT GPIO_NUM_7
#define AUDIO_I2S_SPK_GPIO_BCLK GPIO_NUM_15
#define AUDIO_I2S_SPK_GPIO_LRCK GPIO_NUM_16

#define BOOT_BUTTON_GPIO GPIO_NUM_0

#define RICKY_BOY_VERSION "1.0.0"

#endif  // _BOARD_CONFIG_H_
