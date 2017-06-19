from __future__ import print_function
import struct
import time
import sys

byteToInt = int if sys.version_info[0] >= 3 else ord

class GameCubeControllerState(object):
    def __init__(self, data=None):
        if data is None:
            self.bytes = tuple(0 for i in range(8))
        elif len(data)==16:
            self.bytes = tuple(int(data[i:i+2],16) for i in range(0,16,2))
        elif len(data)==8:
            self.bytes = tuple(byteToInt(a) for a in data)
        else:
            raise Exception("Invalid data")
        self.noOriginGet = self.getBit(0,7-2)
        self.start = self.getBit(0,7-3)
        self.y = self.getBit(0,7-4)
        self.x = self.getBit(0,7-5)
        self.b = self.getBit(0,7-6)
        self.a = self.getBit(0,7-7)
        self.l = self.getBit(1,7-1)
        self.r = self.getBit(1,7-2)
        self.z = self.getBit(1,7-3)
        self.dUp = self.getBit(1,7-4)
        self.dDown = self.getBit(1,7-5)
        self.dRight = self.getBit(1,7-6)
        self.dLeft = self.getBit(1,7-7)
        self.joyX = self.bytes[2]
        self.joyY = self.bytes[3]
        self.cX = self.bytes[4]
        self.cY = self.bytes[5]
        self.shoulderLeft = self.bytes[6]
        self.shoulderRight = self.bytes[7]
        
    @staticmethod
    def isValidHex(data):
        if data is None or len(data) != 16:
            return False
        for x in data:
            if not (ord('0')<=x<=ord('9') or ord('A')<=x<=ord('F')):
                return False
        return True
        
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
        if self.dUp:
            out.append("D-Up")
        if self.dDown:
            out.append("D-Down")
        if self.dLeft:
            out.append("D-Left")
        if self.dRight:
            out.append("D-Right")
        out.append("Joy=(%d,%d)"%(self.joyX,self.joyY))
        out.append("C-Stick=(%d,%d)"%(self.cX,self.cY))
        out.append("Shoulder=(%d,%d)"%(self.shoulderLeft,self.shoulderRight))
        return " ".join(out)
        
def processGameCubeController(ser, delay, processor, quiet=False, debug=False):
    command = b'**#1c0'+bytes(bytearray([delay]))+b'0'
    ser.reset_input_buffer()
    ser.write(command)

    lastState = GameCubeControllerState()
    count = 0
    startt = time.time()

    while True:
        t0 = time.time()
        while time.time() - t0 < 1:
            c = ser.read()
            if c == b'E':
                ser.reset_input_buffer()
                if not quiet: print("Error 1")
                break
            elif c == b'G':
                data = ser.read(16)
                if GameCubeControllerState.isValid(data):
                    newState = GameCubeControllerState(data)
                    processor(newState)
                    lastState = newState
                    count += 1
                    if debug and count%1000 == 0: print("freq %f"%(count/(time.time()-startt)))
                else:
                    if not quiet: print("Error 2: "+str(data))
                    ser.write(command)
                    ser.reset_input_buffer()
                break
        if time.time() - t0 >= 1:
            if not quiet: print("Timeout")
            ser.write(command)
            ser.reset_input_buffer()

if __name__ == '__main__':                
    import serial
    import sys
    import time

    ser = serial.Serial("com3" if len(sys.argv)<2 else sys.argv[1], baudrate=115200, timeout=0.1)
    def show(s0,s1):
        print(s1)
    processGameCubeController(ser, 255, show, debug=True)
