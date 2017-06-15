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
    
    pov1 = -1
    if state.dup:
        pov1 = 0
    elif state.dright:
        pov1 = 1
    elif state.ddown:
        pov1 = 2
    elif state.dleft:
        pov1 = 3
        
    joy._sdk.SetDiscPov(pov1,rid,1)
    
    pov2 = -1
    
    px = state.cx - 127.5
    py = state.cy - 127.5
    if px*px + py*py >= 10*10:
        pov2 = 180/math.pi * math.atan2(py,px)
        
    joy._sdk.SetContPov(pov2,rid,2)
    
if __name__ == '__main__':
    import sys
    import serial
    
    def err(message):
        if DEBUG: print(message)

    port = "com3"    
    maps = {}
    delay = 5

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

    joy = pyvjoy.VJoyDevice(1)
    ser = serial.Serial(port, baudrate=115200, timeout=0.1)
    print("Connected")
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
                err("Error")
                break
            elif c == b'G':
                data = ser.read(8)
                if len(data) == 8:
                    newState = GameCubeControllerState(data)
                    toVJoystick(joy, newState)
                    count += 1
                    if DEBUG and count%1000 == 0: print("freq %f"%(count/(time.time()-startt)))
                else:
                    err("Error")
                    ser.write(command)
                    ser.reset_input_buffer()
                break
        if time.time() - t0 >= 1:
            err("Timeout")
            ser.write(command)
            ser.reset_input_buffer()
