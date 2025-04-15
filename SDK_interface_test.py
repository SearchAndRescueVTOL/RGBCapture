from SDK_USER_PERMISSIONS.SDK_USER_PERMISSIONS.ClientFiles_Python.EnumTypes import *
from SDK_USER_PERMISSIONS.SDK_USER_PERMISSIONS.ClientFiles_Python.Client_API import *
from SDK_USER_PERMISSIONS.SDK_USER_PERMISSIONS.ClientFiles_Python.Serializer_Struct import *
import time
import os
import cv2
import numpy as np
class IntegerVal():
    def __init__(self, number):
        self.value = number
if __name__ == "__main__":
    myCam = pyClient(manualport="COM5") #Boson COM port on windows, check device manager
    myCam.bosonSetGainMode(FLR_BOSON_GAINMODE_E.FLR_BOSON_HIGH_GAIN) #Low gain mode also available if temp > 120C ***
    # myCam.TLinearSetControl(FLR_ENABLE_E.FLR_ENABLE)
    # myCam.radiometrySetTransmissionWindow(1.00) #100% transmission (no window in front of Boson)
    # myCam.TLinearRefreshLUT(FLR_BOSON_GAINMODE_E.FLR_BOSON_HIGH_GAIN) #necessary after setting radiometry parameters like window transmission
    myCam.bosonRunFFC() # Run Fast Field Correction for gain correction/calibration
    data = IntegerVal(1)
    result = myCam.telemetrySetState(data) # enable telemetry
    if result.value:
        print(f"error code: {result.value}")
    result, data = myCam.telemetryGetState()
    if result.value:
        print(f"error code: {result.value}")
    print(f"Data: {data}")
    data = IntegerVal(0)
    result = myCam.telemetrySetLocation(data)
    if result.value:
        print(f"error code: {result.value}")
    result,data = myCam.telemetryGetLocation()
    if result.value:
        print(f"error code: {result.value}")
    print(f"Data2: {data}")
    data = IntegerVal(1)
    result = myCam.telemetrySetPacking(data)
    result,data = myCam.telemetryGetPacking()
    if result.value:
        print(f"error codeuser{result.value}")
    print(f"Data3: {data}")
    # result = myCam.captureSingleFrame()
    # if result.value:
    #     print(f"Error code: {result.value}")
    buffer = 0
    offset = 0
    size = 256
    result, data = myCam.memReadCapture(buffer, offset, size)
    if result.value:
        print(f"Error code: {result.value}")
    print(data)
    # print(res.value)
