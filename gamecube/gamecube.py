import serial
import sys
import time
import struct

class GameCubeControllerState(object):
    def __init__(self, data):
        self.bytes = struct.unpack('BBBBBBBB', data)
        self.noOriginGet = self.getBit(0,7-2)
        self.start = self.getBit(0,7-3)
        self.y = self.getBit(0,7-4)
        self.x = self.getBit(0,7-5)
        self.b = self.getBit(0,7-6)
        self.a = self.getBit(0,7-7)
        self.l = self.getBit(1,7-1)
        self.r = self.getBit(1,7-2)
        self.z = self.getBit(1,7-3)
        self.dup = self.getBit(1,7-4)
        self.ddown = self.getBit(1,7-5)
        self.dright = self.getBit(1,7-6)
        self.dleft = self.getBit(1,7-7)
        self.joyx = self.bytes[2]
        self.joyy = self.bytes[3]
        self.cx = self.bytes[4]
        self.cy = self.bytes[5]
        self.leftbutton = self.bytes[6]
        self.rightbutton = self.bytes[7]
        
    def getBit(self,byteNum,bitNum):
        return (self.bytes[byteNum] & (1<<bitNum))>>bitNum
        
    def __repr__(self):
        out = []
        if self.start:
            out.append("Start")
        if self.a:
            out.append("A")
        if self.b:
            out.append("B")
        if self.x:
            out.append("X")
        if self.y:
            out.append("Y")
        if self.z:
            out.append("Z")
        if self.l:
            out.append("L")
        if self.r:
            out.append("R")
        if self.dup:
            out.append("D-Up")
        if self.ddown:
            out.append("D-Down")
        if self.dleft:
            out.append("D-Left")
        if self.dright:
            out.append("D-Right")
        out.append("Joy=(%d,%d)"%(self.joyx,self.joyy))
        out.append("C-Stick=(%d,%d)"%(self.cx,self.cy))
        out.append("Shoulder=(%d,%d)"%(self.leftbutton,self.rightbutton))
        return " ".join(out)

if __name__ == '__main__':                
    ser = serial.Serial("com3", baudrate=115200, timeout=0.1)
    ser.write(b'*#1')

    while True:
        ser.reset_input_buffer()
        ser.write(b'c000')
        t0 = time.time()
        while time.time() - t0 < 1:
            c = ser.read()
            if c == b'E':
                ser.reset_input_buffer()
                print("Error")
                break
            elif c == b'G':
                print(GameCubeControllerState(ser.read(8)))
                break
        if time.time() - t0 >= 1:
            print("Timeout")
        time.sleep(0.5)
    