/*
 * CanProcessor.h
 *
 *  Created on: 23 дек. 2025 г.
 *      Author: peleng
 */

#ifndef CANPROCESSOR_CANPROCESSOR_H_
#define CANPROCESSOR_CANPROCESSOR_H_

#include "can.h"
#include "CanProcessor/CanProcessor.h"
#include "Queue/cQueue.h"
#include "CanBusMonitor/CanBusMonitor.h"
#include "LED/LED.h"

#define CAN_MSSG_QUEUE_SIZE 100

typedef struct {
    uint32_t timestamp_ms;
    CAN_RxHeaderTypeDef header;
    uint8_t data[8];
} CanMessage_t;

typedef void (*usbOutputCallback)(CanMessage_t& msg, uint32_t size);

class CanProcessor {
public:
	typedef enum {
	    Ok,
	    Error,
	    Busy,
	    Timeout,
	    InvalidParam,
	    BufferFull,
	    BufferEmpty
	} Status;


    enum class State {
        Idle,
        Running,
        Paused
    };

    CanProcessor(usbOutputCallback usb_cb, Queue_t *queue, CanBusMonitor *monitor, Led *led_ptr);
    ~CanProcessor();


    CanProcessor::Status processMessage();

    State getState() const { return (CanProcessor::State)state_; }
    uint32_t getErrorCount() const { return error_count_; }

private:
    State state_;
    uint32_t processed_count_;
    uint32_t error_count_;
    Queue_t *queue_;
    usbOutputCallback usb_callback_;
    CanBusMonitor *bus_monitor_;
    Led *led_;

    CanProcessor::Status validateMessage(const CanMessage_t& msg);
};

#endif /* CANPROCESSOR_CANPROCESSOR_H_ */
