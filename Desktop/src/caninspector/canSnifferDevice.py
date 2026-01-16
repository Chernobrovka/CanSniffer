#!/usr/bin/env python
# -*- coding: utf-8 -*-
import threading

import serial
from PyQt5 import QtCore, QtWidgets
from cobs import cobs
from serial import SerialException
import logging

from src.caninspector.canDatagram import CANDatagram
from src.caninspector.comPortLogger import ComPortLogger

class CanSnifferDevice(QtCore.QThread):

    canBusBitRates = {'250.000': "250",
                      '1000.000': "1000",
                      '500.000': "500",
                      '125.000': "125",
                      '100.000': "100",
                      '50.000': "050",
                      '20.000': "020",
                      '10.000': "010",
                      }
    canBusModes = {'NORMAL': 'NM',
                   'LOOP BACK':'LB',
                   'SILENT': 'SM',
                   'SILENT LOOP BACK': 'SL'
                   }

    datagramReceived = QtCore.pyqtSignal(CANDatagram)

    def __init__(self, serialPortName=None, *args, **kwargs):

        super(CanSnifferDevice, self).__init__(*args, **kwargs)
        self._stop_event = threading.Event()
        self.serialPortName = serialPortName
        self.bitrate = 115200
        self.serialPort = None
        self.isSnifing = False
        self.isConnected = False
        self.connectionStateListenerList = []

        self.logger = ComPortLogger()
        self.logging_enabled = True
        
    def enable_logging(self, enable=True):
        """Включить/выключить логирование"""
        self.logging_enabled = enable
        if enable and not self.logger.log_enabled and self.isConnected:
            self.logger.start_logging(self.serialPortName)
        elif not enable and self.logger.log_enabled:
            self.logger.stop_logging()

    def handle_received_data(self, data):
        try:
            if self.logging_enabled:
                self.logger.log_raw_data(data, "IN")

            datagram = CANDatagram.parseCANDatagram(data[:-1])

            if self.logging_enabled:
                self.logger.log_can_message(datagram)
                self.logger.add_empty_line()

            self.datagramReceived.emit(datagram)
        except Exception as error:
            logging.error(error, exc_info=True)
            logging.error("data =" + str(data), exc_info=True)

            if self.logging_enabled:
                self.logger.log_raw_data(f"ERROR: {error}", "ERR")

    def run(self):
        while self.isSnifing:
            if self.isConnected:
                if self.serialPort is not None:
                    if self.serialPort.isOpen():
                        packet = self.serialPort.read_until(b'\x00')
                        if self.logging_enabled:
                            self.logger.log_raw_data(packet, "RAW")

                        try:
                            decodedCOBS = cobs.decode(packet[:-1])

                            if self.logging_enabled:
                                self.logger.log_raw_data(decodedCOBS, "DEC")

                            self.handle_received_data(decodedCOBS)
                        except UnicodeDecodeError as error:
                            logging.exception(error)
                            if self.logging_enabled:
                                self.logger.log_raw_data(f"Unicode ERROR: {error}", "ERR")
                        except cobs.DecodeError as error:
                            logging.exception(error)
                            if self.logging_enabled:
                                self.logger.log_raw_data(f"COBS ERROR: {error}", "ERR")

    def stop(self):
        self._stop_event.set()
        if self.logger.log_enabled:
            self.logger.stop_logging()

    def startSniffing(self):
        self.isSnifing = True

        if self.logging_enabled and not self.logger.log_enabled:
            self.logger.start_logging(self.serialPortName)

        commandValue = "A".encode() + 'ON_'.encode()
        valueToSend = cobs.encode(commandValue)

        if self.logging_enabled:
            self.logger.log_raw_data(commandValue, "OUT")
            self.logger.log_raw_data(valueToSend + b'\00', "OUT_ENC")

        self.serialPort.write(valueToSend + b'\00')
        self.start()

    def stopSniffing(self):
        commandValue = "A".encode() + 'OFF'.encode()
        valueToSend = cobs.encode(commandValue)

        if self.logging_enabled:
            self.logger.log_raw_data(commandValue, "OUT")
            self.logger.log_raw_data(valueToSend + b'\00', "OUT_ENC")

        self.serialPort.write(valueToSend + b'\00')
        self.isSnifing = False

    def __initCommunications(self):
        self.serialPort = serial.Serial(self.serialPortName, self.bitrate)
        self.serialPort.reset_input_buffer()
        self.serialPort.reset_output_buffer()
        self.isConnected = True

    def __closeCommunication(self):
        # self.responseThread.stop()
        if not self.serialPort.isOpen():
            self.serialPort.close()

        if self.logger.log_enabled:
            self.logger.stop_logging()

    def connectSerial(self):
        try:
            self.__initCommunications()
            self.isConnected = True

            if self.logging_enabled:
                self.logger.start_logging(self.serialPortName)
        except SerialException as serEx:
            logging.warning('Is not possible to open serial port')
            logging.warning('Port =' + self.serialPortName)
            msgBox = QtWidgets.QMessageBox()
            msgBox.setIcon(QtWidgets.QMessageBox.Warning)
            msgBox.setText("Error while trying to open serial port")
            msgBox.setWindowTitle("CAN Inspector Tool")
            msgBox.setStandardButtons(QtWidgets.QMessageBox.Ok)
            msgBox.exec()
            return False
        else:
            self.isConnected = True
            for listener in self.connectionStateListenerList:
                listener.deviceConnected(True)
            return True

    def disconnectSerial(self):
        self.__closeCommunication()
        self.isConnected = False
        for listener in self.connectionStateListenerList:
            listener.deviceConnected(False)

    def addConnectionStateListener(self, listener):
        self.connectionStateListenerList.append(listener)

    def sendDatagram(self, datagran):
        valueToSend = cobs.encode('M'.encode() + datagran.serialize())
        self.serialPort.write(valueToSend + b'\00')

    def sendBitrateCommand(self, bitrateCode):
        commandValue = "S".encode() + str(self.canBusBitRates[bitrateCode]).encode()
        valueToSend = cobs.encode(commandValue)
        self.serialPort.write(valueToSend + b'\00')

    def sendModeCommand(self, modeCode):
        commandValue = "N".encode() + str(self.canBusModes[modeCode]).encode()
        valueToSend = cobs.encode(commandValue)
        self.serialPort.write(valueToSend + b'\00')

    def rebootDevice(self):
        commandValue = "R".encode()
        valueToSend = cobs.encode(commandValue)
        self.serialPort.write(valueToSend + b'\00')

    def sendRawCommand(self, commandSrt):
        commandValue = commandSrt.encode()
        valueToSend = cobs.encode(commandValue)
        self.serialPort.write(valueToSend + b'\00')
    
    def sendFilterCommand(self, bank, type, messageID):
        commandValue = "F".encode()
        if type in ("S", "E"):
            commandValue = commandValue + type.encode()
        else:
            raise Exception('Invalid type [{}] arguments only '
                            'E o S accepted'.format(type))
        if bank in range(13):
            commandValue = commandValue + str(bank).zfill(2).encode()
        else:
            raise Exception('Invalid bank [{}] argument only '
                            'between 0 and 13 is accepted'.format(type))
        if type is "S" and messageID <= 2**11:
            commandValue = commandValue + str(messageID).zfill(4).encode()
        else:
            raise Exception('Invalid messageID {}] argument only '
                            'lower than 2^11 accepted'.format(messageID))
        if type is "E" and messageID <= 2**29:
            commandValue = commandValue + str(messageID).zfill(9).encode()
        else:
            raise Exception('Invalid bank [{}] argument only '
                            'lower than 2^29 accepted'.format(messageID))
        valueToSend = cobs.encode(commandValue)
        self.serialPort.write(valueToSend + b'\00')
