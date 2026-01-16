/*
 * CanProcessor.cpp
 *
 *  Created on: 23 дек. 2025 г.
 *      Author: peleng
 */
#include "CanProcessor.h"

CanProcessor::CanProcessor(usbOutputCallback usb_cb, Queue_t *queue, CanBusMonitor *monitor, Led *led_ptr)
			: state_(State::Idle),
			  processed_count_(0),
			  error_count_(0),
			  queue_ (queue),
			  usb_callback_ (usb_cb),
			  bus_monitor_(monitor),
			  led_(led_ptr){
	q_init(queue_, sizeof(CanMessage_t), CAN_MSSG_QUEUE_SIZE, FIFO, false);
	state_ = State::Running;
}

CanProcessor::Status CanProcessor::processMessage() {
	CanMessage_t dequed_can_message;
	if (q_pop(queue_, &dequed_can_message)){
		if (state_ != State::Running) {
	        return CanProcessor::Status::Error;
	    }

	    auto validation_status = validateMessage(dequed_can_message);
	    if (validation_status != CanProcessor::Status::Ok) {
	    	error_count_++;
	    	return validation_status;
	    }

	    led_->flashOnRx();

	    CanMessage_t msg_copy = dequed_can_message;

	    if (usb_callback_) {
	    	uint32_t data_size = dequed_can_message.header.DLC;

	    	usb_callback_(msg_copy, data_size);
	    }

		bool is_extended = (msg_copy.header.IDE == CAN_ID_EXT);
		bus_monitor_->onMessageReceived(is_extended, msg_copy.header.DLC,  (msg_copy.header.RTR == CAN_RTR_REMOTE));

	    processed_count_++;
	    return CanProcessor::Status::Ok;
	}

	return CanProcessor::Status::Ok;
}

CanProcessor::Status CanProcessor::validateMessage(const CanMessage_t& msg) {
    if (msg.header.DLC > 8) {
        return CanProcessor::Status::InvalidParam;
    }

    if (msg.header.IDE == CAN_ID_EXT && msg.header.ExtId > 0x1FFFFFFF) {
        return CanProcessor::Status::InvalidParam;
    }

    if (msg.header.IDE == CAN_ID_STD && msg.header.StdId > 0x7FF) {
        return CanProcessor::Status::InvalidParam;
    }

    return CanProcessor::Status::Ok;
}
