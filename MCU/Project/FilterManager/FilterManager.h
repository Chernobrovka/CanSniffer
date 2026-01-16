/*
 * FilterManager.h
 *
 *  Created on: 13 янв. 2026 г.
 *      Author: Dmitry
 */

#ifndef FILTERMANAGER_FILTERMANAGER_H_
#define FILTERMANAGER_FILTERMANAGER_H_

#include <cstdint>
#include <cstddef>
#include <cstring>

class FilterManager {
public:

    typedef void (*PrintCallback)(const char* format, ...);
    typedef bool (*ConfigureFilterCallback)(uint8_t bank, uint8_t slot,
											uint32_t id, uint32_t mask,
											bool is_extended);
    typedef bool (*DisableFilterCallback) (uint8_t bank, uint8_t slot);
    typedef void (*DisableAllFiltersCallback)(void);


    struct FilterSlot {
        uint8_t bank;
        uint8_t slot;
        bool valid;

        FilterSlot() : bank(0xFF), slot(0xFF), valid(false) {}
        FilterSlot(uint8_t b, uint8_t s) : bank(b), slot(s), valid(true) {}

        bool isValid() const { return valid; }
    };

    enum class FilterType {
        STD = 0,  // Стандартный (11-bit)
        EXT       // Расширенный (29-bit)
    };

    enum class FilterStatus {
        INACTIVE = 0,
        ACTIVE,
        ERROR
    };

    struct FilterInfo {
        uint32_t id;
        uint32_t mask;
        FilterType type;
        FilterStatus status;
        uint8_t bank_number;
        uint8_t filter_index;  // 0 или 1 для 16-битного режима

        FilterInfo() : id(0), mask(0), type(FilterType::STD),
                      status(FilterStatus::INACTIVE),
                      bank_number(0), filter_index(0) {}

        void clear() {
            id = 0;
            mask = 0;
            type = FilterType::STD;
            status = FilterStatus::INACTIVE;
            bank_number = 0;
            filter_index = 0;
        }
    };

    // Конфигурация
    static constexpr size_t MAX_FILTERS = 28;      // Максимум фильтров (14 банков × 2)
    static constexpr size_t MAX_BANKS = 14;        // Для STM32F1xx
    static constexpr uint32_t STD_MASK_DEFAULT = 0x7FF;      // Маска по умолчанию для STD
    static constexpr uint32_t EXT_MASK_DEFAULT = 0x1FFFFFFF; // Маска по умолчанию для EXT

    FilterManager(PrintCallback print_cb,
    		ConfigureFilterCallback config_filter_cb,
			DisableFilterCallback disable_filter_cb,
			DisableAllFiltersCallback disable_all_filters_cb);

    // Основные методы
    bool addFilter(uint32_t id, uint32_t mask = 0, FilterType type = FilterType::STD);
    bool removeFilter(uint32_t id);
    void removeAllFilters();

    // Поиск фильтров
    const FilterInfo* findFilter(uint32_t id) const;
    bool filterExists(uint32_t id) const;

    // Получение информации
    size_t getActiveFilterCount() const { return active_filter_count_; }
    size_t getTotalFilterCount() const { return MAX_FILTERS; }
    size_t getUsedBankCount() const { return used_bank_count_; }
    size_t getFreeFilterCount() const { return MAX_FILTERS - active_filter_count_; }
    size_t getActiveFilters(FilterInfo* buffer, size_t buffer_size) const;

    // Утилиты
    void printFilterList() const;
    void printFilterInfo(uint32_t id) const;
    const char* filterTypeToString(FilterType type) const;
    const char* filterStatusToString(FilterStatus status) const;

    // Валидация
    static bool isValidId(uint32_t id, FilterType type);
    static bool isValidMask(uint32_t mask, FilterType type);

private:
    struct Bank {
        FilterInfo filters[2];  // 2 фильтра в 16-битном режиме
        bool is_used;
        uint8_t used_slots;

        Bank() : is_used(false), used_slots(0) {
            memset(filters, 0, sizeof(filters));
        }

        bool hasFreeSlot() const { return used_slots < 2; }
        uint8_t getFreeSlot() const {
            for (uint8_t i = 0; i < 2; i++) {
                if (filters[i].status != FilterStatus::ACTIVE) {
                    return i;
                }
            }
            return 2;  // Нет свободных слотов
        }
    };

    Bank banks_[MAX_BANKS];
    size_t active_filter_count_;
    size_t used_bank_count_;

    PrintCallback print_callback_;
    ConfigureFilterCallback config_filter_callback_;
    DisableFilterCallback disable_filter_callback_;
    DisableAllFiltersCallback disable_all_filters_callback_;

    FilterSlot findFreeSlot();
    FilterSlot findFilterSlot(uint32_t id) const;
    uint32_t getDefaultMask(FilterType type) const;
};




#endif /* FILTERMANAGER_FILTERMANAGER_H_ */
