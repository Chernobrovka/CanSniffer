/*
 * App.h
 *
 *  Created on: Dec 22, 2025
 *      Author: Dmitry
 */

#ifndef APP_HPP_
#define APP_HPP_

#include "CAN/CanDriver.h"
#include "CanProcessor/CanProcessor.h"
#include "ProtocolFormatter/ProtocolFormatter.h"
#include "CommandHandler/CommandHandler.h"
#include "CommandProcessor/CommandProcessor.h"
#include "SequenceManager/SequenceManager.h"
#include "FilterManager/FilterManager.h"
#include "CanBusMonitor/CanBusMonitor.h"
#include "LED/LED.h"

extern "C" {
	#include "Queue/cQueue.h"
	#include "COBSLib/cobs.h"
	#include "usart.h"
	#include <stdint.h>
	#include <stdbool.h>
}

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"

#define LOG_UART_PTR &huart1

void appInit(void);
void appLoop(void);

class System {
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

	typedef enum {
		SNIFFER_STOPPED = 0x00,
		SNIFFER_ACTIVE  = 0x01,
	} SnifferAtivityStatus;

	typedef enum {
		NO_DEBUG,
		USB_DEBUG,
		UART_DEBUG,
		SWV_DEBUG,
	} DebugMethod;

	typedef struct {
		bool parsing;
		DebugMethod debug_method;
		bool timer_100ms_ready;
	} State;

	System();
	~System();
	void loop();

	void substr(char *str, char *sub, int start, int len);
	int  toInteger(uint8_t *stringToConvert, int len);

	System::State state = {0};

	CanDriver      *can_driver      = nullptr;
	CommandHandler *command_handler = nullptr;
	ProtocolFormatter *protocol_formatter = nullptr;
	SequenceManager *seq_manager 		  = nullptr;
	FilterManager   *filter_manager = nullptr;
	CanBusMonitor   *bus_monitor = nullptr;
	Led             *led         = nullptr;

	SnifferAtivityStatus snifferAtivityStatus = SNIFFER_STOPPED;
private:
	void timersInit();

	CommandProcessor *command_processor   = nullptr;
	CanProcessor     *can_processor       = nullptr;

	Queue_t command_queue;
	Queue_t can_msg_queue;

	CanMessage_t dequed_can_message;

};

extern System *sys;

#endif /* APP_HPP_ */
