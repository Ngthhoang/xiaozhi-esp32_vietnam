#ifndef RICKY_STATUS_LED_H
#define RICKY_STATUS_LED_H

#include "led/single_led.h"

/**
 * WS2812 trên RICKY_STATUS_LED_GPIO:
 * WiFi đã kết nối + Idle: xanh | Đang nghe: đỏ | Đang nói: vàng.
 */
class RickyStatusLed : public SingleLed {
public:
    explicit RickyStatusLed(gpio_num_t gpio);
    void OnStateChanged() override;
};

#endif
