/*
 * UsbBufferManager.cpp
 *
 *  Created on: Feb 5, 2026
 *      Author: Dmitry
 */
#include "UsbBufferManager.h"
#include "main.h"
#include <cstdio>
#include <cstdarg>

UsbBufferManager::UsbBufferManager(bool *usb_busy_ptr)
    : dropped_count_(0)
    , total_sent_(0)
    , total_flushes_(0)
    , last_message_time_us_(0)
    , last_flush_time_us_(0)
    , usb_busy_(usb_busy_ptr ? usb_busy_ptr : []() {
        static bool default_busy = false;
        return &default_busy;
    }())
    , flush_pending_(false)
    , queue_initialized_(false)
    , flush_timeout_us_(FLUSH_TIMEOUT_US) {

    memset(format_buffer_, 0, sizeof(format_buffer_));
    memset(&queue_, 0, sizeof(queue_));

    q_init_static(&queue_,
                  queue_buffer_,
                  sizeof(RawData),
                  DEFAULT_QUEUE_SIZE,
                  FIFO,
                  true);
}

UsbBufferManager::~UsbBufferManager() {
    flush(true); // Отправляем все оставшиеся сообщения
    q_kill(&queue_);
}

uint32_t UsbBufferManager::getCurrentMicroseconds() const {
    return HAL_GetTick() * 1000;
}

size_t UsbBufferManager::getBufferedCount() const {
    return q_getCount(&queue_);
}

size_t UsbBufferManager::getFreeSpace() const {
    return q_getRemainingCount(&queue_);
}

bool UsbBufferManager::isBufferFull() const {
    return q_isFull(&queue_);
}

void UsbBufferManager::setMaxPacketSize(size_t size) {
    if (size > 0 && size <= MAX_PACKET_SIZE) {
        max_packet_size_ = size;
    }
}

UsbBufferManager::Status UsbBufferManager::sendData(const void* data, size_t length) {
    if (!data || length == 0 || length > MAX_PACKET_SIZE) {
        return Status::ERROR;
    }

    RawData packet(data, length);

    // Используем q_push с объектом queue_
    if (!q_push(&queue_, &packet)) {
        dropped_count_++;
        return Status::BUFFER_FULL;
    }

    last_message_time_us_ = getCurrentMicroseconds();

    if (shouldFlush()) {
        flush_pending_ = true;
    }

    return Status::OK;
}
UsbBufferManager::Status UsbBufferManager::sendDataImmediate(const void* data, size_t length) {
    if (!data || length == 0 || length > max_packet_size_) {
        return Status::ERROR;
    }

    if (*usb_busy_) {
        return Status::USB_BUSY;
    }

    sendPacket(static_cast<const uint8_t*>(data), length);
    return Status::OK;
}

bool UsbBufferManager::shouldFlush() const {
    uint32_t now = getCurrentMicroseconds();

    bool timeout_expired = (now - last_message_time_us_) >= flush_timeout_us_;
    bool queue_almost_full = getFreeSpace() < FORCE_FLUSH_THRESHOLD;
    bool buffer_not_empty = getBufferedCount() > 0;

    return buffer_not_empty && (timeout_expired || queue_almost_full);
}

size_t UsbBufferManager::preparePacketFromQueue(uint8_t* buffer, size_t max_len) {
    size_t total_len = 0;
    RawData packet;

    while (total_len < max_len && q_peek(&queue_, &packet)) {
        if (total_len + packet.length <= max_len) {
            q_drop(&queue_);  // Удаляем только если помещается
            memcpy(buffer + total_len, packet.data, packet.length);
            total_len += packet.length;
        } else {
            break;
        }
    }

    return total_len;
}

void UsbBufferManager::sendPacket(const uint8_t* data, size_t length) {
    if (CDC_Transmit_FS(const_cast<uint8_t*>(data), length) == USBD_OK) {
        if (usb_busy_) {
            *usb_busy_ = true;
        }
        total_sent_ += length;
    }
}

void UsbBufferManager::flush(bool force) {
    if (!force && !flush_pending_) {
        return;
    }

    if (*usb_busy_) {
        return;
    }

    uint8_t packet_buffer[MAX_PACKET_SIZE];
    size_t packet_len = preparePacketFromQueue(packet_buffer, max_packet_size_);

    if (packet_len > 0) {
        sendPacket(packet_buffer, packet_len);
        total_flushes_++;
    }

    flush_pending_ = false;
    last_flush_time_us_ = getCurrentMicroseconds();
}

void UsbBufferManager::update() {
    if (shouldFlush()) {
        flush_pending_ = true;
    }
    flush(false);
}

size_t UsbBufferManager::formatString(char* buffer, size_t max_len,
                                     const char* format, ...) {
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, max_len, format, args);
    va_end(args);

    return (len > 0 && len < static_cast<int>(max_len)) ? len : 0;
}

