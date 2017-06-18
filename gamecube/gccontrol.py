import pyvjoy
import math
from gamecube import *
import input
import time

DEBUG = True
        
class GameCubeToDesktop(object):
    def __init__(self, buttonsToVJoystick=None, buttonsToDesktop=None, combineShoulders = False, threshold=1):
        self.buttonsToVJoystick = buttonsToVJoystick
        self.combineShoulders = combineShoulders
        self.threshold = threshold
        self.vJoystick = buttonsToVJoystick is not None
        self.previous = GameCubeControllerState()
        
    def buttonActive(self, state, button):
        value = getattr(state, button)
        if value == True or value == False:
            return value
        return value >= self.threshold
        
    def setVJoystick(self, joy):
        self.joy = joy
        self.vJoystick = True
      
    @staticmethod
    def remapAxis(value):
        v1 = math.floor(value*32767./255+1.5)
        return min(max(int(v1),1),32768)
        
    def process(self, state):
        if self.vJoystick:
            self.toVJoystick(state)
        if self.buttonsToDesktop:
            self.processButtonDifferences(state)
            
    def processButtonDifferences(self, newState):
        for key in self.buttonsToDesktop:
            if getattr(newState, key) and not getattr(self.previous, key):
                if DEBUG: print("press "+key)
                emulateinput.emulatePress(self.buttonsToDesktop[key])
            elif not getattr(newState, key) and getattr(self.previous, self.buttonsToDesktop):
                emulateinput.emulateRelease(self.buttonsToDesktop[key])
                            
    def toVJoystick(self, state):
        self.joy.data.lButtons = sum(1<<i for i in range(len(self.buttonsToVJoystick)) if self.buttonActive(state, self.buttonsToVJoystick[i]))
        self.joy.data.wAxisX = GameCubeToDesktop.remapAxis(state.joyX)
        self.joy.data.wAxisY = GameCubeToDesktop.remapAxis(255-state.joyY)
        self.joy.data.wAxisZRot = GameCubeToDesktop.remapAxis(state.cX)
        self.joy.data.wAxisYRot = GameCubeToDesktop.remapAxis(255-state.cY)
        if self.combineShoulders:
            self.joy.data.wSlider = GameCubeToDesktop.remapAxis((255+state.shoulderLeft - state.shoulderRight)/2)
        else:
            self.joy.data.wSlider = GameCubeToDesktop.remapAxis(255-state.shoulderRight)
        self.joy.data.wDial = GameCubeToDesktop.remapAxis(255-state.shoulderLeft)
        self.joy.update()
        
        rid = self.joy.rID
        
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
            
        self.joy._sdk.SetContPov(pov1,rid,1)
        
        pov2 = -1
        px = state.cX - 128
        py = state.cY - 128
        if px*px + py*py >= 10*10:
            pov2 = (-9000+int(math.floor(0.5+100*180/math.pi * math.atan2(py,-px)))) % 36000
            
        self.joy._sdk.SetContPov(pov2,rid,2)
        
GC_BUTTONS = ("a", "b", "x", "y", "z", "start", "l", "r")
PAD_OPTIONS_DEFAULT = GameCubeToDesktop(buttonsToVJoystick=GC_BUTTONS)
        
maps = {
    "normal": PAD_OPTIONS_DEFAULT,
    "jetset": GameCubeToDesktop(buttonsToVJoystick=("a", "b", "y", "z", "x", "shoulderRight", "shoulderLeft", "start")),
    "a-ctrl":
        GameCubeToDesktop(buttonsToDesktop={ "a":input.CONTROL, "b":input.SPACE, "dleft":input.LEFT, "dright":input.RIGHT, "dup":input.UP, "ddown":input.DOWN, "z":input.KEY_MINUS, "start":input.KEY_PLUS }),
    "arrow":
        GameCubeToDesktop(buttonsToDesktop={ "a":input.SPACE, "b":input.BACK, "dLeft":input.LEFT, "dRight":input.RIGHT, "dUp":input.UP, "dDown":input.DOWN, "z":input.KEY_MINUS, "start":input.KEY_PLUS }),
    "mc":
        GameCubeToDesktop(buttonsToDesktop={ "a":input.SPACE, "b":input.LSHIFT, "dLeft":(input.MOUSE_RELATIVE,-50,0), "dRight":(input.MOUSE_RELATIVE,50,0), "dUp":input.KEY_W, "dDown":input.KEY_S, 
            "z":input.LBUTTON, "start":input.RBUTTON}),
    "wasd":
        GameCubeToDesktop(buttonsToDesktop={ "a":input.SPACE, "b":input.LSHIFT, "dLeft":input.KEY_A, "dRight":input.KEY_D, "dUp":input.KEY_W, "dDown":input.KEY_S}),
    "qbert":
        GameCubeToDesktop(buttonsToDesktop={ "a":input.KEY_1, "b":input.KEY_2, "dLeft":input.LEFT, "dRight":input.RIGHT, "dUp":input.UP, "dDown":input.DOWN, 
          "z":input.KEY_MINUS, "start":input.KEY_5 }),
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
        elif item == "no-vjoy-start":
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
        if startVJoy and map.vJoystick:
            os.system(r'"c:\Program Files\vJoy\x64\vJoyConfig" enable off')
    
    if startVJoy and map.vJoystick:
        print("Enabling vjoystick")
        os.system(r'"c:\Program Files\vJoy\x64\vJoyConfig" 2 -f -a X Y RZ RY Sl0 Sl1 -b 16 -p 2')
#        os.system(r'"c:\Program Files\vJoy\x64\vJoyConfig" 2 -f -a Sl0 Sl1 -b 16')
        os.system(r'"c:\Program Files\vJoy\x64\vJoyConfig" enable on')
        atexit.register(disable)
    
    print("Running")

    if map.vJoystick:
        joy = pyvjoy.VJoyDevice(2)
        map.setVJoystick(joy)
    processGameCubeController(ser, delay, map.process)
