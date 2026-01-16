/*
 * ProtocolFormatter.cpp
 *
 *  Created on: Dec 29, 2025
 *      Author: peleng
 */

#include "ProtocolFormatter.h"
#include "COBSLib/cobs.h"
#include <cstdio>

ProtocolFormatter::ProtocolFormatter(Format fmt) : format_(fmt) {}

uint16_t ProtocolFormatter::format(const CanMessage_t& msg,
                                  uint8_t* buffer,
                                  uint16_t buffer_size) {
    if (buffer == nullptr || buffer_size == 0) {
        return 0;
    }

    switch (format_) {
        case Format::Raw:
            return rawFormat(msg, buffer, buffer_size);
        case Format::Cobs:
            return cobsFormat(msg, buffer, buffer_size);
        case Format::Ascii:
            return asciiFormat(msg, buffer, buffer_size);
        default:
            return 0;
    }
}

void ProtocolFormatter::formatTypeIdentifier(const CanMessage_t& msg,
                                           uint8_t* buffer,
                                           uint16_t& pos) {
    // Точно как в вашем оригинальном коде
    if (msg.header.IDE == CAN_ID_STD) {
        if (msg.header.RTR == CAN_RTR_DATA) {
            buffer[pos] = 't';
        } else if (msg.header.RTR == CAN_RTR_REMOTE) {
            buffer[pos] = 'r';
        }
    } else if (msg.header.IDE == CAN_ID_EXT) {
        if (msg.header.RTR == CAN_RTR_DATA) {
            buffer[pos] = 'T';
        } else if (msg.header.RTR == CAN_RTR_REMOTE) {
            buffer[pos] = 'R';
        }
    }
    pos++;
}

static void setFormatedDatagramIdentifer(uint32_t idNum, uint8_t* pExitBuffer,
                                        uint16_t* pCursor, int len) {
    char* id = (char*)malloc(sizeof(char) * (len + 1));
    int numOfDigits = 0;
    int valueToConsume = idNum;

    while (valueToConsume != 0) {
        valueToConsume /= 10;     // n = n/10
        ++numOfDigits;
    }

    // Если idNum = 0, то numOfDigits будет 0, исправляем
    if (numOfDigits == 0) {
        numOfDigits = 1;
    }

    sprintf(id + (len - numOfDigits), "%d", (int)idNum);
    for (int eraser = 0; eraser < (len - numOfDigits); eraser++) {
        id[eraser] = '0';
    }

    id[len] = '\0';  // Завершаем строку
    memcpy((char*)pExitBuffer + *pCursor, id, len);
    free(id);
    *pCursor = *pCursor + len;
}

void ProtocolFormatter::formatIdentifier(const CanMessage_t& msg,
                                        uint8_t* buffer,
                                        uint16_t& pos) {
    // Аналог setDatagramIdentifer + setFormatedDatagramIdentifer
    char id_str[10] = {0};

    if (msg.header.IDE == CAN_ID_EXT) {
    	setFormatedDatagramIdentifer(msg.header.ExtId, buffer, &pos, 9);
    } else if (msg.header.IDE == CAN_ID_STD) {
    	setFormatedDatagramIdentifer(msg.header.StdId, buffer, &pos, 4);
    }
}

void ProtocolFormatter::formatDLC(const CanMessage_t& msg,
                                 uint8_t* buffer,
                                 uint16_t& pos) {
    // DLC всегда 0-8, один символ
    buffer[pos] = '0' + (msg.header.DLC % 10); // Преобразуем число в символ
    pos++;
}

void ProtocolFormatter::formatData(const CanMessage_t& msg,
                                  uint8_t* buffer,
                                  uint16_t& pos) {
    // Копируем данные как есть
    if (msg.header.DLC > 0 && msg.header.DLC <= 8) {
        memcpy(&buffer[pos], msg.data, msg.header.DLC);
        pos += msg.header.DLC;
    }
}

void ProtocolFormatter::formatTimestamp(const CanMessage_t& msg,
                                       uint8_t* buffer,
                                       uint16_t& pos) {
    // Вспомогательный метод для ASCII формата
    char time_str[12];
    snprintf(time_str, sizeof(time_str), "%010lu", msg.timestamp_ms);
    memcpy(&buffer[pos], time_str, 10);
    pos += 10;
}

// Статические методы для прямого использования (без создания объекта)

uint16_t ProtocolFormatter::rawFormat(const CanMessage_t& msg,
                                     uint8_t* buffer,
                                     uint16_t buffer_size) {
    uint16_t pos = 0;

    // 1. Тип сообщения (1 байт)
    formatTypeIdentifier(msg, buffer, pos);

    // 2. Идентификатор (4 или 9 байт)
    formatIdentifier(msg, buffer, pos);

    // 3. DLC (1 байт)
    formatDLC(msg, buffer, pos);

    // 4. Данные (0-8 байт)
    if (msg.header.RTR == CAN_RTR_DATA) {
        formatData(msg, buffer, pos);
    }

    return pos;
}

uint16_t ProtocolFormatter::cobsFormat(const CanMessage_t& msg,
                                      uint8_t* buffer,
                                      uint16_t buffer_size) {
    // 1. Сначала создаем "сырое" сообщение
    uint8_t raw_buffer[24] = {0};
    uint16_t raw_len = rawFormat(msg, raw_buffer, sizeof(raw_buffer));

    // Добавляем нулевой байт в конец (как в вашем коде)
    raw_buffer[raw_len] = 0x00;
    raw_len++;

    // 2. Кодируем COBS
    cobs_encode_result result = cobs_encode(buffer, buffer_size,
                                           raw_buffer, raw_len);

    if (result.status == COBS_ENCODE_OK) {
        // Добавляем нулевой байт как разделитель (как в вашем коде)
        if (result.out_len + 1 < buffer_size) {
            buffer[result.out_len] = 0x00;
            return result.out_len + 1;
        }
    }

    return 0;
}

uint16_t ProtocolFormatter::asciiFormat(const CanMessage_t& msg,
                                       uint8_t* buffer,
                                       uint16_t buffer_size) {
    // Формат: [TIME] TYPE ID DLC DATA
    // Пример: [0012345678] T001234567 8 01 02 03 04

    char* buf = reinterpret_cast<char*>(buffer);
    int pos = 0;

    // 1. Временная метка
    pos += snprintf(buf + pos, buffer_size - pos, "[%010lu] ",
                    msg.timestamp_ms);

    // 2. Тип сообщения
    if (msg.header.IDE == CAN_ID_STD) {
        if (msg.header.RTR == CAN_RTR_DATA) {
            pos += snprintf(buf + pos, buffer_size - pos, "STD_DATA ");
        } else {
            pos += snprintf(buf + pos, buffer_size - pos, "STD_REMOTE ");
        }
    } else {
        if (msg.header.RTR == CAN_RTR_DATA) {
            pos += snprintf(buf + pos, buffer_size - pos, "EXT_DATA ");
        } else {
            pos += snprintf(buf + pos, buffer_size - pos, "EXT_REMOTE ");
        }
    }

    // 3. Идентификатор
    if (msg.header.IDE == CAN_ID_STD) {
        pos += snprintf(buf + pos, buffer_size - pos, "0x%03X ",
                       msg.header.StdId);
    } else {
        pos += snprintf(buf + pos, buffer_size - pos, "0x%08lX ",
                       msg.header.ExtId);
    }

    // 4. DLC
    pos += snprintf(buf + pos, buffer_size - pos, "DLC:%d ",
                   msg.header.DLC);

    // 5. Данные (если есть)
    if (msg.header.RTR == CAN_RTR_DATA && msg.header.DLC > 0) {
        pos += snprintf(buf + pos, buffer_size - pos, "DATA:");
        for (int i = 0; i < msg.header.DLC; i++) {
            pos += snprintf(buf + pos, buffer_size - pos, "%02X", msg.data[i]);
            if (i < msg.header.DLC - 1) {
                pos += snprintf(buf + pos, buffer_size - pos, " ");
            }
        }
    }

    // Завершаем строку
    pos += snprintf(buf + pos, buffer_size - pos, "\r\n");

    return pos;
}
