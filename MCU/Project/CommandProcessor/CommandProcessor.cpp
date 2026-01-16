/*
 * CommandProcessor.cpp
 *
 *  Created on: Dec 30, 2025
 *      Author: peleng
 */
#include "CommandProcessor.h"
#include "CommandHandler/CommandHandler.h"

CommandProcessor::CommandProcessor(Queue_t *queue_ptr,
		CanStartCallback can_start_cb,
		CanStopCallback can_stop_cb,
		CanInfoCallback can_info_cb,
		FilterAddCallback filter_add_cb,
		FilterDelCallback filter_del_cb,
		FilterListCallback filter_list_cb,
		WriteCallback write_cb,
		WriteSeqCallback write_seq_cb,
		ReadRawCallback read_raw_cb,
		ReadParsedCallback read_parsed_cb,
		ErrorCallback error_cb,
		HandleBusLoadMonitorCallback handle_bus_load_monitor_cb,
		HandleBusLoadStatusCallback  handle_bus_load_status_cb)
	: command_queue_(queue_ptr),
	  can_start_callback_(can_start_cb),
	  can_stop_callback_(can_stop_cb),
	  can_info_callback_(can_info_cb),
	  filter_add_callback_(filter_add_cb),
	  filter_del_callback_(filter_del_cb),
	  filter_list_callback_(filter_list_cb),
	  write_callback_(write_cb),
	  write_seq_callback_(write_seq_cb),
	  read_raw_callback_(read_raw_cb),
	  read_parsed_callback_(read_parsed_cb),
	  error_callback_(error_cb),
	  handle_bus_load_monitor_callback_(handle_bus_load_monitor_cb),
	  handle_bus_load_status_callback_(handle_bus_load_status_cb){}

void CommandProcessor::processCommand() {
	Command cmd;
    while (getCommand(cmd)) {
        processSingleCommand(cmd);
    }
}

typedef void (*FilterAddCallback)(uint32_t id, uint32_t mask, FilterType type);
typedef void (*FilterDelCallback)(uint32_t id, bool delete_all);

void CommandProcessor::processSingleCommand(const Command& cmd){
	switch (cmd.type) {
        case CMD_CAN_START:{
        	can_start_callback_();
        	break;
        }
        case CMD_CAN_STOP:{
        	can_stop_callback_();
			break;
        }
        case CMD_CAN_INFO:{
        	can_info_callback_();
			break;
        }
        case CMD_FILTER_ADD:{
        	filter_add_callback_(cmd.params.filter.id, cmd.params.filter.mask, cmd.params.filter.filter_type);
            break;
        }
        case CMD_FILTER_DEL:{
        	filter_del_callback_(cmd.params.filter.id, cmd.params.filter.delete_all);
            break;
        }
        case CMD_FILTER_LIST:{
        	filter_list_callback_();
            break;
        }
        case CMD_WRITE:{
        	write_callback_(cmd.params.write.id, (uint8_t*)cmd.params.write.data, cmd.params.write.dlc);
            break;
        }
        case CMD_WRITE_SEQ:{
        	write_seq_callback_(cmd.params.write.id, (uint8_t*)cmd.params.write.data, cmd.params.write.dlc,
        						cmd.params.write.count, cmd.params.write.interval_ms);
            break;
        }
        case CMD_READ_RAW:{
        	read_raw_callback_();
            break;
        }
        case CMD_READ_PARSED:{
        	read_parsed_callback_();
			break;
        }
        case CMD_BUS_LOAD_ON:{
        	handle_bus_load_monitor_callback_(true);
        	break;
        }
        case CMD_BUS_LOAD_OFF:{
			handle_bus_load_monitor_callback_(false);
			break;
		}
        case CMD_BUS_LOAD_STATUS:{
			handle_bus_load_status_callback_();
			break;
		}
        default:
            //printf("Unknown command\r\n");
            break;
    }
}

bool CommandProcessor::getCommand(Command& cmd){
	if (q_isEmpty(command_queue_)){
		return false;
	}

	return q_pop(command_queue_, &cmd);
}
