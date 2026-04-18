#ifndef RICKY_MOTOR_H_
#define RICKY_MOTOR_H_

#include <cstdint>
#include <driver/gpio.h>

/** Điều khiển DC motor qua cầu H: hai đường PWM LEDC (IN1 tiến, IN2 lùi), giống test_rb. */
class RickyMotor {
public:
    RickyMotor() = default;
    void Init(gpio_num_t in1, gpio_num_t in2);
    void Stop();
    void MotorStop() { Stop(); }
    void Forward(int speed_percent);
    void Backward(int speed_percent);
    /** Eye-only pulse: weak raw PWM, then stop, then off delay. */
    void EyeMotorPulseOnce(int weak_speed, uint32_t on_ms, uint32_t off_ms);

private:
    gpio_num_t in1_ = GPIO_NUM_NC;
    gpio_num_t in2_ = GPIO_NUM_NC;
    bool inited_ = false;
};

#endif  // RICKY_MOTOR_H_
