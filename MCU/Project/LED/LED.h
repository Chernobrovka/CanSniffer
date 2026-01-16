/*
 * LED.h
 *
 *  Created on: 15 янв. 2026 г.
 *      Author: peleng
 */

#ifndef LED_LED_H_
#define LED_LED_H_

#include "main.h"
#include <cstdint>

class Led {
public:
    enum class Status {
        OFF,
        ON,
        SLOW_BLINK,
        FAST_BLINK,
        ERROR
    };

    Led(GPIO_TypeDef* status_port, uint16_t status_pin,
                   GPIO_TypeDef* activity_port, uint16_t activity_pin);

    void update(uint32_t tick);

    void setSystemStatus(Status status);
    void indicateCanStarted(bool started);
    void indicateError(bool has_error);

    void flashOnRx();
    void flashOnTx();
    void flashOnCommand();

private:
    GPIO_TypeDef* status_port_;
    uint16_t status_pin_;
    GPIO_TypeDef* activity_port_;
    uint16_t activity_pin_;

    Status current_status_;
    bool activity_flash_active_;
    uint32_t activity_flash_end_;
    uint32_t last_toggle_;

    void setStatusLed(bool on);
    void setActivityLed(bool on);
};

#endif /* LED_LED_H_ */
