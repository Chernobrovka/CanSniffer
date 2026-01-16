/*
 * ProtocolFormatter.h
 *
 *  Created on: Dec 29, 2025
 *      Author: peleng
 */

#ifndef PROTOCOLFORMATTER_PROTOCOLFORMATTER_H_
#define PROTOCOLFORMATTER_PROTOCOLFORMATTER_H_

#include "CanProcessor/CanProcessor.h"
#include <cstring>
#include <cstdint>

class ProtocolFormatter {
public:
    enum class Format {
        Raw,     // Сырые данные
        Cobs,    // COBS кодирование
        Ascii    // Человекочитаемый ASCII
    };

    ProtocolFormatter(Format fmt = Format::Cobs);

    uint16_t format(const CanMessage_t& msg, uint8_t* buffer, uint16_t buffer_size);

    void setFormat(Format fmt) { format_ = fmt; }
    Format getFormat() const { return format_; }

    uint16_t rawFormat(const CanMessage_t& msg, uint8_t* buffer, uint16_t buffer_size);
    uint16_t cobsFormat(const CanMessage_t& msg, uint8_t* buffer, uint16_t buffer_size);
    uint16_t asciiFormat(const CanMessage_t& msg, uint8_t* buffer, uint16_t buffer_size);

private:
    Format format_;

    void formatTypeIdentifier(const CanMessage_t& msg, uint8_t* buffer, uint16_t& pos);
    void formatIdentifier(const CanMessage_t& msg, uint8_t* buffer, uint16_t& pos);
    void formatDLC(const CanMessage_t& msg, uint8_t* buffer, uint16_t& pos);
    void formatData(const CanMessage_t& msg, uint8_t* buffer, uint16_t& pos);
    void formatTimestamp(const CanMessage_t& msg, uint8_t* buffer, uint16_t& pos);
};



#endif /* PROTOCOLFORMATTER_PROTOCOLFORMATTER_H_ */
