/*
 * CommandHandler.cpp
 *
 *  Created on: Dec 30, 2025
 *      Author: Dmitry
 */
#include "CommandHandler.h"
#include "COBSLib/cobs.h"
#include <cstring>
#include <cctype>
#include <cstdlib>

CommandHandler::CommandHandler(Queue_t *queue_ptr)
    : cursor_(0),
      processed_count_(0),
      error_count_(0),
      initialized_(false),
	  command_queue_(queue_ptr){

	q_init(command_queue_, sizeof(Command), QUEUE_SIZE, FIFO, true);

	clearBuffer();

	initialized_ = true;
}

CommandHandler::Result CommandHandler::processByte(uint8_t byte){
	if (!initialized_) return Result::InvalidCommand;

	if (byte == '\n' || byte == '\r'){
		if (cursor_ > 0) {
			if (cursor_ < BUFFER_SIZE){
				rx_buffer_[cursor_] = '\0';
			}
			return completeCommand();
		}
	}

	if (byte < 0x20 && byte != '\t'){
		return Result::OK;
	}

	addByteToBuffer(byte);

	return Result::OK;
}

CommandHandler::Result CommandHandler::processBuffer(const uint8_t* buffer, uint16_t length) {
	if (!initialized_ || buffer == nullptr || length == 0) {
        return Result::InvalidCommand;
    }

    Result result = Result::OK;

    for (uint16_t i = 0; i < length; i++) {
        Result byte_result = processByte(buffer[i]);
        if (byte_result != Result::OK) {
            result = byte_result;
        }
    }

    return result;
}

void CommandHandler::addByteToBuffer(uint8_t byte) {
    if (cursor_ >= BUFFER_SIZE) {
        cursor_ = 0;
        error_count_++;
        return;
    }

    rx_buffer_[cursor_] = byte;
    cursor_++;
}

CommandHandler::Result CommandHandler::completeCommand() {
    if (cursor_ == 0) return Result::InvalidCommand;

    if (cursor_ < BUFFER_SIZE)
    	rx_buffer_[cursor_] = '\0';
    else
    	rx_buffer_[BUFFER_SIZE - 1] = '\0';

    Command cmd;
    CommandHandler::Result parse_result = parseCommand((const char*)rx_buffer_, &cmd);

    if (parse_result != Result::OK) {
        cursor_ = 0;
        error_count_++;
        return parse_result;
    }

    if (q_isFull(command_queue_)) {
        cursor_ = 0;
        error_count_++;
        return Result::QueueFull;
    }

    if (!q_push(command_queue_, &cmd)) {
        cursor_ = 0;
        error_count_++;
        return Result::QueueFull;
    }

    HAL_GPIO_TogglePin(LED_USB_GPIO_Port, LED_USB_Pin);

    cursor_ = 0;
    processed_count_++;

    return Result::OK;
}

void CommandHandler::clearBuffer() {
    cursor_ = 0;
    memset(rx_buffer_, 0, BUFFER_SIZE);
}

CommandHandler::Result CommandHandler::parseCommand(const char* command_str, Command* cmd) {
    if (!cmd) return Result::ParseError;

    memset(cmd, 0, sizeof(Command));

    char tokens[MAX_TOKENS][TOKEN_SIZE] = {0};
    int token_count = tokenize(command_str, tokens);

    if (token_count == 0) {
        return Result::InvalidCommand;
    }

    // Определяем тип команды
    if (strcmp(tokens[0], "can") == 0) {
        if (token_count < 2) {
            return Result::InvalidCommand;
        }

        if (strcmp(tokens[1], "start") == 0) {
            cmd->type = CMD_CAN_START;
            return Result::OK;
        }
        else if (strcmp(tokens[1], "stop") == 0) {
            cmd->type = CMD_CAN_STOP;
            return Result::OK;
        }
        else if (strcmp(tokens[1], "info") == 0) {
            cmd->type = CMD_CAN_INFO;
            return Result::OK;
        }
    }
    else if (strcmp(tokens[0], "filter") == 0) {
        if (token_count < 2) {
            return Result::InvalidCommand;
        }

        if (strcmp(tokens[1], "add") == 0) {
            return parseFilterAdd(tokens, token_count, cmd);
        }
        else if (strcmp(tokens[1], "del") == 0) {
            cmd->type = CMD_FILTER_DEL;

            if (token_count < 3) {
                return Result::InvalidCommand;
            }

            if (strcmp(tokens[2], "all") == 0) {
                cmd->params.filter.delete_all = true;
            } else {
                cmd->params.filter.delete_all = false;
                cmd->params.filter.id = parseHex(tokens[2]);
            }
            return Result::OK;
        }
        else if (strcmp(tokens[1], "list") == 0) {
            cmd->type = CMD_FILTER_LIST;
            return Result::OK;
        }
    }
    else if (strcmp(tokens[0], "write") == 0) {
        if (token_count < 2) {
            return Result::InvalidCommand;
        }

        if (strcmp(tokens[1], "seq") == 0) {
            return parseWriteSeq(tokens, token_count, cmd);
        }
        else {
            return parseWrite(tokens, token_count, cmd);
        }
    }
    else if (strcmp(tokens[0], "read") == 0) {
        if (token_count < 2) {
            return Result::InvalidCommand;
        }

        if (strcmp(tokens[1], "raw") == 0) {
            cmd->type = CMD_READ_RAW;
            return Result::OK;
        }
        else if (strcmp(tokens[1], "parsed") == 0) {
            cmd->type = CMD_READ_PARSED;
            return Result::OK;
        }
    }
    else if (strcmp(tokens[0], "bus") == 0) {
        if (token_count < 2) {
            return Result::InvalidCommand;
        }

        if (strcmp(tokens[1], "load") == 0) {
            if (token_count < 3) {
                return Result::InvalidCommand;
            }

            if (strcmp(tokens[2], "on") == 0) {
                cmd->type = CMD_BUS_LOAD_ON;
                return Result::OK;
            }
            else if (strcmp(tokens[2], "off") == 0) {
                cmd->type = CMD_BUS_LOAD_OFF;
                return Result::OK;
            }
            else if (strcmp(tokens[2], "status") == 0) {
                cmd->type = CMD_BUS_LOAD_STATUS;
                return Result::OK;
            }
        }
    }

    return Result::InvalidCommand;
}

int CommandHandler::tokenize(const char* input, char tokens[][TOKEN_SIZE]) {
    int count = 0;
    int pos = 0;
    int len = strlen(input);

    while (pos < len && count < MAX_TOKENS) {
        // Пропускаем пробелы
        while (pos < len && isspace(input[pos])) pos++;
        if (pos >= len) break;

        int token_pos = 0;

        // Читаем токен до следующего пробела или конца строки
        while (pos < len && !isspace(input[pos]) && token_pos < TOKEN_SIZE - 1) {
            tokens[count][token_pos++] = input[pos++];
        }
        tokens[count][token_pos] = '\0';
        count++;
    }

    return count;
}

uint32_t CommandHandler::parseHex(const char* str) {
    uint32_t value = 0;

    // Пропускаем "0x" если есть
    if (strlen(str) > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str += 2;
    }

    while (*str) {
        char c = *str++;
        value <<= 4;

        if (c >= '0' && c <= '9') {
            value |= (c - '0');
        } else if (c >= 'a' && c <= 'f') {
            value |= (c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            value |= (c - 'A' + 10);
        } else {
            return 0;
        }
    }

    return value;
}

// Парсинг байтов данных
bool CommandHandler::parseDataBytes(const char* str, uint8_t* data, uint8_t* dlc) {
    int byte_count = 0;
    char hex[3] = {0};
    int hex_pos = 0;
    bool last_was_delimiter = false;

    while (*str && byte_count < 8) {
        char c = *str++;

        // Если это разделитель
        if (c == ' ' || c == '-' || c == ':') {
            if (hex_pos == 2) {
                data[byte_count++] = (uint8_t)parseHex(hex);
                hex_pos = 0;
            }
            last_was_delimiter = true;
            continue;
        }

        last_was_delimiter = false;

        // Проверяем hex символ
        if (!isxdigit(c)) {
            return false;
        }

        hex[hex_pos++] = c;

        if (hex_pos == 2) {
            hex[2] = '\0';
            data[byte_count++] = (uint8_t)parseHex(hex);
            hex_pos = 0;
        }
    }

    // Последний неполный байт
    if (hex_pos == 1) {
        hex[1] = '0';
        hex[2] = '\0';
        data[byte_count++] = (uint8_t)parseHex(hex);
    } else if (hex_pos == 2) {
        hex[2] = '\0';
        data[byte_count++] = (uint8_t)parseHex(hex);
    }

    *dlc = byte_count;
    return true;
}

// Парсинг команды filter add
CommandHandler::Result CommandHandler::parseFilterAdd(char tokens[][TOKEN_SIZE], int token_count, Command* cmd) {
    cmd->type = CMD_FILTER_ADD;

    if (token_count < 3) {
        return Result::InvalidCommand;
    }

    // Парсим ID
    cmd->params.filter.id = parseHex(tokens[2]);

    // Устанавливаем значения по умолчанию
    cmd->params.filter.mask = 0x7FF;  // По умолчанию для STD
    cmd->params.filter.filter_type = FILTER_TYPE_STD;
    cmd->params.filter.delete_all = false;

    if (token_count >= 4) {
        cmd->params.filter.mask = parseHex(tokens[3]);
    }

    // Обрабатываем тип если есть
    if (token_count >= 5) {
        if (strcmp(tokens[4], "ext") == 0 || strcmp(tokens[4], "EXT") == 0) {
            cmd->params.filter.filter_type = FILTER_TYPE_EXT;

            // Если маска еще дефолтная - обновляем для EXT
            if (token_count == 4 || cmd->params.filter.mask == 0x7FF) {
                cmd->params.filter.mask = 0x1FFFFFFF;
            }
        }
        else if (strcmp(tokens[4], "std") == 0 || strcmp(tokens[4], "STD") == 0) {
            // Явно указан std - оставляем как есть
            cmd->params.filter.filter_type = FILTER_TYPE_STD;

            // Если маска дефолтная для ext - меняем на std дефолт
            if (cmd->params.filter.mask == 0x1FFFFFFF) {
                cmd->params.filter.mask = 0x7FF;
            }
        }
    }

    return Result::OK;
}

// Парсинг команды write
CommandHandler::Result CommandHandler::parseWrite(char tokens[][TOKEN_SIZE], int token_count, Command* cmd) {
    cmd->type = CMD_WRITE;

    if (token_count < 3) {
        return Result::InvalidCommand;
    }

    // Парсим ID (первый токен после "write")
    cmd->params.write.id = parseHex(tokens[1]);

    // Объединяем оставшиеся токены для данных
    char data_str[64] = {0};
    for (int i = 2; i < token_count; i++) {
        if (i > 2) strcat(data_str, " ");
        strcat(data_str, tokens[i]);
    }

    if (!parseDataBytes(data_str, cmd->params.write.data, &cmd->params.write.dlc)) {
        return Result::ParseError;
    }

    cmd->params.write.count = 1;
    cmd->params.write.interval_ms = 0;

    return Result::OK;
}

// Парсинг команды write seq
CommandHandler::Result CommandHandler::parseWriteSeq(char tokens[][TOKEN_SIZE], int token_count, Command* cmd) {
    cmd->type = CMD_WRITE_SEQ;

    if (token_count < 6) {
        return Result::InvalidCommand;
    }

    // write seq <id> <data> <count> <interval_ms>
    cmd->params.write.id = parseHex(tokens[2]);

    // Данные
    if (!parseDataBytes(tokens[3], cmd->params.write.data, &cmd->params.write.dlc)) {
        return Result::ParseError;
    }

    // Count и interval
    cmd->params.write.count = atoi(tokens[4]);
    cmd->params.write.interval_ms = atoi(tokens[5]);

    if (cmd->params.write.count > 10000) {
        return Result::ParseError;
    }

    return Result::OK;
}

// Утилиты
bool CommandHandler::isDelimiter(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

void CommandHandler::trimWhitespace(char* str) {
    char* end;

    // Trim leading space
    while(isspace((unsigned char)*str)) str++;

    if(*str == 0) return;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator
    *(end + 1) = '\0';
}
