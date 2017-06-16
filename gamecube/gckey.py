import serial
import sys
import time
import emulateinput
import input
import numbers
from gamecube import *

DEBUG = True

maps = {
    "a-ctrl":
        { "a":input.CONTROL, "b":input.SPACE, "dleft":input.LEFT, "dright":input.RIGHT, "dup":input.UP, "ddown":input.DOWN, "z":input.KEY_MINUS, "start":input.KEY_PLUS },
    "arrow":
        { "a":input.SPACE, "b":input.BACK, "dLeft":input.LEFT, "dRight":input.RIGHT, "dUp":input.UP, "dDown":input.DOWN, "z":input.KEY_MINUS, "start":input.KEY_PLUS },
    "mc":
        { "a":input.SPACE, "b":input.LSHIFT, "dLeft":(input.MOUSE_RELATIVE,-50,0), "dRight":(input.MOUSE_RELATIVE,50,0), "dUp":input.KEY_W, "dDown":input.KEY_S, 
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

ser = serial.Serial(port, baudrate=115200, timeout=0.2)
print("Connected")
processGameCubeController(ser, delay, lambda s0,s1: processDifferences(s0, s1, map))
