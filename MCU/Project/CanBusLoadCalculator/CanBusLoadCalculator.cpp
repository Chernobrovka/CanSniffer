/*
 * CanBusLoadCalculator.cpp
 *
 *  Created on: 14 янв. 2026 г.
 *      Author: Dmitry
 */

#include "CanBusLoadCalculator.h"
#include "main.h"
#include <cmath>
#include <cstring>

CanBusLoadCalculator::CanBusLoadCalculator(uint32_t baudrate)
    : history_index_(0),
      history_count_(0),
      baudrate_(baudrate),
      calculation_window_ms_(CALCULATION_WINDOW_MS),
      total_messages_(0),
      total_bits_(0),
      error_count_(0) {

    reset();
    memset(message_history_, 0, sizeof(message_history_));
    memset(&last_result_, 0, sizeof(last_result_));
}

void CanBusLoadCalculator::reset() {
    history_index_ = 0;
    history_count_ = 0;
    total_messages_ = 0;
    total_bits_ = 0;
    error_count_ = 0;

    memset(&last_result_, 0, sizeof(last_result_));
}

void CanBusLoadCalculator::setBaudrate(uint32_t baudrate) {
    if (baudrate == 0) {
        error_count_++;
        return;
    }
    baudrate_ = baudrate;
}

void CanBusLoadCalculator::addStandardFrame(uint32_t id, uint8_t dlc, bool is_remote) {
    (void)id;  // ID не влияет на размер кадра, только на заголовок
    addMessage(false, dlc, is_remote);
}

void CanBusLoadCalculator::addExtendedFrame(uint32_t id, uint8_t dlc, bool is_remote) {
    (void)id;  // ID не влияет на размер кадра, только на заголовок
    addMessage(true, dlc, is_remote);
}

void CanBusLoadCalculator::addMessage(bool is_extended, uint8_t dlc, bool is_remote) {
    if (dlc > 8) {
        error_count_++;
        return;
    }

    uint32_t bits = calculateBitsInFrame(is_extended, dlc, is_remote);
    uint32_t current_time = HAL_GetTick();  // Или другая функция получения времени

    addToHistory(current_time, bits);

    total_messages_++;
    total_bits_ += bits;
}

CanBusLoadCalculator::BusLoadResult CanBusLoadCalculator::calculateLoad(uint32_t current_time_ms) {
    BusLoadResult result;

    // Очищаем старые записи
    cleanupOldRecords(current_time_ms);

    // Рассчитываем биты в окне
    uint32_t bits_in_window = calculateBitsInWindow(current_time_ms);
    uint32_t max_possible_bits = (baudrate_ * calculation_window_ms_) / 1000;

    // Рассчитываем нагрузку
    result.load_percentage = calculateLoadPercentage(bits_in_window, calculation_window_ms_, baudrate_);
    result.bitrate_actual = (bits_in_window * 1000) / calculation_window_ms_;
    result.message_count = history_count_;
    result.total_bits = bits_in_window;
    result.max_possible_bits = max_possible_bits;
    result.timestamp_ms = current_time_ms;

    // Сохраняем результат
    last_result_ = result;

    return result;
}

uint32_t CanBusLoadCalculator::calculateBitsInFrame(bool is_extended, uint8_t dlc, bool is_remote) {
    // Формула расчета бит в CAN кадре:
    // 1. SOF (Start of Frame) - 1 бит
    // 2. Identifier - 11 бит (STD) или 29 бит (EXT)
    // 3. RTR (Remote Transmission Request) - 1 бит
    // 4. IDE (Identifier Extension) - 1 бит (только для EXT)
    // 5. r0 (Reserved) - 1 бит
    // 6. DLC (Data Length Code) - 4 бита
    // 7. Data Field - 0-64 бит (dlc * 8)
    // 8. CRC - 15 бит
    // 9. CRC Delimiter - 1 бит
    // 10. ACK Slot - 1 бит
    // 11. ACK Delimiter - 1 бит
    // 12. EOF (End of Frame) - 7 бит
    // 13. IFS (Interframe Space) - 3 бита

    // Базовые биты (без данных)
    uint32_t base_bits = 1;  // SOF

    // Identifier
    if (is_extended) {
        base_bits += 29;  // Extended ID
        base_bits += 1;   // IDE bit (часть Extended ID)
    } else {
        base_bits += 11;  // Standard ID
    }

    // Дополнительные биты
    base_bits += 1;  // RTR
    if (!is_extended) {
        base_bits += 1;  // IDE bit для STD (всегда 0)
    }
    base_bits += 1;  // r0 (reserved)
    base_bits += 4;  // DLC

    // Данные (если не RTR)
    uint32_t data_bits = 0;
    if (!is_remote) {
        data_bits = dlc * 8;
    }

    // Остальные биты
    uint32_t footer_bits = 15 + 1 + 1 + 1 + 7 + 3;  // CRC(15) + CRC_Delim(1) + ACK(1) + ACK_Delim(1) + EOF(7) + IFS(3)

    return base_bits + data_bits + footer_bits;
}

float CanBusLoadCalculator::calculateLoadPercentage(uint32_t total_bits, uint32_t time_window_ms, uint32_t baudrate) {
    if (baudrate == 0 || time_window_ms == 0) {
        return 0.0f;
    }

    uint32_t max_possible_bits = (baudrate * time_window_ms) / 1000;
    if (max_possible_bits == 0) {
        return 100.0f;  // Теоретически невозможно
    }

    float percentage = (static_cast<float>(total_bits) * 100.0f) / max_possible_bits;

    // Ограничиваем 0-100%
    if (percentage < 0.0f) percentage = 0.0f;
    if (percentage > 100.0f) percentage = 100.0f;

    return percentage;
}

void CanBusLoadCalculator::addToHistory(uint32_t timestamp_ms, uint32_t bits) {
    message_history_[history_index_] = MessageRecord(timestamp_ms, bits);

    history_index_ = (history_index_ + 1) % MAX_RECORDS;
    if (history_count_ < MAX_RECORDS) {
        history_count_++;
    }
}

void CanBusLoadCalculator::cleanupOldRecords(uint32_t current_time_ms) {
    if (history_count_ == 0) return;

    size_t new_count = 0;
    size_t start_index = (history_index_ + MAX_RECORDS - history_count_) % MAX_RECORDS;

    for (size_t i = 0; i < history_count_; i++) {
        size_t index = (start_index + i) % MAX_RECORDS;
        MessageRecord& record = message_history_[index];

        // Если запись в пределах окна, сохраняем ее
        if (current_time_ms - record.timestamp_ms <= calculation_window_ms_) {
            if (i != new_count) {
                message_history_[new_count] = record;
            }
            new_count++;
        }
    }

    history_count_ = new_count;
    history_index_ = (start_index + new_count) % MAX_RECORDS;
}

uint32_t CanBusLoadCalculator::calculateBitsInWindow(uint32_t current_time_ms) const {
    if (history_count_ == 0) return 0;

    uint32_t total_bits = 0;
    size_t start_index = (history_index_ + MAX_RECORDS - history_count_) % MAX_RECORDS;

    for (size_t i = 0; i < history_count_; i++) {
        size_t index = (start_index + i) % MAX_RECORDS;
        const MessageRecord& record = message_history_[index];

        // Учитываем только записи в пределах окна
        if (current_time_ms - record.timestamp_ms <= calculation_window_ms_) {
            total_bits += record.bits;
        }
    }

    return total_bits;
}


