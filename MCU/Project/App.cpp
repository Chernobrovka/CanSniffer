/*
 * App.c
 *
 *  Created on: Dec 22, 2025
 *      Author: Dmitry
 */
#include "App.hpp"
#include "LogPrint/LogPrint.h"
#include "usbd_cdc_if.h"
#include "gpio.h"
#include "tim.h"
#include "main.h"
#include <cstdarg>

static void usbSendCallback(CanMessage_t& msg, uint32_t data_size);
static void canStartCallback(void);
static void canStopCallback(void);
static void canInfoCallback(void);
static void filterAddCallback(uint32_t id, uint32_t mask, FilterType type);
static void filterDeleteCallback(uint32_t id, bool delete_all);
static void filterListCallback(void);
static void writeCallback(uint32_t id, uint8_t* data, uint8_t dlc);
static void writeSequenceCallback(uint32_t id, uint8_t* data, uint8_t dlc,
        uint32_t count, uint32_t interval_ms);
static void readRawCallback(void);
static void readParsedCallback(void);
static void errorCallback(const char *error_msg);
static void handleBusLoadMonitor(bool enable);
static void handleBusLoadStatus(void);
static void canSendCallback(uint32_t id, bool is_extended, bool is_remote, uint8_t* data, uint8_t dlc);
static bool configureFilterCallback(uint8_t bank, uint8_t slot,
                                   uint32_t id, uint32_t mask,
                                   bool is_extended);
static bool disableFilterCallback(uint8_t bank, uint8_t slot);
static void disableAllFiltersCallback();
static void usbPrint(const char* format, ...);

static void debugPrintInternal(const char* format, ...);

System *sys = nullptr;


void appInit(void){
	sys = new System();
}

void appLoop(void){
	sys->loop();
}

System::System(){
	timersInit();

	protocol_formatter = new ProtocolFormatter(ProtocolFormatter::Format::Raw);

	bus_monitor = new CanBusMonitor();

	can_processor = new CanProcessor(usbSendCallback, &can_msg_queue, bus_monitor, led);

	command_handler = new CommandHandler(&command_queue);

	command_processor = new CommandProcessor(&command_queue,
											canStartCallback,
											canStopCallback,
											canInfoCallback,
											filterAddCallback,
											filterDeleteCallback,
											filterListCallback,
											writeCallback,
											writeSequenceCallback,
											readRawCallback,
											readParsedCallback,
											errorCallback,
											handleBusLoadMonitor,
											handleBusLoadStatus);

	seq_manager = new SequenceManager(canSendCallback);

	filter_manager = new FilterManager(usbPrint,
										configureFilterCallback,
										disableFilterCallback,
										disableAllFiltersCallback);

	led = new Led(LED_CAN_GPIO_Port, LED_CAN_Pin, LED_USB_GPIO_Port, LED_USB_Pin);

	usb_buffer_manager = new UsbBufferManager(&this->usb_busy);
    usb_buffer_manager->setFlushTimeout(500);
    usb_buffer_manager->setMaxPacketSize(64);


	CanDriver::Status can_status;

	can_driver = new CanDriver(&hcan1, &can_msg_queue);
	can_status = can_driver->setFilterAcceptAll(0);
	if (can_status != CanDriver::Status::OK){
		debugPrintInternal("CAN filter error!\n");
	}

	can_status = can_driver->start();
	if (can_status != CanDriver::Status::OK){
		debugPrintInternal("CAN filter error!\n");
	}

	led->setSystemStatus(Led::Status::ON);
	led->flashOnCommand();
	HAL_Delay(200);
	led->setSystemStatus(Led::Status::OFF);
}

void System::loop(){
	uint32_t current_time = HAL_GetTick();

	can_processor->processMessage();

	if (bus_monitor && state.timer_100ms_ready){
		bus_monitor->update(current_time);
		state.timer_100ms_ready = false;
	}

	seq_manager->update(current_time);

	led->update(current_time);

	command_processor->processCommand();

	usb_buffer_manager->update();
}

void System::timersInit(){
	// 100 ms timer
	if (HAL_TIM_Base_Start_IT(&htim14) != HAL_OK){
		debugPrintInternal("Timer14 dont started.\n\r");
	 }
	 state.timer_100ms_ready = false;
}

void usbSendCallback(CanMessage_t& msg, uint32_t data_size){
    char buffer[48];
    char* ptr = buffer;

    uint32_t seconds = msg.timestamp_ms / 1000; // === 1. ВРЕМЯ: 3 цифры + '.' + 3 цифры ===
    uint32_t milliseconds = msg.timestamp_ms % 1000;

    *ptr++ = '0' + (seconds / 100); // Секунды: всегда 3 цифры
    *ptr++ = '0' + ((seconds / 10) % 10);
    *ptr++ = '0' + (seconds % 10);
    *ptr++ = '.';

    *ptr++ = '0' + (milliseconds / 100);     // Миллисекунды: всегда 3 цифры
    *ptr++ = '0' + ((milliseconds / 10) % 10);
    *ptr++ = '0' + (milliseconds % 10);
    *ptr++ = ' ';


    if (msg.header.RTR == CAN_RTR_REMOTE) { // === 2. ТИП ФРЕЙМА (R для remote) ===
        *ptr++ = 'R';
        *ptr++ = ' ';
    }

    if (msg.header.IDE == CAN_ID_STD) {
        uint32_t id = msg.header.StdId;
        *ptr++ = hexdigit_upper[(id >> 8) & 0xF];
        *ptr++ = hexdigit_upper[(id >> 4) & 0xF];
        *ptr++ = hexdigit_upper[id & 0xF];
    } else {
        uint32_t id = msg.header.ExtId;
        *ptr++ = hexdigit_upper[(id >> 28) & 0xF];
        *ptr++ = hexdigit_upper[(id >> 24) & 0xF];
        *ptr++ = hexdigit_upper[(id >> 20) & 0xF];
        *ptr++ = hexdigit_upper[(id >> 16) & 0xF];
        *ptr++ = hexdigit_upper[(id >> 12) & 0xF];
        *ptr++ = hexdigit_upper[(id >> 8) & 0xF];
        *ptr++ = hexdigit_upper[(id >> 4) & 0xF];
        *ptr++ = hexdigit_upper[id & 0xF];
    }
    *ptr++ = ' ';


    *ptr++ = '0' + msg.header.DLC; // === 4. DLC ===
    *ptr++ = ' ';

    // === 5. ДАННЫЕ ===
    if (msg.header.RTR == CAN_RTR_DATA) {
        for (uint8_t i = 0; i < msg.header.DLC; i++) {
            *ptr++ = hexdigit_upper[(msg.data[i] >> 4) & 0xF];
            *ptr++ = hexdigit_upper[msg.data[i] & 0xF];
        }
    }

    *ptr++ = '\r'; // === 6. ЗАВЕРШЕНИЕ ===
    *ptr++ = '\n';

    uint16_t len = ptr - buffer;
    if (len > 0 && len <= sizeof(buffer)) {
        sys->usb_buffer_manager->sendData(buffer, len);
    }
}

static void canStartCallback(void){
	if (sys->can_driver->activateNotification() != CanDriver::Status::OK){
		sys->led->indicateError(true);
		usbPrint("ERROR: Failed to start CAN interface\r\n");
		return ;
	}
	sys->snifferAtivityStatus = System::SNIFFER_ACTIVE;
	sys->led->indicateCanStarted(true);
    usbPrint("OK: CAN interface started successfully\r\n");
}

static void canStopCallback(void){
	if (sys->can_driver->deactivateNotification() != CanDriver::Status::OK) {
		sys->led->indicateError(true);
        usbPrint("ERROR: Failed to stop CAN interface\r\n");
        return;
	}
	sys->snifferAtivityStatus = System::SNIFFER_STOPPED;
	sys->led->indicateCanStarted(false);
    usbPrint("OK: CAN interface stopped\r\n");
}

static void canInfoCallback(void){
	sys->led->flashOnCommand();
	char buffer[2048];
	int len = 0;

    memset(buffer, 0, sizeof(buffer));

    // ====== ЗАГОЛОВОК ======
    len += snprintf(buffer + len, sizeof(buffer) - len,
                   "\r\n"
                   "========================================\r\n"
                   "          CAN SNIFFER SYSTEM INFO       \r\n"
                   "========================================\r\n\r\n");

    // ====== ДОСТУПНЫЕ КОМАНДЫ ======
    len += snprintf(buffer + len, sizeof(buffer) - len,
                   "AVAILABLE COMMANDS:\r\n"
                   "  can start       - Start CAN interface\r\n"
                   "  can stop        - Stop CAN interface\r\n"
                   "  can info        - This information\r\n"
                   "  filter add <id> [mask] [type] - Add filter\r\n"
                   "  filter del <id|all> - Delete filter\r\n"
                   "  filter list     - List active filters\r\n"
                   "  write <id> <data> - Send CAN message\r\n"
                   "  write seq <id> <data> <count> <interval_ms>\r\n"
                   "  read raw        - Raw message monitoring\r\n"
				   "  read parsed     - Parsed message monitoring\r\n"
				   "  bus load on     - Start bus load monitoring\r\n"
				   "  bus load off    - Stop bus load monitoring\r\n"
				   "  bus load status - Show current bus load\r\n\r\n");

    // ====== ФОРМАТ ДАННЫХ ======
    len += snprintf(buffer + len, sizeof(buffer) - len,
                   "DATA FORMATS:\r\n"
                   "  ID:            decimal or 0xhex (0x100)\r\n"
                   "  Data:          hex bytes (01 A2 FF or 01A2FF)\r\n"
                   "  Filter type:   std (11-bit) or ext (29-bit)\r\n"
                   "  Mask:          hex value (0x7F0 for range)\r\n\r\n");

    // ====== ПРИМЕРЫ ======
    len += snprintf(buffer + len, sizeof(buffer) - len,
                   "EXAMPLES:\r\n"
                   "  can start\r\n"
                   "  filter add 0x100 0x7F0 std\r\n"
                   "  write 0x123 01 02 03 04\r\n"
                   "  write seq 0x200 AABB 10 100\r\n"
                   "  read raw\r\n\r\n");

    // ====== СИСТЕМНАЯ ИНФОРМАЦИЯ ======
    len += snprintf(buffer + len, sizeof(buffer) - len,
                   "SYSTEM INFO:\r\n"
                   "  MCU:           STM32F407VET6\r\n"
                   "  Clock:         64 MHz\r\n"
                   "  Memory:        128 kB RAM, 512 kB Flash\r\n"
                   "  Version:       1.2.0\r\n"
                   "  Build date:    12.02.2026\r\n");

    len += snprintf(buffer + len, sizeof(buffer) - len,
                   "========================================\r\n\r\n");

    uint32_t timeout = HAL_GetTick() + 5000;
    while (sys->usb_busy && (HAL_GetTick() < timeout)) {
        HAL_Delay(1);
    }

    sys->usb_busy = true;
	CDC_Transmit_FS((uint8_t*)buffer, len);
}

static void filterAddCallback(uint32_t id, uint32_t mask, FilterType type) {
	bool success = sys->filter_manager->addFilter(id, mask, (FilterManager::FilterType)type);

    if (!success) {
    	sys->led->indicateError(true);
        debugPrintInternal("ERROR: Failed to add filter\r\n");
        debugPrintInternal("Available filters: %zu/%zu\r\n",
        		sys->filter_manager->getFreeFilterCount(),
				sys->filter_manager->getTotalFilterCount());
    }
    else sys->led->flashOnCommand();
}

static void filterDeleteCallback(uint32_t id, bool delete_all){
	sys->led->flashOnCommand();

	if (delete_all) {
    	sys->filter_manager->removeAllFilters();
    	debugPrintInternal("All filters removed\r\n");
    } else {
        if (sys->filter_manager->removeFilter(id)) {
        	debugPrintInternal("Filter 0x%08lX removed\r\n", id);
        } else {
        	debugPrintInternal("ERROR: Filter 0x%08lX not found\r\n", id);
        }
    }
}

static void filterListCallback(void){
	sys->led->flashOnCommand();
	sys->filter_manager->printFilterList();
}

static void writeCallback(uint32_t id, uint8_t* data, uint8_t dlc){
	bool is_extended = (id > 0x7FF);
	bool is_remote = false;

	sys->led->flashOnTx();
	sys->can_driver->sendMessage(id, is_extended, is_remote, data, dlc);
}

static void writeSequenceCallback(uint32_t id, uint8_t* data, uint8_t dlc,
        uint32_t count, uint32_t interval_ms){
	sys->led->flashOnCommand();
	bool success = sys->seq_manager->startSequence(id, data, dlc, count, interval_ms);

    if (success) {
        usbPrint("Sequence started successfully. Active: %zu\r\n",
               sys->seq_manager->getActiveCount());
    } else {
    	usbPrint("ERROR: Failed to start sequence (max %zu)\r\n",
               SequenceManager::MAX_SEQUENCES);
    }
}

static void readRawCallback(void){
	sys->led->flashOnCommand();
	sys->state.parsing = false;
}

static void readParsedCallback(void){
	sys->led->flashOnCommand();
	sys->state.parsing = true;
}

static void errorCallback(const char *error_msg){
    if (sys && sys->led) {
        sys->led->indicateError(true);
    }

    if (error_msg) {
        debugPrintInternal("ERROR: %s\r\n", error_msg);
    }
}

static void handleBusLoadMonitor(bool enable) {
    if (enable) {
    	sys->led->flashOnCommand();
        sys->bus_monitor->startMonitoring();
    } else {
    	sys->bus_monitor->stopMonitoring();
    }
}

static void handleBusLoadStatus(void) {
	sys->led->flashOnCommand();
    float load = sys->bus_monitor->getCurrentLoad();
    usbPrint("Current CAN bus load: %.1f%%\r\n", load);
}

static void canSendCallback(uint32_t id, bool is_extended, bool is_remote, uint8_t* data, uint8_t dlc){
	sys->led->flashOnTx();
	sys->can_driver->sendMessage(id, is_extended, is_remote, data, dlc);
}

static bool configureFilterCallback(uint8_t bank, uint8_t slot,
                                   uint32_t id, uint32_t mask,
                                   bool is_extended) {
    CanDriver::Status status = sys->can_driver->setFilter(bank, slot, id, mask, is_extended);

    if (status != CanDriver::Status::OK) {
        debugPrint("ERROR: Failed to configure filter bank %d, slot %d\r\n", bank, slot);
        debugPrint("ID: 0x%08lX, Mask: 0x%08lX, Type: %s\r\n",
               id, mask, is_extended ? "EXT" : "STD");
        return false;
    }

    debugPrint("Filter configured: Bank %d, Slot %d, ID 0x%08lX\r\n", bank, slot, id);

    return true;
}

static bool disableFilterCallback(uint8_t bank, uint8_t slot){
    CanDriver::Status status = sys->can_driver->disableFilter(bank, slot);

    if (status != CanDriver::Status::OK) {
    	debugPrint("WARNING: Failed to disable filter bank %d, slot %d\r\n", bank, slot);
    	return false;
    }

    debugPrint("Filter disabled: Bank %d, Slot %d\r\n", bank, slot);
    return true;
}

static void disableAllFiltersCallback(){
    CanDriver::Status status = sys->can_driver->disableAllFilters();

    if (status == CanDriver::Status::OK) {
    	debugPrint("All filters disabled successfully\r\n");
    } else {
    	debugPrint("WARNING: Some filters failed to disable\r\n");
    }
}

static void usbPrint(const char* format, ...){
    static char buffer[256];
    va_list args;
    uint32_t timeout;

    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    timeout = HAL_GetTick() + 50000;
    while (sys->usb_busy && (HAL_GetTick() < timeout)) {
        HAL_Delay(1);
    }

    if (sys->usb_busy) {
        return;
    }

    if (len > 0 && len < (int)sizeof(buffer)) {
        //sys->usb_busy = true; // Устанавливаем флаг занятости
        CDC_Transmit_FS((uint8_t*)buffer, (uint16_t)len);
        //sys->usb_buffer_manager->sendData((uint8_t*)buffer, (uint16_t)len);
    }
}

static void debugPrintInternal(const char* format, ...){
	if (sys->state.debug_method == System::DebugMethod::USB_DEBUG){
		usbPrint(format);
	}
	else if (sys->state.debug_method == System::DebugMethod::UART_DEBUG){
		debugPrint(format);
	}
	else if (sys->state.debug_method == System::DebugMethod::SWV_DEBUG){
		printf(format);
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
	if (htim == &htim14){
		sys->state.timer_100ms_ready = true;
	}
}
