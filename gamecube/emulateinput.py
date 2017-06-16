from input import *
import time
import threading
import atexit
import numbers

class KeyRepeatThread(threading.Thread):
    def __init__(self):
        super(KeyRepeatThread, self).__init__()
        self.repeatingKey = None
        self.timePressed = None
        self.repeatDelay = 0.4
        self.repeatTime = 1./30
        self.stopped = False
        self.stoprequest = threading.Event()

    def press(self, key, repeat=True):
        if key == LBUTTON or key == RBUTTON or key == MBUTTON or not isinstance(key,numbers.Number):
            repeat=False
        self.repeatingKey = key if repeat else None
        self.timePressed = time.time()
        pressKey(key)

    def release(self, key):
        self.repeatingKey = None
        releaseKey(key)

    def join(self):
        self.stoprequest.set()
        super(KeyRepeatThread, self).join()

    def run(self):
        while not self.stoprequest.isSet():
            if self.repeatingKey is not None:
                if time.time() >= self.timePressed + self.repeatDelay:
                    releaseKey(self.repeatingKey)
                    pressKey(self.repeatingKey)
            time.sleep(self.repeatTime)

keyRepeatThread = None

def emulatePress(key, repeat=True):
    global keyRepeatThread
    if keyRepeatThread is None:
        keyRepeatThread = KeyRepeatThread()
        keyRepeatThread.start()
        atexit.register(keyRepeatThread.join)
    keyRepeatThread.press(key, repeat=repeat)

def emulateRelease(key):
    keyRepeatThread.release(key)
    
def stopKeyRepeat():
    keyRepeatThread.stop()
