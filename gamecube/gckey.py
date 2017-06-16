import serial
import sys
import time
import emulateinput
import input
import numbers
from gamecube import GameCubeControllerState

DEBUG = True

maps = {
    "a-ctrl":
        { "a":input.CONTROL, "b":input.SPACE, "dleft":input.LEFT, "dright":input.RIGHT, "dup":input.UP, "ddown":input.DOWN, "z":input.KEY_MINUS, "start":input.KEY_PLUS },
    "arrow":
        { "a":input.SPACE, "b":input.BACK, "dleft":input.LEFT, "dright":input.RIGHT, "dup":input.UP, "ddown":input.DOWN, "z":input.KEY_MINUS, "start":input.KEY_PLUS },
    "mcpad":
        { "a":input.SPACE, "b":input.LSHIFT, "dleft":(input.MOUSE_RELATIVE,-50,0), "dright":(input.MOUSE_RELATIVE,50,0), "dup":input.KEY_W, "ddown":input.KEY_S, 
            "z":input.LBUTTON, "start":input.RBUTTON}
    }

def processDifferences(oldState, newState, map):
    for key in map:
        if getattr(newState, key) and not getattr(oldState, key):
            if DEBUG: print("press "+key)
            emulateinput.emulatePress(map[key])
        elif not getattr(newState, key) and getattr(oldState, key):
            emulateinput.emulateRelease(map[key])
            
def err(message):
    if DEBUG: print(message)

port = "com3"    
map = maps["arrow"]   
delay = 5

for item in sys.argv[1:]:
    if item in maps:
        map = maps[item]
    else:
        try:
            delay = int(item)
        except:
            port = item
    
command = b'**#1c0'+bytes(bytearray([delay]))+b'0'

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
                processDifferences(lastState, newState, map)
                lastState = newState
                count += 1
                if DEBUG and count%1000 == 0: print("freq %f"%(count/(time.time()-startt)))
            else:
                err("Error")
                ser.write(command)
                ser.reset_input_buffer()
            break
    if time.time() - t0 >= 1:
        print("Timeout")
        ser.write(command)
        ser.reset_input_buffer()
        