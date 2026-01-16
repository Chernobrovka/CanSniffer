/*
 * CanDriver.h
 *
 *  Created on: Dec 22, 2025
 *      Author: peleng
 */

#ifndef CAN_CANDRIVER_H_
#define CAN_CANDRIVER_H_

#include "App.hpp"
#include "Queue/cQueue.h"
#include "can.h"
#include <cstdint>
#include <cstdbool>


class CanDriver {
public:
    enum class State {
        STOPPED,
        ACTIVE,
        ERROR
    };

    enum class Status {
        OK = 0,
        ERROR = 1,
        NOT_STARTED = 2,
        INVALID_PARAM = 3,
		BUSY = 4,
		TIMEOUT = 5,
    };

    CanDriver(CAN_HandleTypeDef* can_ptr, Queue_t* queue_ptr);

    Status start();
    Status stop();
    Status setBaudrate(uint32_t baudrate);
    Status setMode(uint32_t mode);
    Status sendMessage(uint32_t id, bool is_extended, bool is_remote,
                       uint8_t* data, uint8_t dlc);

    // Фильтры
    Status setFilterAcceptAll(uint8_t filter_bank);
    Status setFilterStandardID(uint8_t filter_bank, uint16_t id);
    Status setFilterStandardIDWithMask(uint8_t filter_bank, uint16_t id, uint16_t mask);
    Status setFilterExtendedID(uint8_t filter_bank, uint32_t id);
    Status setFilterExtendedIDWithMask(uint8_t filter_bank, uint32_t id, uint32_t mask);
    Status setFilter(uint8_t filter_bank, uint8_t filter_slot,
                                           uint32_t id, uint32_t mask, bool is_extended);

    Status disableFilter(uint8_t filter_bank, uint8_t filter_slot);
    Status disableFilterBank(uint8_t filter_bank);
    Status disableAllFilters();

    Status activateNotification();
    Status deactivateNotification();

    void handleRxInterrupt(CAN_HandleTypeDef* hcan);

private:
    Status reconfigureBus();

    uint32_t baudrate_ = 1000;
    uint32_t mode_ = CAN_MODE_NORMAL;
    State state_ = State::STOPPED;
    uint32_t error_count_ = 0;
    CAN_HandleTypeDef* hcan_ = nullptr;
    Queue_t* queue_ = nullptr;

    Status checkHALStatus(HAL_StatusTypeDef hal_status);
};

#endif /* CAN_CANDRIVER_H_ */
