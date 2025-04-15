import cv2
import time
import numpy as np
if __name__ == "__main__":
    device_index = 1 #make sure this device is the boson (it will show black if incorrect)
    cap = cv2.VideoCapture(device_index + cv2.CAP_DSHOW)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 512) #use 256 for Boson 320
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640) #use 320 for Boson 320
    cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter.fourcc('Y', '1', '6', ' '))
    cap.set(cv2.CAP_PROP_CONVERT_RGB, 0)
    frame_buf = []
    start = time.time()
    stream_ret, frame = cap.read()
    if stream_ret: 
        #convert frame to 8-bit for contrast (frame1=8-bit) (frame=16-bit)
        if frame.dtype == np.uint16:
            min_val, max_val = np.min(frame), np.max(frame)
            frame1 = ((frame - min_val) / (max_val - min_val) * 255).astype(np.uint8)
    print(f"Read Time + decode: {time.time() - start}")
    cv2.imshow("image", frame1)
    cv2.waitKey(0)