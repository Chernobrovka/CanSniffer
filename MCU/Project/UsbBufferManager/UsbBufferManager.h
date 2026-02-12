/*
 * UsbBufferManager.h
 *
 *  Created on: Feb 5, 2026
 *      Author: Dmitry
 */

#ifndef USBBUFFERMANAGER_USBBUFFERMANAGER_H_
#define USBBUFFERMANAGER_USBBUFFERMANAGER_H_

#include "Queue/cQueue.h"
#include <cstdint>
#include <cstring>
#include "usbd_cdc_if.h"


class UsbBufferManager {
public:
	static constexpr size_t FORCE_FLUSH_THRESHOLD = 8;
    static constexpr uint32_t FLUSH_TIMEOUT_US = 20000;
    static constexpr size_t MAX_PACKET_SIZE = 256;
    static constexpr size_t DEFAULT_QUEUE_SIZE = 64;

    struct RawData {
        uint8_t data[MAX_PACKET_SIZE];
        size_t length;

        RawData() : length(0) {
            memset(data, 0, sizeof(data));
        }

        RawData(const void* src, size_t len) : length(len) {
            if (src && len > 0 && len <= MAX_PACKET_SIZE) {
                memcpy(data, src, len);
            } else {
                length = 0;
            }
        }
    };

    enum class Status {
        OK,
        BUFFER_FULL,
        USB_BUSY,
        ERROR,
        QUEUE_ERROR
    };

    UsbBufferManager(bool *usb_busy_ptr = nullptr);
    ~UsbBufferManager();

    Status sendData(const void* data, size_t length);
    Status sendDataImmediate(const void* data, size_t length);

    void flush(bool force = false);
    void update();

    size_t getBufferedCount() const;
    size_t getDroppedCount() const { return dropped_count_; }
    bool isUsbBusy() const { return *usb_busy_; }
    bool isBufferFull() const;
    size_t getQueueSize() const { return queue_.rec_nb; }
    size_t getFreeSpace() const;

    void setFlushTimeout(uint32_t timeout_us) { flush_timeout_us_ = timeout_us; }
    void setMaxPacketSize(size_t size);
    void resizeBuffer(size_t new_size);

    static size_t formatString(char* buffer, size_t max_len,
                              const char* format, ...) __attribute__((format(printf, 3, 4)));

private:
    void sendPacket(const uint8_t* data, size_t length);
    uint32_t getCurrentMicroseconds() const;
    bool shouldFlush() const;
    size_t preparePacketFromQueue(uint8_t* buffer, size_t max_len);

    uint8_t queue_buffer_[DEFAULT_QUEUE_SIZE * sizeof(RawData)];
    Queue_t queue_;

    RawData temp_data_;

    size_t dropped_count_;
    uint32_t total_sent_;
    uint32_t total_flushes_;

    uint32_t last_message_time_us_;
    uint32_t last_flush_time_us_;

    bool *usb_busy_;
    bool flush_pending_;
    bool queue_initialized_;

    size_t max_packet_size_;
    uint32_t flush_timeout_us_;

    static constexpr size_t FORMAT_BUFFER_SIZE = 256;
    char format_buffer_[FORMAT_BUFFER_SIZE];
};


#endif /* USBBUFFERMANAGER_USBBUFFERMANAGER_H_ */
