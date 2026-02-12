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
#include "UsbBufferManager/UsbBufferManager.h"

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

	System::State state = {0};

	CanDriver      *can_driver      = nullptr;
	CommandHandler *command_handler = nullptr;
	ProtocolFormatter *protocol_formatter = nullptr;
	SequenceManager *seq_manager 		  = nullptr;
	FilterManager   *filter_manager = nullptr;
	CanBusMonitor   *bus_monitor = nullptr;
	Led             *led         = nullptr;
	UsbBufferManager *usb_buffer_manager = nullptr;

	SnifferAtivityStatus snifferAtivityStatus = SNIFFER_STOPPED;

	bool usb_busy = false;
private:
	void timersInit();

	CommandProcessor *command_processor   = nullptr;
	CanProcessor     *can_processor       = nullptr;

	Queue_t command_queue;
	Queue_t can_msg_queue;

	CanMessage_t dequed_can_message;

};

extern System *sys;

static const char hexdigit_lower[] = "0123456789abcdef";
static const char hexdigit_upper[] = "0123456789ABCDEF";
static const char digits_100_upper[100][2] = {
    '0','0', '0','1', '0','2', '0','3', '0','4', '0','5', '0','6', '0','7', '0','8', '0','9',
    '1','0', '1','1', '1','2', '1','3', '1','4', '1','5', '1','6', '1','7', '1','8', '1','9',
    '2','0', '2','1', '2','2', '2','3', '2','4', '2','5', '2','6', '2','7', '2','8', '2','9',
    '3','0', '3','1', '3','2', '3','3', '3','4', '3','5', '3','6', '3','7', '3','8', '3','9',
    '4','0', '4','1', '4','2', '4','3', '4','4', '4','5', '4','6', '4','7', '4','8', '4','9',
    '5','0', '5','1', '5','2', '5','3', '5','4', '5','5', '5','6', '5','7', '5','8', '5','9',
    '6','0', '6','1', '6','2', '6','3', '6','4', '6','5', '6','6', '6','7', '6','8', '6','9',
    '7','0', '7','1', '7','2', '7','3', '7','4', '7','5', '7','6', '7','7', '7','8', '7','9',
    '8','0', '8','1', '8','2', '8','3', '8','4', '8','5', '8','6', '8','7', '8','8', '8','9',
    '9','0', '9','1', '9','2', '9','3', '9','4', '9','5', '9','6', '9','7', '9','8', '9','9'
};


#endif /* APP_HPP_ */
