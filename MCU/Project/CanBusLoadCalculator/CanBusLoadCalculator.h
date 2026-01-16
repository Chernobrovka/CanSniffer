/*
 * CanBusLoadCalculator.h
 *
 *  Created on: 14 янв. 2026 г.
 *      Author: peleng
 */

#ifndef CANBUSLOADCALCULATOR_CANBUSLOADCALCULATOR_H_
#define CANBUSLOADCALCULATOR_CANBUSLOADCALCULATOR_H_

#include <cstdint>
#include <cstddef>

#include <cstdint>
#include <cstddef>

class CanBusLoadCalculator {
public:
    // Конфигурация
    static constexpr uint32_t DEFAULT_BAUDRATE = 500000;  // 500 kbps
    static constexpr uint32_t CALCULATION_WINDOW_MS = 1000;  // Окно расчета 1 секунда

    // Результаты расчета нагрузки
    struct BusLoadResult {
        float load_percentage;      // Загрузка в процентах (0-100%)
        uint32_t bitrate_actual;    // Фактическая битрейт (бит/сек)
        uint32_t message_count;     // Количество сообщений в окне
        uint32_t total_bits;        // Всего бит в окне
        uint32_t max_possible_bits; // Максимум возможных бит в окне
        uint32_t timestamp_ms;      // Время расчета
    };

    CanBusLoadCalculator(uint32_t baudrate = DEFAULT_BAUDRATE);

    // Основные методы
    void reset();
    void setBaudrate(uint32_t baudrate);

    // Добавление сообщений для расчета
    void addStandardFrame(uint32_t id, uint8_t dlc, bool is_remote = false);
    void addExtendedFrame(uint32_t id, uint8_t dlc, bool is_remote = false);
    void addMessage(bool is_extended, uint8_t dlc, bool is_remote = false);

    // Расчет нагрузки
    BusLoadResult calculateLoad(uint32_t current_time_ms);
    float getCurrentLoadPercentage() const { return last_result_.load_percentage; }

    // Статистика
    uint32_t getTotalMessages() const { return total_messages_; }
    uint32_t getTotalBits() const { return total_bits_; }
    uint32_t getErrorCount() const { return error_count_; }

    // Утилиты
    static uint32_t calculateBitsInFrame(bool is_extended, uint8_t dlc, bool is_remote);
    static float calculateLoadPercentage(uint32_t total_bits, uint32_t time_window_ms, uint32_t baudrate);

private:
    // Структура для хранения сообщений в окне
    struct MessageRecord {
        uint32_t timestamp_ms;
        uint32_t bits;

        MessageRecord() : timestamp_ms(0), bits(0) {}
        MessageRecord(uint32_t time, uint32_t b) : timestamp_ms(time), bits(b) {}
    };

    static constexpr size_t MAX_RECORDS = 1000;  // Максимум записей в истории

    MessageRecord message_history_[MAX_RECORDS];
    size_t history_index_;
    uint32_t history_count_;

    uint32_t baudrate_;
    uint32_t calculation_window_ms_;

    // Статистика
    uint32_t total_messages_;
    uint32_t total_bits_;
    uint32_t error_count_;

    BusLoadResult last_result_;

    // Вспомогательные методы
    void addToHistory(uint32_t timestamp_ms, uint32_t bits);
    void cleanupOldRecords(uint32_t current_time_ms);
    uint32_t calculateBitsInWindow(uint32_t current_time_ms) const;
};

#endif /* CANBUSLOADCALCULATOR_CANBUSLOADCALCULATOR_H_ */
