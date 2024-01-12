import socket
import cv2
import numpy as np
import csv
from PIL import Image

IM_X = 720
IM_Y = 480
IM_CHANNEL = 3
HEADER_BYTES = 4
BODY_BYTES = IM_X * 3
PACKAGESIZE = BODY_BYTES + HEADER_BYTES


def read_image(page):
    image_path = f'SD_image_{page}.bmp'
    img = cv2.imread(image_path)
    return img


def sparse_optical_flow(prev_img, next_img):
    prev_gray = cv2.cvtColor(prev_img, cv2.COLOR_BGR2GRAY)
    next_gray = cv2.cvtColor(next_img, cv2.COLOR_BGR2GRAY)

    feature_params = dict(maxCorners=100,
                          qualityLevel=0.3,
                          minDistance=7,
                          blockSize=7)

    prev_pts = cv2.goodFeaturesToTrack(prev_gray, mask=None, **feature_params)

    lk_params = dict(winSize=(15, 15),
                     maxLevel=2,
                     criteria=(cv2.TERM_CRITERIA_EPS | cv2.TERM_CRITERIA_COUNT, 10, 0.03))

    next_pts, status, error = cv2.calcOpticalFlowPyrLK(prev_gray, next_gray, prev_pts, None, **lk_params)

    good_prev = prev_pts[status == 1]
    good_next = next_pts[status == 1]

    return good_prev, good_next


def draw_optical_flow(image, prev_pts, next_pts):
    for i, (prev, next) in enumerate(zip(prev_pts, next_pts)):
        x_prev, y_prev = prev.ravel().astype(int)
        x_next, y_next = next.ravel().astype(int)
        cv2.line(image, (x_prev, y_prev), (x_next, y_next), (0, 255, 0), 2)
    return image


def main():
    # Create a UDP socket
    global last_lwip_cnt, lwip_cnt, max_page, pixel
    image_array = np.zeros((IM_Y, IM_X, IM_CHANNEL), np.uint8)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 65536)
    sock.setblocking(0)

    port = 7
    ip = '192.168.1.77'
    sock.bind((ip, port))
    win_name = "UDP Video"
    pac_index = 0
    lwip_cnt = 0
    last_lwip_cnt = 0
    max_page = 1

    csv_file = open('optical_flow_data.csv', mode='w', newline='')
    csv_writer = csv.writer(csv_file)
    csv_writer.writerow(['Page', 'X_prev', 'Y_prev', 'X_next', 'Y_next'])

    video_output = cv2.VideoWriter('optical_flow_output.mp4', cv2.VideoWriter_fourcc(*'mp4v'), 30, (720, 480))

    while True:
        try:
            data = sock.recv(65535)  # 2264)  #1+720*3+100  2261
            lwip_cnt += 1
        except:
            continue
        # print(data, "\n")

        if lwip_cnt != last_lwip_cnt:

            last_lwip_cnt = lwip_cnt  # 更新 last_lwip_cnt
            for pac_index in range(24):  # 0~4, 724~728,        ***
                cur_line = int.from_bytes(data[724 * pac_index: 4 + 724 * pac_index], 'little', signed=False)
                # cur_line = int.from_bytes(data, 'little', signed=False)
                # print("cur_line :　", cur_line, "\n")
                # Receive one line rgb data, update image_array
                if cur_line < 0 or cur_line >= IM_Y:
                    continue
                # for i in range(4, 4+720*3, 4):
                for col in range(IM_X):

                    color_location = HEADER_BYTES + col * IM_CHANNEL
                    for i in range(3):
                        pixel = int.from_bytes(data[4 + 724 * pac_index + col: 4 + 724 * pac_index + col + 1], 'little',
                                               signed=False)
                        image_array[cur_line][col][i] = pixel
                        # print("col :　", col, "\n")

                # print("last pixel:　", pixel, "\n")

                if cur_line == IM_Y - 1:
                    cv2.imwrite(f'SD_image_{max_page}.bmp', image_array)
                    if max_page == 1:
                        cv2.imwrite(f'optical_flow_output_{max_page}.bmp', image_array)  # 1st

                    # if (max_page+1) % 5 == 0 and max_page > 0:  #per 5 frame
                    if max_page > 1:  #=2
                        # for max_page in range(max_max_page-1, max_max_page+1):  #***
                        # for max_page in range(max_max_page-2, max_max_page+3):
                        print("max_page :　", max_page, "\n")
                        
                        prev_img = read_image(max_page-1)
                        next_img = read_image(max_page)

                        good_prev, good_next = sparse_optical_flow(prev_img, next_img)

                        optical_flow_img = np.zeros_like(prev_img)
                        optical_flow_img = draw_optical_flow(optical_flow_img, good_prev, good_next)

                        output_img = cv2.addWeighted(next_img, 1, optical_flow_img, 1, 0)

                        # Print and save optical flow data
                        for i, (prev, next) in enumerate(zip(good_prev, good_next)):
                            x_prev, y_prev = prev.ravel().astype(int)
                            x_next, y_next = next.ravel().astype(int)
                            print(
                                f"max_page: {max_page}, X_prev: {x_prev}, Y_prev: {y_prev}, X_next: {x_next}, Y_next: {y_next}")
                            csv_writer.writerow([max_page, x_prev, y_prev, x_next, y_next])

                        # 若偵測到page上限值mod 10 = 0，再刷新光流畫面、CSV檔案、撥放視頻
                        cv2.imwrite(f'optical_flow_output_{max_page}.bmp', output_img)  # 另存為每10張的光流畫面
                        video_output.write(output_img)  # 寫入視頻
                        # csv_file.close()  # 關閉舊的CSV檔案
                        # csv_file = open('optical_flow_data.csv', mode='w', newline='')  # 開啟新的CSV檔案
                        csv_writer = csv.writer(csv_file)
                        csv_writer.writerow(['max_page', 'X_prev', 'Y_prev', 'X_next', 'Y_next'])

                        prev_img = next_img

                    # if max_page == 60:  # 若保存完視頻大於60張，自動撥放
                    #     cap = cv2.VideoCapture('optical_flow_output.mp4')
                    #     while cap.isOpened():
                    #         ret, frame = cap.read()
                    #         if not ret:
                    #             break
                    #         cv2.imshow('Optical Flow Video', frame)
                    #         if cv2.waitKey(25) & 0xFF == ord('q'):
                    #             break

                    #     cap.release()
                    max_page = max_page + 1

                # csv_file.close()
                # video_output.release()

        print("total shake :　", lwip_cnt, "\n")

    print("The client is quitting.")
    # sock.close()
    # cv2.destroyAllWindows()


if __name__ == '__main__':
    main()
