import serial
import sys
import time
import emulateinput
import input
from gamecube import GameCubeControllerState

arrowMap = { "a":input.SPACE, "b":input.BACK, "dleft":input.LEFT, "dright":input.RIGHT, "dup":input.UP, "ddown":input.DOWN }

def processDifferences(oldState, newState, map):
    for key in map:
        if getattr(newState, key) and not getattr(oldState, key):
            emulateinput.emulatePress(map[key])
        elif not getattr(newState, key) and getattr(oldState, key):
            emulateinput.emulateRelease(map[key])

ser = serial.Serial("com3", baudrate=115200, timeout=0.1)
ser.write(b'*#1')

lastState = GameCubeControllerState()

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
            data = ser.read(8)
            if len(data) == 8:
                newState = GameCubeControllerState(data)
                processDifferences(lastState, newState, arrowMap)
                lastState = newState
            else:
                print("Error")
            break
    if time.time() - t0 >= 1:
        print("Timeout")
    time.sleep(0.5)
