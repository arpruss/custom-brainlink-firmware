import pyvjoy
import math
from gamecube import *
import time

DEBUG = True
GC_BUTTONS = ("a", "b", "x", "y", "z", "l", "r")

def toVJoystick(joy, state, buttons=GC_BUTTONS):
    joy.data.lButtons = sum(1<<i for i in range(len(buttons)) if getattr(state, buttons[i]))
    joy.data.wAxisX = state.joyx-128
    joy.data.wAxisY = 128-state.joyy
    joy.data.wSlider = state.rightbutton
    joy.data.wDial = state.leftbutton
    joy.update()
    
    rid = joy.rID
    
    vector = [0,0]
    if state.dup:
        vector[1] += 1
    elif state.dright:
        vector[0] += 1
    elif state.ddown:
        vector[1] -= 1
    elif state.dleft:
        vector[0] -= 1
        
    if vector[0] == 0 and vector[1] == 0:
        pov1 = -1
    else:
        pov1 = (100*int(math.floor(0.5+180/math.pi * math.atan2(vector[1],vector[0])))) % 35999
        
    joy._sdk.SetContPov(pov1,rid,1)
    
    pov2 = -1
    
    px = state.cx - 127.5
    py = state.cy - 127.5
    if px*px + py*py >= 10*10:
        pov2 = int(math.floor(0.5+100*180/math.pi * math.atan2(py,px))) % 35999

        
    joy._sdk.SetContPov(pov2,rid,2)
    
if __name__ == '__main__':
    import sys
    import serial
    import os
    import atexit
    
    def err(message):
        if DEBUG: print(message)

    port = "com3"    
    maps = {}
    delay = 6

    for item in sys.argv[1:]:
        if item in maps:
            map = maps[item]
        else:
            try:
                delay = int(item)
            except:
                port = item
                print (port)
        
    command = b'**#1c0'+bytes(bytearray([delay]))+b'0'

    print("Connecting to serial")
    ser = serial.Serial(port, baudrate=115200, timeout=0.1)

    def disable():
        print("Exiting tovjoystick")
        ser.close()
        os.system(r'"c:\Program Files\vJoy\x64\vJoyConfig" enable off')
    
    print("Enabling vjoystick")
    os.system(r'"c:\Program Files\vJoy\x64\vJoyConfig" 2 -f -a X Y     Sl0 Sl1 -b 16 -p 2')
    os.system(r'"c:\Program Files\vJoy\x64\vJoyConfig" enable on')
    atexit.register(disable)
    
    print("Running")

    joy = pyvjoy.VJoyDevice(2)
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
                err("Error 1")
                break
            elif c == b'G':
                data = ser.read(8)
                if len(data) == 8:
                    newState = GameCubeControllerState(data)
                    toVJoystick(joy, newState)
                    count += 1
                    if DEBUG and count%1000 == 0: print("freq %f"%(count/(time.time()-startt)))
                else:
                    err("Error 2")
                    ser.write(command)
                    ser.reset_input_buffer()
                break
        if time.time() - t0 >= 1:
            err("Timeout")
            ser.write(command)
            ser.reset_input_buffer()
