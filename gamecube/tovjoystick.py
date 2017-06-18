import pyvjoy
import math
from gamecube import *
import time

DEBUG = True
GC_BUTTONS = ("a", "b", "x", "y", "z", "start", "l", "r")
        
class GameCubeToVJoystick(object):
    def __init__(self, buttonMap, combineShoulders = False, threshold=1):
        self.buttonMap = buttonMap
        self.combineShoulders = combineShoulders
        self.threshold = threshold
      
    def buttonActive(self, state, button):
        value = getattr(state, button)
        if value == True or value == False:
            return value
        return value >= self.threshold
      
    @staticmethod
    def remapAxis(value):
        v1 = math.floor(value*32767./255+1.5)
        return min(max(int(v1),1),32768)

    def toVJoystick(self, joy, state):
        joy.data.lButtons = sum(1<<i for i in range(len(self.buttonMap)) if self.buttonActive(state, self.buttonMap[i]))
        joy.data.wAxisX = GameCubeToVJoystick.remapAxis(state.joyX)
        joy.data.wAxisY = GameCubeToVJoystick.remapAxis(255-state.joyY)
        joy.data.wAxisZRot = GameCubeToVJoystick.remapAxis(state.cX)
        joy.data.wAxisYRot = GameCubeToVJoystick.remapAxis(255-state.cY)
        if self.combineShoulders:
            joy.data.wSlider = GameCubeToVJoystick.remapAxis((255+state.shoulderLeft - state.shoulderRight)/2)
        else:
            joy.data.wSlider = GameCubeToVJoystick.remapAxis(255-state.shoulderRight)
        joy.data.wDial = GameCubeToVJoystick.remapAxis(255-state.shoulderLeft)
        joy.update()
        
        rid = joy.rID
        
        vector = [0,0]
        if state.dUp:
            vector[1] += 1
        elif state.dRight:
            vector[0] += 1
        elif state.dDown:
            vector[1] -= 1
        elif state.dLeft:
            vector[0] -= 1
            
        if vector[0] == 0 and vector[1] == 0:
            pov1 = -1
        else:
            pov1 = (-9000+(100*int(math.floor(0.5+180/math.pi * math.atan2(vector[1],-vector[0]))))) % 36000
            
        joy._sdk.SetContPov(pov1,rid,1)
        
        pov2 = -1
        px = state.cX - 128
        py = state.cY - 128
        if px*px + py*py >= 10*10:
            pov2 = (-9000+int(math.floor(0.5+100*180/math.pi * math.atan2(py,-px)))) % 36000
            
        joy._sdk.SetContPov(pov2,rid,2)
        
PAD_OPTIONS_DEFAULT = GameCubeToVJoystick(GC_BUTTONS)
        
maps = {
    "normal": PAD_OPTIONS_DEFAULT,
    "normal-combined": GameCubeToVJoystick(GC_BUTTONS, combineShoulders = True),
    "jetset": GameCubeToVJoystick(("a", "b", "y", "z", "x", "shoulderRight", "shoulderLeft", "start"))
    }
    
if __name__ == '__main__':
    import sys
    import serial
    import serial.tools.list_ports
    import os
    import atexit
    
    def err(message):
        if DEBUG: print(message)

    port = None
    map = maps["normal"]
    delay = 5
    startVJoy = True
    
    options = {}

    for item in sys.argv[1:]:
        if item in maps:
            map = maps[item]
        elif item == "skip-vjoy":
            startVJoy = False
        elif item == "combine-shoulders":
            map.combineShoulders = True
        else:
            try:
                delay = int(item)
            except:
                port = item
                print (port)
        
    if port is None:
        port = serial.tools.list_ports.comports()[0].device
        
    print("Connecting to serial on "+port)
    ser = serial.Serial(port, baudrate=115200, timeout=0.2)

    def disable():
        print("Exiting tovjoystick")
        ser.write(b'*#0')
        time.sleep(0.5)
        ser.reset_input_buffer()
        ser.close()
        if startVJoy:
            os.system(r'"c:\Program Files\vJoy\x64\vJoyConfig" enable off')
    
    if startVJoy:
        print("Enabling vjoystick")
        os.system(r'"c:\Program Files\vJoy\x64\vJoyConfig" 2 -f -a X Y RZ RY Sl0 Sl1 -b 16 -p 2')
#        os.system(r'"c:\Program Files\vJoy\x64\vJoyConfig" 2 -f -a Sl0 Sl1 -b 16')
        os.system(r'"c:\Program Files\vJoy\x64\vJoyConfig" enable on')
        atexit.register(disable)
    
    print("Running")

    joy = pyvjoy.VJoyDevice(2)
    processGameCubeController(ser, delay, lambda s0,s1: map.toVJoystick(joy, s1))
