# CAN Sniffer & Analyzer - Professional Embedded Tool

## 📋 Overview

**CAN Sniffer** is a professional-grade embedded tool for real-time monitoring, analysis, and injection of CAN (Controller Area Network) bus traffic. Built on STM32 microcontroller, this tool provides comprehensive CAN bus analysis capabilities through a user-friendly USB CLI interface.

## ✨ Features

### 🎯 **Core Functionality**
- **Real-time Monitoring** - Capture and display CAN traffic in real-time
- **Bi-directional Communication** - Both receive and transmit CAN messages
- **Smart Filtering** - Hardware-accelerated ID-based filtering
- **Multiple Output Formats** - Raw hex and parsed human-readable formats
- **Bus Load Analysis** - Monitor CAN bus utilization in real-time

### 🔧 **Technical Specifications**
- **##**MCU**: STM32F407VET6 (ARM Cortex-M4 @ 64MHz)
- **##**Memory**: 128KB RAM, 512KB Flash
- **##**CAN Interface**: Full CAN 2.0A/2.0B support (11-bit & 29-bit IDs)
- **##**USB**: Virtual COM Port (CDC) for CLI interface
- **##**LED Indicators**: Dual-LED status system
- **##**Baud Rates**: Configurable up to 1Mbps

## 🚀 Quick Start

### Hardware Connection

CAN_H ───► Vehicle/Device CAN High
CAN_L ───► Vehicle/Device CAN Low
USB ───► Computer for CLI interface
text


### Basic Usage
1. Connect to USB, open terminal (115200 baud)
2. Start CAN monitoring:
   ```bash
   can start
   read raw

   Monitor traffic - messages will stream in real-time

📖 Command Reference
# CAN Control
text

can start       - Start CAN interface
can stop        - Stop CAN interface  
can info        - Show system information

# Filter Management
text

filter add <id> [mask] [std|ext]  - Add hardware filter
filter del <id|all>               - Delete filter(s)
filter list                       - List active filters

# Message Operations
text

write <id> <data>                 - Send single CAN message
write seq <id> <data> <cnt> <ms>  - Send message sequence
read raw                          - Raw hex output mode
read parsed                       - Parsed output mode

# Bus Analysis
text

bus load on      - Start bus load monitoring
bus load off     - Stop bus load monitoring  
bus load status  - Show current bus load

 💡 Usage Examples
# Basic Monitoring
bash

can start
filter add 0x100 0x7F0 std
read raw

# Send Diagnostic Request
bash

can start
write 0x7DF 02 01 0C 00 00 00 00 00

# Sequence Testing
bash

can start
write seq 0x123 01 02 03 04 100 50

## 🎨 LED Indicators
# LED1 (Red/Green) - System Status

    OFF: CAN interface stopped

    SLOW BLINK: CAN active, normal operation

    FAST BLINK: Error detected

    ON: Initialization/Test mode

# LED2 (Blue) - Activity

    Short flash (50ms): CAN message transmitted

    Short flash (50ms): CAN message received

    Long flash (100ms): USB command received

🔌 Protocol Support
# CAN Standards

    ✅ ISO 11898-2 (High-speed CAN)

    ✅ CAN 2.0A (11-bit identifiers)

    ✅ CAN 2.0B (29-bit identifiers)

    ✅ Remote Transmission Requests (RTR)

# Data Formats
text

Raw format:   [timestamp] T/R ID [dlc] data_bytes
Parsed format: Detailed message breakdown with ASCII view

⚙️ Configuration
# CAN Settings

    Baud rate: Configurable via hardware

    Sampling point: 75% (optimal for automotive)

    SJW: 1 time quantum

    Filters: 14 hardware filter banks

# USB Settings

    Baud rate: 115200

    Data bits: 8

    Stop bits: 1

    Parity: None

    Flow control: None
