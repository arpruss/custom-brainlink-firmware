import pyvjoy
import math
from gamecube import *
import sysinput
import time
import emulateinput
from builtins import input

DEBUG = True
        
class GameCubeToDesktop(object):
    def __init__(self, buttonsToVJoystick=None, buttonsToDesktop=None, combineShoulders = False, threshold=1):
        self.buttonsToVJoystick = buttonsToVJoystick
        self.buttonsToDesktop = buttonsToDesktop
        self.combineShoulders = combineShoulders
        self.threshold = threshold
        self.useVJoystick = buttonsToVJoystick is not None
        self.previous = GameCubeControllerState()
        
    def buttonActive(self, state, button):
        value = getattr(state, button)
        if value == True or value == False:
            return value
        return value >= self.threshold
        
    def setVJoy(self, joy):
        self.joy = joy
        self.useVJoystick = True
      
    @staticmethod
    def remapAxis(value):
        v1 = math.floor(value*32767./255+1.5)
        return min(max(int(v1),1),32768)
        
    def process(self, state):
        if self.useVJoystick:
            self.toVJoystick(state)
        if self.buttonsToDesktop:
            self.processButtonDifferences(state)
            
    def processButtonDifferences(self, newState):
        for key in self.buttonsToDesktop:
            if getattr(newState, key) and not getattr(self.previous, key):
                if DEBUG: print("press "+key)
                emulateinput.emulatePress(self.buttonsToDesktop[key])
            elif not getattr(newState, key) and getattr(self.previous, key):
                emulateinput.emulateRelease(self.buttonsToDesktop[key])
        self.previous = newState
                            
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
        
maps = {
    "default": GameCubeToDesktop(buttonsToVJoystick=GC_BUTTONS),
    "jetset": GameCubeToDesktop(buttonsToVJoystick=("a", "b", "y", "z", "x", "shoulderRight", "shoulderLeft", "start")),
    "arrow-ctrl":
        GameCubeToDesktop(buttonsToDesktop={ "a":sysinput.CONTROL, "b":sysinput.SPACE, "dleft":sysinput.LEFT, "dright":sysinput.RIGHT, "dup":sysinput.UP, "ddown":sysinput.DOWN, "z":sysinput.KEY_MINUS, "start":sysinput.KEY_PLUS }),
    "arrow":
        GameCubeToDesktop(buttonsToDesktop={ "a":sysinput.SPACE, "b":sysinput.BACK, "dLeft":sysinput.LEFT, "dRight":sysinput.RIGHT, "dUp":sysinput.UP, "dDown":sysinput.DOWN, "z":sysinput.KEY_MINUS, "start":sysinput.KEY_PLUS }),
    "mc":
        GameCubeToDesktop(buttonsToDesktop={ "a":sysinput.SPACE, "b":sysinput.LSHIFT, "dLeft":(sysinput.MOUSE_RELATIVE,-50,0), "dRight":(sysinput.MOUSE_RELATIVE,50,0), "dUp":sysinput.KEY_W, "dDown":sysinput.KEY_S, 
            "z":sysinput.LBUTTON, "start":sysinput.RBUTTON}),
    "wasd":
        GameCubeToDesktop(buttonsToDesktop={ "a":sysinput.SPACE, "b":sysinput.LSHIFT, "dLeft":sysinput.KEY_A, "dRight":sysinput.KEY_D, "dUp":sysinput.KEY_W, "dDown":sysinput.KEY_S}),
    "qbert":
        GameCubeToDesktop(buttonsToDesktop={ "a":sysinput.KEY_1, "b":sysinput.KEY_2, "dLeft":sysinput.LEFT, "dRight":sysinput.RIGHT, "dUp":sysinput.UP, "dDown":sysinput.DOWN, 
          "z":sysinput.KEY_MINUS, "start":sysinput.KEY_5 }),
    }
    
menu = ( ("Default mapping (vJoystick)", maps["default"]),
         ("Arrow keys with A=CTRL, B=SPACE", maps["arrow-ctrl"]),
         ("Arrow keys with A=SPACE, B=BACKSPACE", maps["arrow"]),
         ("DPad=WASD", maps["wasd"]),
         ("Jet Set Radio (vJoystick)", maps["jetset"]),
         ("Minecraft", maps["mc"]),
         ("QBert (archive.org)", maps["qbert"]) )
            
def getFromMenu(menu):
    while True:
        for i,item in enumerate(menu):
            print("%d. %s" % (i,item[0]))
        selection = input("Choose:")
        if selection == "":
            print(menu[0][0])
            return menu[0][1]
        try:
            print(menu[int(selection)][0])
            return menu[int(selection)][1]
        except:
            print("Invalid")               
    
if __name__ == '__main__':
    import sys
    import serial
    import serial.tools.list_ports
    import os
    import atexit
    
    def err(message):
        if DEBUG: print(message)

    port = None
    map = None
    delay = 5
    startVJoy = True
    
    for item in sys.argv[1:]:
        if item == "menu":
            map = getFromMenu(menu)
        elif item in maps:
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
                
    if map is None:
        map = getFromMenu(menu)
                
    if port is None:
        port = sorted(a.device for a in serial.tools.list_ports.comports())[0]
        
    print("Connecting to serial on "+port)
    ser = serial.Serial(port, baudrate=115200, timeout=0.2)
    
    if not map.useVJoystick:
        startVJoy = False

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

    if map.useVJoystick:
        joy = pyvjoy.VJoyDevice(2)
        map.setVJoy(joy)
    processGameCubeController(ser, delay, map.process)
