/*
 * LED.cpp
 *
 *  Created on: 15 янв. 2026 г.
 *      Author: peleng
 */

#include "LED.h"

Led::Led(GPIO_TypeDef* status_port, uint16_t status_pin,
                               GPIO_TypeDef* activity_port, uint16_t activity_pin)
    : status_port_(status_port)
    , status_pin_(status_pin)
    , activity_port_(activity_port)
    , activity_pin_(activity_pin)
    , current_status_(Status::OFF)
    , activity_flash_active_(false)
    , activity_flash_end_(0)
    , last_toggle_(0) {
    setStatusLed(false);
    setActivityLed(false);
}

void Led::update(uint32_t tick) {
    // Обновление статусного светодиода
    switch (current_status_) {
        case Status::SLOW_BLINK:
            if ((tick - last_toggle_) >= 500) {
                HAL_GPIO_TogglePin(status_port_, status_pin_);
                last_toggle_ = tick;
            }
            break;

        case Status::FAST_BLINK:
            if ((tick - last_toggle_) >= 100) {
                HAL_GPIO_TogglePin(status_port_, status_pin_);
                last_toggle_ = tick;
            }
            break;

        case Status::ERROR:
            // 3 быстрых мигания, пауза
            if ((tick - last_toggle_) >= 100) {
                static uint8_t blink_count = 0;
                HAL_GPIO_TogglePin(status_port_, status_pin_);
                last_toggle_ = tick;

                if (HAL_GPIO_ReadPin(status_port_, status_pin_) == GPIO_PIN_RESET) {
                    blink_count++;
                    if (blink_count >= 6) {
                        blink_count = 0;
                        last_toggle_ = tick + 1000; // Пауза 1 сек
                    }
                }
            }
            break;

        default:
            break;
    }

    // Обновление светодиода активности
    if (activity_flash_active_ && tick >= activity_flash_end_) {
        setActivityLed(false);
        activity_flash_active_ = false;
    }
}

void Led::setSystemStatus(Status status) {
    current_status_ = status;
    last_toggle_ = 0;

    switch (status) {
        case Status::OFF:
            setStatusLed(false);
            break;
        case Status::ON:
            setStatusLed(true);
            break;
        case Status::SLOW_BLINK:
        case Status::FAST_BLINK:
        case Status::ERROR:
            setStatusLed(true);
            break;
    }
}

void Led::indicateCanStarted(bool started) {
    setSystemStatus(started ? Status::SLOW_BLINK : Status::OFF);
}

void Led::indicateError(bool has_error) {
    setSystemStatus(has_error ? Status::ERROR : Status::SLOW_BLINK);
}

void Led::flashOnRx() {
    setActivityLed(true);
    activity_flash_active_ = true;
    activity_flash_end_ = HAL_GetTick() + 50;
}

void Led::flashOnTx() {
    setActivityLed(true);
    activity_flash_active_ = true;
    activity_flash_end_ = HAL_GetTick() + 50;
}

void Led::flashOnCommand() {
    setActivityLed(true);
    activity_flash_active_ = true;
    activity_flash_end_ = HAL_GetTick() + 100;
}

void Led::setStatusLed(bool on) {
    HAL_GPIO_WritePin(status_port_, status_pin_,
                     on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void Led::setActivityLed(bool on) {
    HAL_GPIO_WritePin(activity_port_, activity_pin_,
                     on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}


