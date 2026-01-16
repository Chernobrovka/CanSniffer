#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os
import time
from datetime import datetime
import re

class ComPortLogger:
    """Логгер для COM порта"""
    
    def __init__(self, log_dir="logs"):
        self.log_dir = log_dir
        self.current_log_file = None
        self.log_enabled = False
        self.log_start_time = None
        
        # Создать директорию для логов
        os.makedirs(log_dir, exist_ok=True)
    
    def _sanitize_filename(self, filename):
        """Очистить имя файла от недопустимых символов"""
        # Заменить недопустимые символы на подчеркивания
        sanitized = re.sub(r'[<>:"/\\|?*]', '_', filename)
        # Удалить начальные и конечные пробелы/точки
        sanitized = sanitized.strip(' .')
        # Ограничить длину
        if len(sanitized) > 100:
            sanitized = sanitized[:100]
        return sanitized
    
    def _get_safe_port_name(self, port_name):
        """Получить безопасное имя порта для использования в имени файла"""
        if not port_name:
            return ""
        
        # Извлечь только имя устройства без пути
        port_basename = os.path.basename(port_name)
        
        # Для Linux: /dev/ttyUSB0 → ttyUSB0, /dev/ttyS0 → ttyS0
        # Для Windows: COM3 → COM3
        safe_name = self._sanitize_filename(port_basename)
        return safe_name
    
    def start_logging(self, port_name=None):
        """Начать логирование"""
        if self.log_enabled:
            self.stop_logging()
        
        # Получить безопасное имя порта
        safe_port_name = self._get_safe_port_name(port_name)
        
        # Создать имя файла с timestamp и именем порта
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        if safe_port_name:
            filename = f"com_log_{timestamp}_{safe_port_name}.txt"
        else:
            filename = f"com_log_{timestamp}.txt"
        
        filepath = os.path.join(self.log_dir, filename)
        
        try:
            self.current_log_file = open(filepath, 'a', encoding='utf-8')
            self.log_start_time = time.time()
            self.log_enabled = True
            
            # Записать заголовок
            self._write_header(port_name)
            
            print(f"COM logging started: {filepath}")
            return filepath
            
        except Exception as e:
            print(f"Failed to start logging: {e}")
            # Попробовать создать файл с простым именем
            return self._create_simple_log_file()
    
    def _create_simple_log_file(self):
        """Создать лог-файл с простым именем в случае ошибки"""
        try:
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            filename = f"com_log_{timestamp}.txt"
            filepath = os.path.join(self.log_dir, filename)
            
            self.current_log_file = open(filepath, 'a', encoding='utf-8')
            self.log_start_time = time.time()
            self.log_enabled = True
            
            self._write_header("Unknown")
            
            print(f"Created simple log file: {filepath}")
            return filepath
            
        except Exception as e:
            print(f"Failed to create simple log file: {e}")
            return None
    
    def stop_logging(self):
        """Остановить логирование"""
        if self.current_log_file:
            try:
                self._write_footer()
                self.current_log_file.close()
            except Exception as e:
                print(f"Error closing log file: {e}")
            finally:
                self.current_log_file = None
        self.log_enabled = False
        print("COM logging stopped")
    
    def log_raw_data(self, data, direction="IN"):
        """
        Записать сырые данные
        direction: "IN" - входящие данные, "OUT" - исходящие, "ERR" - ошибки
        """
        if not self.log_enabled or not self.current_log_file:
            return
        
        try:
            timestamp = time.time() - self.log_start_time
            timestamp_str = f"[{timestamp:012.3f}]"
            direction_str = f"[{direction:3s}]"
            
            # Форматировать данные
            if isinstance(data, bytes):
                # Байты
                hex_str = ' '.join(f"{b:02X}" for b in data)
                ascii_str = ''.join(chr(b) if 32 <= b < 127 else '.' for b in data)
                log_line = f"{timestamp_str} {direction_str} HEX: {hex_str:40s} | ASCII: {ascii_str}\n"
            elif isinstance(data, str):
                # Строка
                log_line = f"{timestamp_str} {direction_str} STR: {data}\n"
            else:
                # Другие типы
                log_line = f"{timestamp_str} {direction_str} OBJ: {str(data)}\n"
            
            self.current_log_file.write(log_line)
            self.current_log_file.flush()  # Сразу на диск
            
        except Exception as e:
            print(f"Log write error: {e}")
    
    def log_can_message(self, can_datagram):
        """Записать CAN сообщение"""
        if not self.log_enabled or not self.current_log_file:
            return
        
        try:
            timestamp = time.time() - self.log_start_time
            timestamp_str = f"[{timestamp:012.3f}]"
            
            # Получить тип сообщения
            type_code = can_datagram.getDatagramTypeCode() if hasattr(can_datagram, 'getDatagramTypeCode') else '?'
            
            type_map = {'t': 'STD_DATA', 'r': 'STD_RTR', 
                       'T': 'EXT_DATA', 'R': 'EXT_RTR'}
            msg_type = type_map.get(type_code, 'UNKNOWN')
            
            # ID в HEX
            message_id = can_datagram.messageID if hasattr(can_datagram, 'messageID') else 0
            
            if type_code in ['T', 'R']:
                can_id = f"0x{message_id:08X}"
            elif type_code in ['t', 'r']:
                can_id = f"0x{message_id & 0x7FF:03X}"
            else:
                can_id = f"0x{message_id:08X}"
            
            # Данные
            data_str = ""
            if hasattr(can_datagram, 'data') and can_datagram.data is not None and len(can_datagram.data) > 0:
                dlc = can_datagram.dlc if hasattr(can_datagram, 'dlc') else len(can_datagram.data)
                data_str = ' '.join(f"{b:02X}" for b in can_datagram.data[:dlc])
            
            log_line = f"{timestamp_str} [CAN] Type: {msg_type:12s} ID: {can_id:10s} Data: {data_str}\n"
            
            self.current_log_file.write(log_line)
            self.current_log_file.flush()
            
        except Exception as e:
            print(f"CAN log error: {e}")
    
    def add_empty_line(self):
        """Добавить пустую строку в лог"""
        if self.log_enabled and self.current_log_file:
            try:
                self.current_log_file.write("\n")
                self.current_log_file.flush()
            except Exception as e:
                print(f"Error adding empty line: {e}")

    def _write_header(self, port_name):
        """Записать заголовок лога"""
        if not self.current_log_file:
            return
            
        header = [
            "=" * 80,
            "COM PORT LOG FILE",
            f"Created: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}",
            f"Port: {port_name if port_name else 'Not specified'}",
            "Format: [timestamp] [direction] data",
            "=" * 80,
            ""
        ]
        self.current_log_file.write('\n'.join(header) + '\n')
        self.current_log_file.flush()
    
    def _write_footer(self):
        """Записать футер лога"""
        if self.current_log_file and self.log_start_time:
            footer = [
                "",
                "=" * 80,
                f"Log ended: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}",
                f"Duration: {time.time() - self.log_start_time:.1f} seconds",
                "=" * 80
            ]
            self.current_log_file.write('\n'.join(footer) + '\n')
            self.current_log_file.flush()