/*
 * CanBusMonitor.h
 *
 *  Created on: 14 янв. 2026 г.
 *      Author: peleng
 */
#ifndef CANBUSMONITOR_CANBUSMONITOR_H_
#define CANBUSMONITOR_CANBUSMONITOR_H_

#include "CanBusLoadCalculator/CanBusLoadCalculator.h"
#include "usbd_cdc_if.h"
#include <cstdio>

class CanBusMonitor {
private:
    CanBusLoadCalculator load_calculator_;
    uint32_t last_print_time_;
    bool monitoring_enabled_;

public:
    CanBusMonitor(uint32_t baudrate = 1000000)
        : load_calculator_(baudrate),
          last_print_time_(0),
          monitoring_enabled_(false) {}

    void onMessageReceived(bool is_extended, uint32_t dlc, uint32_t is_rtr) {
        if (!monitoring_enabled_) return;

        load_calculator_.addMessage(is_extended, dlc, is_rtr);
    }

    void update(uint32_t current_time_ms) {
        if (!monitoring_enabled_) return;

        if (current_time_ms - last_print_time_ >= 1000) {
            CanBusLoadCalculator::BusLoadResult result =
                load_calculator_.calculateLoad(current_time_ms);

            printBusLoad(result);
            last_print_time_ = current_time_ms;
        }
    }

    void startMonitoring() {
        monitoring_enabled_ = true;
        load_calculator_.reset();
        last_print_time_ = HAL_GetTick();
        printf("CAN bus load monitoring started\r\n");
    }

    void stopMonitoring() {
        monitoring_enabled_ = false;
        printf("CAN bus load monitoring stopped\r\n");
    }

    bool isMonitoring() const { return monitoring_enabled_; }

    float getCurrentLoad() const {
        return load_calculator_.getCurrentLoadPercentage();
    }

private:
    void printBusLoad(const CanBusLoadCalculator::BusLoadResult& result) {
        char buffer[256];

        snprintf(buffer, sizeof(buffer),
                "\r\n=== CAN Bus Load ===\r\n"
                "Load:        %.1f%%\r\n"
                "Actual rate: %u bps\r\n"
                "Messages:    %u in last second\r\n"
                "Bits:        %u / %u max\r\n"
                "Time:        %u ms\r\n"
                "=====================\r\n",
                result.load_percentage,
                result.bitrate_actual,
                result.message_count,
                result.total_bits,
                result.max_possible_bits,
                result.timestamp_ms);

        CDC_Transmit_FS((uint8_t*)buffer, strlen(buffer));
    }
};

#endif /* CANBUSMONITOR_CANBUSMONITOR_H_ */
