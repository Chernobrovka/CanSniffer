/*
 * CommandHandler.h
 *
 *  Created on: Dec 30, 2025
 *      Author: peleng
 */

#ifndef COMMANDHANDLER_COMMANDHANDLER_H_
#define COMMANDHANDLER_COMMANDHANDLER_H_

#include "main.h"

extern "C" {
	#include "Queue/cQueue.h"

	#include "gpio.h"
}

typedef enum {
    CMD_UNKNOWN = 0,

    // CAN команды
    CMD_CAN_START,
    CMD_CAN_STOP,
    CMD_CAN_INFO,

    // Фильтры
    CMD_FILTER_ADD,
    CMD_FILTER_DEL,
    CMD_FILTER_LIST,

    // Запись
    CMD_WRITE,
    CMD_WRITE_SEQ,

    // Чтение
    CMD_READ_RAW,
    CMD_READ_PARSED,

    CMD_BUS_LOAD_ON,
    CMD_BUS_LOAD_OFF,
    CMD_BUS_LOAD_STATUS
} CommandType;

typedef enum {
    FILTER_TYPE_STD = 0,
    FILTER_TYPE_EXT
} FilterType;

// Структура команды
typedef struct {
    CommandType type;

    union {
        // Для фильтров
        struct {
            uint32_t id;
            uint32_t mask;
            FilterType filter_type;
            bool delete_all;
        } filter;

        // Для записи
        struct {
            uint32_t id;
            uint8_t data[8];
            uint8_t dlc;
            uint32_t count;
            uint32_t interval_ms;
        } write;
    } params;
} Command;

class CommandHandler {
public:

    static constexpr uint16_t BUFFER_SIZE = 256;
    static constexpr uint16_t COMMAND_SIZE = 30;
    static constexpr uint16_t QUEUE_SIZE = 100;

    static constexpr uint8_t TOKEN_SIZE = 32;
    static constexpr uint8_t MAX_TOKENS = 10;

/*    struct Command {
        uint8_t data[COMMAND_SIZE];
        uint8_t size;
    };*/

    enum class Result {
        OK,
        BufferOverflow,
        QueueFull,
        InvalidCommand,
		Incomplete,
		ParseError,
    };

    CommandHandler(Queue_t *queue_ptr);
    ~CommandHandler() = default;

    Result processByte(uint8_t byte);
    Result processBuffer(const uint8_t* buffer, uint16_t length);

    uint16_t getQueueCount() const;
    uint16_t getProcessedCount() const { return processed_count_; }
    uint16_t getErrorCount() const { return error_count_; }

private:

    void addByteToBuffer(uint8_t byte);
    Result completeCommand();

    Result parseCommand(const char* command_str, Command* cmd);
    int tokenize(const char* input, char tokens[][TOKEN_SIZE]);
    uint32_t parseHex(const char* str);
    bool parseDataBytes(const char* str, uint8_t* data, uint8_t* dlc);
    Result parseFilterAdd(char tokens[][TOKEN_SIZE], int token_count, Command* cmd);
    Result parseWrite(char tokens[][TOKEN_SIZE], int token_count, Command* cmd);
    Result parseWriteSeq(char tokens[][TOKEN_SIZE], int token_count, Command* cmd);
    bool isDelimiter(char c);
    void trimWhitespace(char* str);

    void clearBuffer();

    uint8_t rx_buffer_[BUFFER_SIZE];
    volatile uint16_t cursor_;

    Queue_t *command_queue_;

    uint16_t processed_count_;
    uint16_t error_count_;

    bool initialized_;
};


#endif /* COMMANDHANDLER_COMMANDHANDLER_H_ */
