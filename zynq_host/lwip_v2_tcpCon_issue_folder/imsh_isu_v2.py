
import socket
import cv2
import numpy as np
from PIL import Image

IM_X = 720
IM_Y = 480
IM_CHANNEL = 3
HEADER_BYTES = 4
BODY_BYTES = IM_X * 3
PACKAGESIZE = BODY_BYTES + HEADER_BYTES
def main():
    # Create a UDP socket
    image_array = np.zeros((IM_Y, IM_X, IM_CHANNEL), np.uint8)  # 翻轉IM_X和IM_Y以匹配圖像維度
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 65536)
    sock.setblocking(0)

    port = 7
    ip = '192.168.1.77'
    sock.bind((ip, port))
    win_name = "UDP Video"

    while True:
        try:
            data, _ = sock.recvfrom(PACKAGESIZE)
            # 從接收到的二進位數據中解碼圖像數據
            image_array = np.frombuffer(data[HEADER_BYTES:], dtype=np.uint8).reshape((IM_Y, IM_X, IM_CHANNEL))
            # 顯示圖像
            cv2.imshow(win_name, image_array)
            cv2.waitKey(1)  # 等待1毫秒，持續顯示圖像
        except:
            continue

if __name__ == "__main__":
    main()

