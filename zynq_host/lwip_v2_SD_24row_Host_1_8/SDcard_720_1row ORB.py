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
page = 1

class ORBProcessor:
    def __init__(self):
        self.orb = cv2.ORB_create()

    def process(self, image):
        keypoints, descriptors = self.orb.detectAndCompute(image, None)
        # Return keypoints and descriptors for further processing
        return keypoints, descriptors

orb_processor = ORBProcessor()

def draw_matches(image, keypoints, position):
    for kp in keypoints:
        x, y = kp.pt
        cv2.circle(image, (int(x), int(y)), 5, (0, 255, 0), -1)
    cv2.imshow("Matches", image)
    print(f"Match found at position: {position}")

def main():
    # Create a UDP socket
    global page, front_page, descriptors_front
    image_array = np.zeros((IM_Y, IM_X, IM_CHANNEL), np.uint8)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 65536)
    sock.setblocking(0)

    port = 7
    ip = '192.168.1.77'
    sock.bind((ip, port))
    win_name = "UDP Video"

    while True:
        try:
            data = sock.recv(65535)  # 2264)  #1+720*3+100  2261
        except:
            continue
        print(data, "\n")

        cur_line = int.from_bytes(data[0:HEADER_BYTES], 'little', signed=False)
        # cur_line = int.from_bytes(data, 'little', signed=False)
        print("cur_line :　", cur_line, "\n")
        # Receive one line rgb data, update image_array
        if cur_line < 0 or cur_line >= IM_Y:
            continue
        # for i in range(4, 4+720*3, 4):
        for col in range(IM_X):

            color_location = HEADER_BYTES + col * IM_CHANNEL
            for i in range(3):
                pixel = int.from_bytes(data[4 + col: 4 + col + 1], 'little', signed=False)
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
            # image_array = cv2.cvtColor(image_array, cv2.COLOR_RGB2BGR)

            if page > 2:
                cv2.imwrite(f'SD_page_{page}.bmp', image_array)
                keypoints, descriptors = orb_processor.process(image_array)

                # ORB processing for Frontpage and backpage
                if page % 2 == 1:  # Frontpage
                    keypoints_front, descriptors_front = keypoints, descriptors
                else:  # Backpage
                    matches = cv2.BFMatcher(cv2.NORM_HAMMING, crossCheck=True)
                    keypoints_back, descriptors_back = keypoints, descriptors
                    if descriptors_front is not None and descriptors_back is not None:
                        matches = matches.match(descriptors_front, descriptors_back)
                        matches = sorted(matches, key=lambda x: x.distance)
                        if matches and matches[0].distance != 0:
                            draw_matches(image_array, keypoints_back, page)
                            # Do something else with the matches if needed

            page += 1

    print("The client is quitting.")
    # sock.close()
    # cv2.destroyAllWindows()


if __name__ == '__main__':
    main()
