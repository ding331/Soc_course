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
    image_array = np.zeros((IM_X, IM_Y, IM_CHANNEL), np.uint8)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 65536)
    sock.setblocking(0)

    port = 7
    ip = '192.168.1.77'
    sock.bind((ip, port))
    win_name = "UDP Video"

    while True:
        try:
            data = sock.recv(2264)#2261)  #1+720*3+100  2261
        except:
            continue
        print(data, "\n")

        cur_line = int.from_bytes(data[0:HEADER_BYTES ], 'little', signed=False)
        # cur_line = int.from_bytes(data, 'little', signed=False)
        print("cur_line :　", cur_line, "\n")
        # Receive one line rgb data, update image_array
        if cur_line < 0 or cur_line >= IM_Y:
            continue
        # for i in range(4, 4+720*3, 4):
        for col in range(IM_X):

            color_location = HEADER_BYTES + col * IM_CHANNEL
            for i in range(3):
                pixel = int.from_bytes(data[4+col*3+i:4+col*3+i +1], 'little', signed=False)
                image_array[col][cur_line][i] = pixel#[color_location]
            # image_array[col][cur_line][1] = pixel[color_location + 1]
            # image_array[col][cur_line][0] = pixel[color_location + 2]

                print("arr[col][", i, "] :　", pixel, "\n")
            # If current line == endline update image.
            print("cur_line :　", cur_line, "\n")

            if cur_line == IM_Y - 1:

                cv2.imshow(win_name, image_array)
                key = cv2.waitKey(1)

                if key == ord('q'):
                    break

            # # 10-bits chunks from data[HEADER_BYTES + 720 * 480:]


            # modified_data = data[HEADER_BYTES + 720*3: ]
            # # Use for loop to scan modified_data if != 0 then print(value of data[])
            # for i in range(0, len(modified_data), 4):
            #     value = int.from_bytes(modified_data[i:i + 4], 'little', signed=False)
            #     if value != 0:
            #         print(f"Value at index {i}: {value}")

        print("col :　", col, "\n")

    print("The client is quitting.")
    # sock.close()
    # cv2.destroyAllWindows()

if __name__ == '__main__':
    main()
