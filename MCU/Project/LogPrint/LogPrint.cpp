
/*
 * LogPrint.c
 *
 *  Created on: Dec 2, 2025
 *      Author: gadzilla
 */
#ifdef DEBUG
#include "LogPrint.h"
#include <App.hpp>
#include "usart.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>


void debugPrint(const char* format, ...) {
    char buffer[128];
    va_list args;

    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (len > 0 && len < sizeof(buffer)) {
        HAL_UART_Transmit(LOG_UART_PTR, (uint8_t*)buffer, len, 2000);
    }
}
#else

void debugPrint(const char* format, ...) {
	(void)format;
}

#endif
