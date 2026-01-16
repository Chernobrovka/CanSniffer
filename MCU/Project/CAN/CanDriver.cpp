/*
 * CanDriver.cpp
 *
 *  Created on: Dec 22, 2025
 *      Author: peleng
 */
#include "App.hpp"
#include "CanDriver.h"
#include "CanProcessor/CanProcessor.h"
#include <cstring>

CanDriver::CanDriver(CAN_HandleTypeDef* can_ptr, Queue_t* queue_ptr) {
	baudrate_ = 1000;
	mode_ = CAN_MODE_NORMAL;
	state_ = State::STOPPED;
	error_count_ = 0;
	hcan_ = can_ptr;
	queue_ = queue_ptr;
}

CanDriver::Status CanDriver::start(){
	if (!hcan_) return Status::ERROR;

	if (HAL_CAN_Start(hcan_) != HAL_OK){
        error_count_++;
        state_ = State::ERROR;
        return Status::ERROR;
	}

	#if 0
	if (activateNotification() != Status::OK){
        HAL_CAN_Stop(hcan_);
        state_ = State::ERROR;
        return Status::ERROR;
	}
	#endif

	state_ = State::ACTIVE;
	return Status::OK;
}

CanDriver::Status CanDriver::stop() {
    if (!hcan_) {
        return Status::ERROR;
    }

    if (HAL_CAN_Stop(hcan_) != HAL_OK) {
        state_ = State::ERROR;
        error_count_++;
        return Status::ERROR;
    }

    state_ = State::STOPPED;
    return Status::OK;
}

CanDriver::Status CanDriver::reconfigureBus() {
    if (HAL_CAN_DeInit(hcan_) != HAL_OK) {
        error_count_++;
        return Status::ERROR;
    }

    if (HAL_CAN_Init(hcan_) != HAL_OK) {
        state_ = State::ERROR;
        error_count_++;
        return Status::ERROR;
    }

    return Status::OK;
}

CanDriver::Status CanDriver::setBaudrate(uint32_t baudrate) {
    if (!hcan_) {
        return Status::ERROR;
    }

    // Сохраняем текущее состояние
    bool was_active = (state_ == State::ACTIVE);

    // Останавливаем перед изменением
    if (was_active) {
        Status stop_status = stop();
        if (stop_status != Status::OK) {
            return stop_status;
        }
    }

    // Устанавливаем предделитель
    switch (baudrate) {
        case 10:    hcan_->Init.Prescaler = 200; break;
        case 20:    hcan_->Init.Prescaler = 100; break;
        case 50:    hcan_->Init.Prescaler = 40; break;
        case 100:   hcan_->Init.Prescaler = 20; break;
        case 125:   hcan_->Init.Prescaler = 16; break;
        case 250:   hcan_->Init.Prescaler = 8; break;
        case 500:   hcan_->Init.Prescaler = 4; break;
        case 1000:  hcan_->Init.Prescaler = 2; break;
        default:    return Status::INVALID_PARAM;
    }

    baudrate_ = baudrate;

    // Переконфигурируем шину
    Status reconf_status = reconfigureBus();
    if (reconf_status != Status::OK) {
        return reconf_status;
    }

    // Возобновляем работу если была активна
    if (was_active) {
        return start();
    }

    return Status::OK;
}

CanDriver::Status CanDriver::setMode(uint32_t mode) {
    if (!hcan_) {
        return Status::ERROR;
    }

    // Сохраняем текущее состояние
    bool was_active = (state_ == State::ACTIVE);

    // Останавливаем перед изменением
    if (was_active) {
        Status stop_status = stop();
        if (stop_status != Status::OK) {
            return stop_status;
        }
    }

    // Устанавливаем режим
    switch (mode) {
        case CAN_MODE_NORMAL:
            hcan_->Init.Mode = CAN_MODE_NORMAL;
            break;
        case CAN_MODE_LOOPBACK:
            hcan_->Init.Mode = CAN_MODE_LOOPBACK;
            break;
        case CAN_MODE_SILENT:
            hcan_->Init.Mode = CAN_MODE_SILENT;
            break;
        case CAN_MODE_SILENT_LOOPBACK:
            hcan_->Init.Mode = CAN_MODE_SILENT_LOOPBACK;
            break;
        default:
            error_count_++;
            return Status::INVALID_PARAM;
    }

    mode_ = mode;

    // Переконфигурируем шину
    Status reconf_status = reconfigureBus();
    if (reconf_status != Status::OK) {
        return reconf_status;
    }

    // Возобновляем работу если была активна
    if (was_active) {
        return start();
    }

    return Status::OK;
}

CanDriver::Status CanDriver::sendMessage(uint32_t id, bool is_extended,
                                        bool is_remote, uint8_t* data,
                                        uint8_t dlc) {
    if (state_ != State::ACTIVE) {
        return Status::NOT_STARTED;
    }

    if (!hcan_ || !data || dlc > 8) {
        return Status::INVALID_PARAM;
    }

    uint32_t TxMailbox;
    CAN_TxHeaderTypeDef pHeader;
    pHeader.DLC = dlc;

    if (is_extended) {
        pHeader.IDE = CAN_ID_EXT;
        pHeader.ExtId = id;
    } else {
        pHeader.IDE = CAN_ID_STD;
        pHeader.StdId = id;
    }

    pHeader.RTR = is_remote ? CAN_RTR_REMOTE : CAN_RTR_DATA;
    pHeader.TransmitGlobalTime = DISABLE;

    if (HAL_CAN_AddTxMessage(this->hcan_, &pHeader, data, &TxMailbox) != HAL_OK) {
        state_ = State::ERROR;
        error_count_++;
        return Status::ERROR;
    }

    return Status::OK;
}

CanDriver::Status CanDriver::setFilterAcceptAll(uint8_t filter_bank) {
	return disableFilter(filter_bank, 0);
}

CanDriver::Status CanDriver::setFilterStandardID(uint8_t filter_bank, uint16_t id) {
    CAN_FilterTypeDef sFilterConfig;

    sFilterConfig.FilterBank = filter_bank;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = id << 5;  // 11-bit ID in bits 21-31
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = 0x7FF << 5;  // Mask for 11-bit ID
    sFilterConfig.FilterMaskIdLow = 0x0000;
    sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.SlaveStartFilterBank = 14;  // For dual CAN support

    HAL_StatusTypeDef hal_status = HAL_CAN_ConfigFilter(hcan_, &sFilterConfig);
    return checkHALStatus(hal_status);
}

CanDriver::Status CanDriver::setFilterStandardIDWithMask(uint8_t filter_bank, uint16_t id, uint16_t mask) {
    CAN_FilterTypeDef sFilterConfig;

    sFilterConfig.FilterBank = filter_bank;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = (id & 0x7FF) << 5;
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = (mask & 0x7FF) << 5;
    sFilterConfig.FilterMaskIdLow = 0x0000;

    sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.SlaveStartFilterBank = 14;

    HAL_StatusTypeDef hal_status = HAL_CAN_ConfigFilter(hcan_, &sFilterConfig);
    return checkHALStatus(hal_status);
}

CanDriver::Status CanDriver::setFilterExtendedID(uint8_t filter_bank, uint32_t id) {
    uint16_t low16bits = id & 0xFFFF;
    uint16_t high16Bits = (id >> 16) & 0xFFFF;

    CAN_FilterTypeDef sFilterConfig;

    sFilterConfig.FilterBank = filter_bank;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = high16Bits;
    sFilterConfig.FilterIdLow = low16bits << 3;  // Extended ID in bits 3-31
    sFilterConfig.FilterMaskIdHigh = 0xFFFF;
    sFilterConfig.FilterMaskIdLow = 0xFFF8;  // Mask for 29-bit ID (bits 3-31)
    sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.SlaveStartFilterBank = 14;  // For dual CAN support

    HAL_StatusTypeDef hal_status = HAL_CAN_ConfigFilter(hcan_, &sFilterConfig);
    return checkHALStatus(hal_status);
}

CanDriver::Status CanDriver::setFilterExtendedIDWithMask(uint8_t filter_bank, uint32_t id, uint32_t mask) {
    CAN_FilterTypeDef sFilterConfig;

    sFilterConfig.FilterBank = filter_bank;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = (id >> 13) & 0xFFFF;
    sFilterConfig.FilterIdLow = ((id & 0x1FFF) << 3) | 0x04;
    sFilterConfig.FilterMaskIdHigh = (mask >> 13) & 0xFFFF;
    sFilterConfig.FilterMaskIdLow = ((mask & 0x1FFF) << 3) | 0x04;
    sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.SlaveStartFilterBank = 14;

    HAL_StatusTypeDef hal_status = HAL_CAN_ConfigFilter(hcan_, &sFilterConfig);
    return checkHALStatus(hal_status);
}

CanDriver::Status CanDriver::setFilter(uint8_t filter_bank, uint8_t filter_slot,
                                       uint32_t id, uint32_t mask, bool is_extended) {
    if (is_extended) {
        return setFilterExtendedIDWithMask(filter_bank, id, mask);
    } else {
        return setFilterStandardIDWithMask(filter_bank, (uint16_t)id, (uint16_t)mask);
    }
}

CanDriver::Status CanDriver::disableFilter(uint8_t filter_bank, uint8_t filter_slot) {
    (void)filter_slot;  // В 32-битном режиме не используем слот

    CAN_FilterTypeDef sFilterConfig;

    sFilterConfig.FilterBank = filter_bank;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;

    // Настраиваем фильтр на прием ВСЕХ сообщений
    sFilterConfig.FilterIdHigh = 0x0000;
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = 0x0000;
    sFilterConfig.FilterMaskIdLow = 0x0000;

    sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    sFilterConfig.FilterActivation = ENABLE;  // Активируем, но пропускаем все

    HAL_StatusTypeDef hal_status = HAL_CAN_ConfigFilter(hcan_, &sFilterConfig);
    return checkHALStatus(hal_status);
}

CanDriver::Status CanDriver::disableFilterBank(uint8_t filter_bank) {
    CAN_FilterTypeDef sFilterConfig;

    sFilterConfig.FilterBank = filter_bank;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = 0x0000;
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = 0x0000;
    sFilterConfig.FilterMaskIdLow = 0x0000;
    sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    sFilterConfig.FilterActivation = DISABLE;  // Деактивируем полностью

    HAL_StatusTypeDef hal_status = HAL_CAN_ConfigFilter(hcan_, &sFilterConfig);
    return checkHALStatus(hal_status);
}

CanDriver::Status CanDriver::disableAllFilters() {
    Status overall_status = Status::OK;

    const uint8_t MAX_FILTER_BANKS = 14;  // Настроить под вашу модель

    for (uint8_t bank = 0; bank < MAX_FILTER_BANKS; bank++) {
        Status status = disableFilterBank(bank);
        if (status != Status::OK) {
            overall_status = Status::ERROR;
        }
    }
    return overall_status;
}

CanDriver::Status CanDriver::activateNotification() {
    if (!hcan_) {
        return Status::ERROR;
    }

    if (HAL_CAN_ActivateNotification(hcan_, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
        return Status::ERROR;
    }

    return Status::OK;
}

CanDriver::Status CanDriver::deactivateNotification() {
    if (!hcan_) {
        return Status::ERROR;
    }

    if (HAL_CAN_DeactivateNotification(hcan_, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
        return Status::ERROR;
    }

    return Status::OK;
}

void CanDriver::handleRxInterrupt(CAN_HandleTypeDef* hcan) {
	CAN_RxHeaderTypeDef header;
	uint8_t data[8];

	if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &header, data) == HAL_OK) {
		CanMessage_t msg;
		msg.timestamp_ms = HAL_GetTick();
		msg.header = header;
		memcpy(msg.data, data, header.DLC);

		q_push(this->queue_, &msg);
	}
}

CanDriver::Status CanDriver::checkHALStatus(HAL_StatusTypeDef hal_status) {
    switch (hal_status) {
        case HAL_OK:
            return Status::OK;
        case HAL_ERROR:
            state_ = State::ERROR;
            error_count_++;
            return Status::ERROR;
        case HAL_BUSY:
            return Status::BUSY;
        case HAL_TIMEOUT:
            return Status::TIMEOUT;
        default:
            state_ = State::ERROR;
            error_count_++;
            return Status::ERROR;
    }
}

extern "C" void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* hcan){
	sys->can_driver->handleRxInterrupt(hcan);
}
