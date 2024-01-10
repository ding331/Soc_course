import socket

import cv2
import numpy as np

IM_X = 720
IM_Y = 480
IM_CHANNEL = 3
HEADER_BYTES = 4
BODY_BYTES = IM_X * 3
pac_indexKAGESIZE = BODY_BYTES + HEADER_BYTES
page = 1


def main():
    # Create a UDP socket
    global pixel, cur_line, page
    image_array = np.zeros((IM_Y, IM_X, IM_CHANNEL), np.uint8)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 65536)
    sock.setblocking(False)

    port = 7
    ip = '192.168.1.77'
    sock.bind((ip, port))
    win_name = "UDP Video"
    pac_index = 0
    lwip_cnt = 0

    while True:
        try:
            data = sock.recv(65535)  # 2264)  #1+720*3+100  2261
            lwip_cnt += 1

        except:
            continue
        print(data, "\n")

        for pac_index in range(24):                   #0~4, 724~728,        ***
            cur_line = int.from_bytes(data[724 * pac_index: 4 + 724 * pac_index], 'little', signed=False)
            # cur_line = int.from_bytes(data, 'little', signed=False)
            print("cur_line :　", cur_line, "\n")


            # Receive one line rgb data, update image_array
            if cur_line < 0 or cur_line >= IM_Y:
                continue
            # for i in range(4, 4+720*3, 4):
            for col in range(IM_X):

                color_location = HEADER_BYTES + col * IM_CHANNEL
                for i in range(3):      #4~724, 728~1428                                             ***
                    pixel = int.from_bytes(data[4 + 724 *pac_index + col: 4 + 724 *pac_index + col + 1], 'little', signed=False)
                    image_array[cur_line][col][i] = pixel  # [color_location]
                    # image_array[cur_line][col][1] = pixel[color_location + 1]
                    # image_array[cur_line][col][0] = pixel[color_location + 2]

                    # print("arr[", col, "][", i, "] :　", pixel, "\n")
                # If current line == endline update image.
                # print("cur_line :　", cur_line, "\n")

                # print("col :　", col, "\n")
            print("last pixel:　", pixel, "\n")

        # 切割成每 10 bits 一個 int 並印出
        # start_index = 2164
        # end_index = 2264
        # bit_string = ''.join(format(byte, '08b') for byte in data[2164:2264])
        # bits = [int(bit_string[i:i + 10], 2) for i in range(0, len(bit_string), 10)]
        # for value in bits:
        #     if value != 0:
        #         print(value)

        if cur_line == IM_Y - 1:

            cv2.imshow(win_name, image_array)
            key = cv2.waitKey(1)

            if key == ord('q'):
                break
            cv2.imwrite(f'SD_image_{page}.bmp', image_array)
            page = page + 1
        print("total shake :　", lwip_cnt, "\n")
    print("The client is quitting.")
    # sock.close()
    # cv2.destroyAllWindows()


if __name__ == '__main__':
    main()
