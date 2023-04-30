import pyautogui as pg
import serial
import argparse
import time
import logging

from pycaw.pycaw import AudioUtilities, IAudioEndpointVolume
from comtypes import CLSCTX_ALL
from ctypes import cast, POINTER
import numpy as np


class MyControllerMap:
    def __init__(self):
        
        self.devices = AudioUtilities.GetSpeakers()
        self.interface = self.devices.Activate(IAudioEndpointVolume._iid_, CLSCTX_ALL, None)
        self.volume = cast(self.interface, POINTER(IAudioEndpointVolume))
        self.volMin,self.volMax = self.volume.GetVolumeRange()[:2]
        print(self.volume.GetVolumeRange()[:2])
        self.button = {'A': 'L','B':'H'}# Fast forward (10 seg) pro Youtube
       
class SerialControllerInterface:
    # Protocolo
    # byte 1 -> Botão 1 (estado - Apertado 1 ou não 0)
    # byte 2 -> EOP - End of Packet -> valor reservado 'X'

    def __init__(self, port, baudrate):
        self.ser = serial.Serial(port, baudrate=baudrate)
        self.mapping = MyControllerMap()
        self.incoming = '0'
        pg.PAUSE = 0  ## remove delay
        self.hand = True
        pg.moveTo(500, 1100)
        pg.click()
        # pg.press('win')
        # pg.write('teams')
        # pg.press('enter')

    def update(self):
        ## Sync protocol
        logging.debug("Handshake realizado")
        head = self.ser.read()
        while head != b'A':
            head = self.ser.read()
            logging.debug("Wrong Head received: {}".format(head))

        logging.debug("Correct Head received: {}".format(head))

        data = self.ser.read()
        logging.debug("Received DATA: {}".format(data))
        
        if data == b'V':
            logging.info("Controlando volume")
            byte1 = self.ser.read()
            byte2 = self.ser.read()
            byte1 = int.from_bytes(byte1 , "little")
            byte2 = int.from_bytes(byte2, "little")
            vol = (byte2 << 8) | byte1
            self.mapping.volume.SetMasterVolumeLevel(np.interp(vol,[100,4096],[self.mapping.volMin,self.mapping.volMax]) , None)
        if data == b'1':
            logging.info("Hotkey A")
            pg.hotkey('ctrl', 'shift', 'm')
        elif data == b'2':
            logging.info("hotkey B")
            pg.hotkey('ctrl', 'shift', 'o')
        elif data == b'3':
            logging.info("hotkey C")
            pg.hotkey('ctrl', 'shift', 'a')
        elif data == b'4':
            logging.info("hotkey D")
            pg.hotkey('ctrl', 'shift', 'h')
        elif data == b'5':
            logging.info("hotkey E")
            pg.hotkey('ctrl', 'shift', 'k')
        
        self.incoming = self.ser.read()
        while self.incoming != b'X':
            self.incoming = self.ser.read()
            logging.debug("Wrong EOP received: {}".format(self.incoming))
        logging.debug("Wrong EOP received: {}".format(self.incoming))
        


class DummyControllerInterface:
    def __init__(self):
        self.mapping = MyControllerMap()

    def update(self):
        pg.hotkey('ctrl', 'shift', 'M')
        logging.info("[Dummy] Pressed A button")
        time.sleep(1)
        pg.hotkey('ctrl', 'shift', 'H')
        logging.info("[Dummy] Pressed B button")
        time.sleep(1)


if __name__ == '__main__':
    interfaces = ['dummy', 'serial']
    argparse = argparse.ArgumentParser()
    argparse.add_argument('serial_port', type=str)
    argparse.add_argument('-b', '--baudrate', type=int, default=9600)
    argparse.add_argument('-c', '--controller_interface', type=str, default='serial', choices=interfaces)
    argparse.add_argument('-d', '--debug', default=False, action='store_true')
    args = argparse.parse_args()
    if args.debug:
        logging.basicConfig(level=logging.DEBUG)

    print("Connection to {} using {} interface ({})".format(args.serial_port, args.controller_interface, args.baudrate))
    if args.controller_interface == 'dummy':
        controller = DummyControllerInterface()
    else:
        controller = SerialControllerInterface(port=args.serial_port, baudrate=args.baudrate)

    while True:
        controller.update()
