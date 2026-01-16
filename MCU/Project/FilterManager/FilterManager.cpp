/*
 * FilterManager.cpp
 *
 *  Created on: 13 янв. 2026 г.
 *      Author: Dmitry
 */
#include "FilterManager.h"
#include <cstdio>

FilterManager::FilterManager(PrintCallback print_cb,
							ConfigureFilterCallback config_filter_cb,
							DisableFilterCallback disable_filter_cb,
							DisableAllFiltersCallback disable_all_filters_cb)
    : active_filter_count_(0),
      used_bank_count_(0),
	  print_callback_(print_cb),
	  config_filter_callback_(config_filter_cb),
	  disable_filter_callback_(disable_filter_cb),
	  disable_all_filters_callback_(disable_all_filters_callback_){
    // Инициализация всех банков
    for (auto& bank : banks_) {
        bank.is_used = false;
        bank.used_slots = 0;
    }
}

bool FilterManager::addFilter(uint32_t id, uint32_t mask, FilterType type) {
    if (!isValidId(id, type)) {
        return false;
    }

    if (mask == 0) mask = getDefaultMask(type);

    if (!isValidMask(mask, type)) return false;

    if (filterExists(id)) {
        print_callback_("ERROR: Filter with ID 0x%08lX already exists\r\n", id);
        return false;
    }

    if (active_filter_count_ >= MAX_FILTERS) {
    	print_callback_("ERROR: Filter limit reached (%zu/%zu)\r\n",
               active_filter_count_, MAX_FILTERS);
        return false;
    }

    FilterSlot slot = findFreeSlot();
    if (!slot.isValid()) {
    	print_callback_("ERROR: No free filter slots available\r\n");
        return false;
    }

    uint8_t bank_num = slot.bank;
    uint8_t slot_num = slot.slot;

    Bank& bank = banks_[bank_num];

    FilterInfo& filter_info = bank.filters[slot_num];
    filter_info.id = id;
    filter_info.mask = mask;
    filter_info.type = type;
    filter_info.status = FilterStatus::ACTIVE;
    filter_info.bank_number = bank_num;
    filter_info.filter_index = slot_num;

    if (!config_filter_callback_(bank_num, slot_num, id, mask, (bool)type)) {
    	print_callback_("ERROR: Failed to configure hardware filter\r\n");
        filter_info.status = FilterStatus::ERROR;
        return false;
    }

    if (!bank.is_used) {
        bank.is_used = true;
        used_bank_count_++;
    }

    bank.used_slots++;
    active_filter_count_++;

    print_callback_("OK: Filter added - ID: 0x%08lX, Mask: 0x%08lX, Type: %s, Bank: %d, Slot: %d\r\n",
           id, mask, filterTypeToString(type), bank_num, slot_num);

    return true;
}

bool FilterManager::removeFilter(uint32_t id) {
    FilterSlot slot = findFilterSlot(id);
    if (!slot.isValid()) {
    	print_callback_("ERROR: Filter with ID 0x%08lX not found\r\n", id);
        return false;
    }

    uint8_t bank_num = slot.bank;
    uint8_t slot_num = slot.slot;

    Bank& bank = banks_[bank_num];
    FilterInfo& filter_info = bank.filters[slot_num];

    if (!disable_filter_callback_(bank_num, slot_num)) {
    	print_callback_("WARNING: Failed to disable hardware filter\r\n");
    }

    filter_info.status = FilterStatus::INACTIVE;
    bank.used_slots--;
    active_filter_count_--;

    if (bank.used_slots == 0) {
        bank.is_used = false;
        used_bank_count_--;
    }

    print_callback_("OK: Filter removed - ID: 0x%08lX, Bank: %d, Slot: %d\r\n",
           id, bank_num, slot_num);

    return true;
}

void FilterManager::removeAllFilters() {
	print_callback_("Removing all filters...\r\n");

    disable_all_filters_callback_();

    for (auto& bank : banks_) {
        bank.is_used = false;
        bank.used_slots = 0;

        for (auto& filter : bank.filters) {
            filter.status = FilterStatus::INACTIVE;
        }
    }

    active_filter_count_ = 0;
    used_bank_count_ = 0;

    print_callback_("OK: All filters removed\r\n");
}

const FilterManager::FilterInfo* FilterManager::findFilter(uint32_t id) const {
    FilterSlot slot = findFilterSlot(id);
    if (slot.isValid()) {
        return &banks_[slot.bank].filters[slot.slot];
    }
    return nullptr;
}

bool FilterManager::filterExists(uint32_t id) const {
    return findFilter(id) != nullptr;
}

size_t FilterManager::getActiveFilters(FilterInfo* buffer, size_t buffer_size) const {
    if (!buffer || buffer_size == 0) {
        return 0;
    }

    size_t count = 0;
    for (const auto& bank : banks_) {
        if (!bank.is_used) continue;

        for (const auto& filter : bank.filters) {
            if (filter.status == FilterStatus::ACTIVE && count < buffer_size) {
                buffer[count++] = filter;
            }
        }
    }

    return count;
}

void FilterManager::printFilterList() const {
    print_callback_("\r\n=== Active Filters ===\r\n");
    print_callback_("Total: %zu/%zu filters, %zu/%zu banks used\r\n",
           active_filter_count_, MAX_FILTERS,
           used_bank_count_, MAX_BANKS);
    print_callback_("--------------------------------\r\n");

    if (active_filter_count_ == 0) {
    	print_callback_("No active filters\r\n");
    } else {
    	print_callback_("#  Bank Slot ID         Mask        Type Status\r\n");
    	print_callback_("-- ---- ---- ---------- ---------- ---- ------\r\n");

        size_t index = 1;
        for (const auto& bank : banks_) {
            if (!bank.is_used) continue;

            for (const auto& filter : bank.filters) {
                if (filter.status == FilterStatus::ACTIVE) {
                	print_callback_("%-2zu %-4d %-4d 0x%08lX 0x%08lX %-4s %-6s\r\n",
                           index++,
                           filter.bank_number,
                           filter.filter_index,
                           filter.id,
                           filter.mask,
                           filterTypeToString(filter.type),
                           filterStatusToString(filter.status));
                }
            }
        }
    }
    print_callback_("================================\r\n\r\n");
}

void FilterManager::printFilterInfo(uint32_t id) const {
    const FilterInfo* filter = findFilter(id);
    if (!filter) {
    	print_callback_("ERROR: Filter 0x%08lX not found\r\n", id);
        return;
    }

    print_callback_("\r\n=== Filter Details ===\r\n");
    print_callback_("ID:          0x%08lX\r\n", filter->id);
    print_callback_("Mask:        0x%08lX\r\n", filter->mask);
    print_callback_("Type:        %s\r\n", filterTypeToString(filter->type));
    print_callback_("Status:      %s\r\n", filterStatusToString(filter->status));
    print_callback_("Bank:        %d\r\n", filter->bank_number);
    print_callback_("Slot:        %d\r\n", filter->filter_index);
    print_callback_("=====================\r\n\r\n");
}

const char* FilterManager::filterTypeToString(FilterType type) const {
    switch (type) {
        case FilterType::STD: return "STD";
        case FilterType::EXT: return "EXT";
        default: return "UNKNOWN";
    }
}

const char* FilterManager::filterStatusToString(FilterStatus status) const {
    switch (status) {
        case FilterStatus::INACTIVE: return "INACTIVE";
        case FilterStatus::ACTIVE: return "ACTIVE";
        case FilterStatus::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

bool FilterManager::isValidId(uint32_t id, FilterType type) {
    if (type == FilterType::STD) {
        return id <= 0x7FF;  // 11-bit
    } else {
        return id <= 0x1FFFFFFF;  // 29-bit
    }
}

bool FilterManager::isValidMask(uint32_t mask, FilterType type) {
    if (type == FilterType::STD) {
        return mask <= 0x7FF;  // 11-bit
    } else {
        return mask <= 0x1FFFFFFF;  // 29-bit
    }
}

// Private methods

FilterManager::FilterSlot FilterManager::findFreeSlot() {
    // Сначала ищем банк с одним свободным слотом
    for (uint8_t bank_num = 0; bank_num < MAX_BANKS; bank_num++) {
        Bank& bank = banks_[bank_num];

        if (bank.is_used && bank.hasFreeSlot()) {
            uint8_t slot = bank.getFreeSlot();
            if (slot < 2) {
                return FilterSlot(bank_num, slot);
            }
        }
    }

    // Если нет, ищем полностью свободный банк
    for (uint8_t bank_num = 0; bank_num < MAX_BANKS; bank_num++) {
        Bank& bank = banks_[bank_num];

        if (!bank.is_used) {
            return FilterSlot(bank_num, 0);  // Первый слот свободного банка
        }
    }

    return FilterSlot();  // Нет свободных мест
}

FilterManager::FilterSlot FilterManager::findFilterSlot(uint32_t id) const {
    for (uint8_t bank_num = 0; bank_num < MAX_BANKS; bank_num++) {
        const Bank& bank = banks_[bank_num];

        if (!bank.is_used) continue;

        for (uint8_t slot = 0; slot < 2; slot++) {
            if (bank.filters[slot].status == FilterStatus::ACTIVE &&
                bank.filters[slot].id == id) {
                return FilterSlot(bank_num, slot);
            }
        }
    }

    return FilterSlot();  // Не найден
}

uint32_t FilterManager::getDefaultMask(FilterType type) const {
    return (type == FilterType::STD) ? STD_MASK_DEFAULT : EXT_MASK_DEFAULT;
}
