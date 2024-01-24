import socket
import cv2
import numpy as np
import csv
from PIL import Image
import struct

IM_X = 720
IM_Y = 480
IM_CHANNEL = 3
HEADER_BYTES = 4
BODY_BYTES = IM_X * 3
PACKAGESIZE = BODY_BYTES + HEADER_BYTES
page = 2


def main(lwip_cnt=None):
    # Create a UDP socket
    global cur_line, pixel, page
    image_array = np.zeros((IM_Y, IM_X, IM_CHANNEL), np.uint8)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 65536)
    sock.setblocking(False)

    port = 7
    ip = '192.168.1.77'
    sock.bind((ip, port))
    win_name = "UDP Video"
    row = 0
    lwip_cnt = 0
    last_lwip_cnt = 0

    csv_file = open('optical_flow_data.csv', mode='w', newline='')
    csv_writer = csv.writer(csv_file)
    csv_writer.writerow(['X_prev', 'Y_prev', 'page'])

    while True:
        try:
            data = sock.recv(65535)
            lwip_cnt += 1
        except:
            continue
        # print(data, "\n")

        # Splitting data into 24 rows
        if lwip_cnt != last_lwip_cnt:
            for row in range(24):
                cur_line = int.from_bytes(data[row * 2264: row * 2264 + 4], 'little', signed=False)
                if cur_line < 0 or cur_line >= IM_Y:
                    continue
                # print("cur_line :　", cur_line, "\n")

                for col in range(IM_X):
                    color_location = HEADER_BYTES + col * IM_CHANNEL
                    for i in range(IM_CHANNEL):
                        pixel = int.from_bytes(data[4 + (row * 2264) + (col * 3) + i : 4 + (row * 2264) + (col * 3) + i + 1],
                                               'little', signed=False)
                        image_array[cur_line][col][i] = pixel
                        # if pixel != 0:  # b_out test
                        #     # print("!=0 pixel:　", pixel, "\n")
                        #     print(f"page: {page}, pixel !=0 Address: {col , cur_line}, Byte: {pixel:02X}, Hex: {hex(pixel)}")

                # print("last pixel:　", pixel, "\n")

            # optical_flow_data.csv
                match_pos = data[row * 2264 - 100 : row * 2264]

                # 拆分成40个20位的子部分
                for i in range(40):
                    start_bit = i * 20
                    end_bit = (i + 1) * 20
                    value_20bits = int.from_bytes(match_pos[start_bit // 8 : end_bit // 8], byteorder='big', signed=False)

                    # 如果20位的值不为零
                    if value_20bits != 0:
                        # 拆分成两个10位的值
                        value_10bit_1 = (value_20bits >> 10) & 0x3FF
                        value_10bit_2 = value_20bits & 0x3FF

                        # 写入CSV文件
                        csv_writer.writerow([value_10bit_1, value_10bit_2])
                # bits = ''.join(format(byte, '08b') for byte in data[row * 2264 - 100 : row * 2264])
                # # int_values = [int(bits[i:i + 10], 2) for i in range(0, len(bits), 10)]
                # # for value in int_values:
                #     # print(value)
                # for i in range(0, len(bits), 20):
                #     int_values = [int(bits[i:i + 10]), int(bits[i +10:i + 20])] 
                # #
                #     if int_values != 0:
                #         csv_writer.writerow(int_values)                    
                   
                if cur_line == IM_Y - 1:
                    cv2.imwrite(f'/img/PL_ORB_thres2_{page}.bmp', image_array)
                    
                    page = page + 1
            print("total shake :　", lwip_cnt, "\n")
            last_lwip_cnt = lwip_cnt  # 更新 last_lwip_cnt

    print("The client is quitting.")
    # csv_file.close()
    # sock.close()
    # cv2.destroyAllWindows()


if __name__ == '__main__':
    main()
