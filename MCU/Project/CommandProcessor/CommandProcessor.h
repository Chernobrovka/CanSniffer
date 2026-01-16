/*
 * CommandProcessor.h
 *
 *  Created on: Dec 30, 2025
 *      Author: Dmitry
 */

#ifndef COMMANDPROCESSOR_COMMANDPROCESSOR_H_
#define COMMANDPROCESSOR_COMMANDPROCESSOR_H_

#include "CommandHandler/CommandHandler.h"
#include "LED/LED.h"
extern "C" {
	#include "Queue/cQueue.h"
	#include "COBSLib/cobs.h"
}

class CommandProcessor {
public:

	typedef void (*CanStartCallback)(void);
	typedef void (*CanStopCallback)(void);
	typedef void (*CanInfoCallback)(void);
	typedef void (*FilterAddCallback)(uint32_t id, uint32_t mask, FilterType type);
	typedef void (*FilterDelCallback)(uint32_t id, bool delete_all);
	typedef void (*FilterListCallback)(void);
	typedef void (*WriteCallback)(uint32_t id, uint8_t* data, uint8_t dlc);
	typedef void (*WriteSeqCallback)(uint32_t id,uint8_t* data, uint8_t dlc,
	                                 uint32_t count, uint32_t interval_ms);
	typedef void (*ReadRawCallback)(void);
	typedef void (*ReadParsedCallback)(void);
	typedef void (*ErrorCallback)(const char* error_msg);
	typedef void (*HandleBusLoadMonitorCallback)(bool enable);
	typedef void (*HandleBusLoadStatusCallback)(void);

    static constexpr uint16_t COMMAND_SIZE = 30;

	CommandProcessor(Queue_t *queue_ptr,
			CanStartCallback can_start_cb,
			CanStopCallback can_stop_cb,
			CanInfoCallback can_info_cb,
			FilterAddCallback filter_add_cb,
			FilterDelCallback filter_del_cb,
			FilterListCallback filter_list_cb,
			WriteCallback write_cb,
			WriteSeqCallback write_seq_cb,
			ReadRawCallback read_raw_cb,
			ReadParsedCallback read_parsed_cb,
			ErrorCallback error_cb,
			HandleBusLoadMonitorCallback handle_bus_load_monitor_cb,
			HandleBusLoadStatusCallback  handle_bus_load_status_cb
			);

    ~CommandProcessor() = default;

    void processCommand();

private:
    void processSingleCommand(const Command& cmd);
    bool getCommand(Command& cmd);

    Queue_t *command_queue_;

	CanStartCallback can_start_callback_;
	CanStopCallback can_stop_callback_;
	CanInfoCallback can_info_callback_;
	FilterAddCallback filter_add_callback_;
	FilterDelCallback filter_del_callback_;
	FilterListCallback filter_list_callback_;
	WriteCallback write_callback_;
	WriteSeqCallback write_seq_callback_;
	ReadRawCallback read_raw_callback_;
	ReadParsedCallback read_parsed_callback_;
	ErrorCallback error_callback_;
	HandleBusLoadMonitorCallback handle_bus_load_monitor_callback_;
	HandleBusLoadStatusCallback  handle_bus_load_status_callback_;
};


#endif /* COMMANDPROCESSOR_COMMANDPROCESSOR_H_ */
