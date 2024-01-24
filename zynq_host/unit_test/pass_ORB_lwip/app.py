import random
import socket
import threading
from flask import Flask, render_template, jsonify, request, send_file, Response
from io import BytesIO
import cv2
import numpy as np
import csv

app = Flask(__name__, static_url_path='', static_folder='static')

result_img_queue = []
'''
result_queue_example = [
    {
        "x": [0, 1, 2],
        "y": [0, 1, 2]
    },
]
'''
result_queue = []

IM_X = 720
IM_Y = 480
IM_CHANNEL = 3
HEADER_BYTES = 4
BODY_BYTES = IM_X * 3
PACKAGESIZE = BODY_BYTES + HEADER_BYTES


# frame = 1


@app.route('/', methods=['GET'])
def hello_world():
    if request.method != 'GET':
        response = jsonify({'message': 'Method not allowed'})
        response.status_code = 405
        return response

    return render_template('index.html')


@app.route('/image/<int:frame>', methods=['GET'])
def get_image(frame):
    if request.method != 'GET':
        response = jsonify({'message': 'Method not allowed'})
        response.status_code = 405
        return response

    if len(result_img_queue) == 0:
        response = jsonify({'message': 'No image in queue'})
        response.status_code = 404
        return response

    if frame > len(result_img_queue) - 1:
        response = jsonify({'message': 'Index out of range'})
        response.status_code = 404
        return response

    img_bytes = result_img_queue[frame]
    return send_file(BytesIO(img_bytes), mimetype='image/jpeg')


@app.route('/result/<int:frame>', methods=['GET'])
def get_result(frame):
    if request.method != 'GET':
        response = jsonify({'message': 'Method not allowed'})
        response.status_code = 405
        return response

    if len(result_queue) == 0:
        response = jsonify({'message': 'No result in queue'})
        response.status_code = 404
        return response

    if frame > len(result_queue) - 1:
        response = jsonify({'message': 'Index out of range'})
        response.status_code = 404
        return response

    response = jsonify(result_queue[frame])
    response.status_code = 200
    return response


def udp_socket_server(ip, port):
    global last_lwip_cnt, lwip_cnt, page, pixel
    image_array = np.zeros((IM_Y, IM_X, IM_CHANNEL), np.uint8)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 65536)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.setblocking(0)

    # port = 7
    # ip = '192.168.1.77'
    sock.bind((ip, port))
    win_name = "UDP Video"
    pac_index = 0
    lwip_cnt = 0
    last_lwip_cnt = 0
    # page = 0
    page = 1

    csv_file = open('optical_flow_data.csv', mode='w', newline='')
    csv_writer = csv.writer(csv_file)
    csv_writer.writerow(['X_prev', 'Y_prev'])

    video_output = cv2.VideoWriter(
        '../PL_ORB.mp4', cv2.VideoWriter_fourcc(*'mp4v'), 15, (720, 480))

    # while True:
    #     try:
    #         data = sock.recv(65535)  # 2264)  #1+720*3+100  2261
    #         lwip_cnt += 1
    #     except:
    #         continue
    #     # print(data, "\n")

    #     if lwip_cnt != last_lwip_cnt:

    #         last_lwip_cnt = lwip_cnt  # 更新 last_lwip_cnt
    #         for row in range(24):  # 0~4, 724~728,        ***
    #             cur_line = int.from_bytes(
    #                 data[724 * pac_index: 4 + 724 * pac_index], 'little', signed=False)
    #             # cur_line = int.from_bytes(data, 'little', signed=False)
    #             # print("cur_line :　", cur_line, "\n")
    #             # Receive one line rgb data, update image_array
    #             if cur_line < 0 or cur_line >= IM_Y:
    #                 continue
    #             # for i in range(4, 4+720*3, 4):
    #             for col in range(IM_X):

    #                 color_location = HEADER_BYTES + col * IM_CHANNEL
    #                 for i in range(3):
    #                     pixel = int.from_bytes(data[4 + (row * 2264) + (col * 3) + i : 4 + (row * 2264) + (col * 3) + i + 1],
    #                                            'little', signed=False)
    #                     image_array[cur_line][col][i] = pixel
    #                     # print("col :　", col, "\n")

    #             # print("last pixel:　", pixel, "\n")

    #             if cur_line == IM_Y - 1:
    #                 print("page : ", page)
    #                 cv2.imwrite(
    #                     f'static/img/PL_ORB_{page}.bmp', image_array)
    #                 video_output.write(image_array)

    #             # handle_image(image_array)
    #                 if 30 <= page <= 60:  # 若保存完視頻大於60張，自動撥放
    #                     video_output.release()

    #                     print("video_thread.start")
    #                     video_thread = threading.Thread(target=read_video)
    #                     video_thread.daemon = True
    #                     video_thread.start()
    #                 page += 1

    #             # video_output.release()

    #         print("total shake :　", lwip_cnt, "\n")
    #         last_lwip_cnt = lwip_cnt  # 更新 last_lwip_cnt
    for web_index in range(50):                              #ORB pixel latency
        video_output.write(cv2.imread(f'img/PL_ORB_thres2_{web_index + 30}.bmp'))
        #csv_file
        read_csv_and_enqueue('optical_flow_data.csv', result_queue)
    video_output.release()
    print("The client is quitting.")


def read_csv_and_enqueue(csv_filename, result_queue):
    try:
        with open(csv_filename, 'r') as csvfile:
            csv_reader = csv.reader(csvfile)
            next(csv_reader)  # 跳过表头
            for row in csv_reader:
                handle_result(row)
    except Exception as e:
        print(e)
        return False

def read_video():
    cap = cv2.VideoCapture('../PL_ORB.mp4')
    while cap.isOpened():
        ret, img = cap.read()
        if len(result_img_queue) > 60:
            break
        if not ret:
            break

        _, img_encoded = cv2.imencode('.jpg', img)
        img_bytes = img_encoded.tobytes()

        result_img_queue.append(img_bytes)
    print("Finish read video")
    cap.release()


def read_image(page):
    image_path = f'static/img/SD_image_{page}.bmp'
    img = cv2.imread(image_path)
    return img


def handle_image(img):
    try:
        _, img_encoded = cv2.imencode('.jpg', img)
        img_bytes = img_encoded.tobytes()

        result_img_queue.append(img_bytes)
        return True
    except Exception as e:
        print(e)
        return False


def handle_result(result):
    try:
        result_queue.append(result)
    except Exception as e:
        print(e)
        return False


# 測試用
# def read_video():
#     cap = cv2.VideoCapture("D:/coding/SOC_design_final_2023/web/test/test.mp4")
#     while cap.isOpened():
#         ret, img = cap.read()
#         if len(result_img_queue) > 1000:
#             break
#         if not ret:
#             break

#         _, img_encoded = cv2.imencode('.jpg', img)
#         img_bytes = img_encoded.tobytes()

#         result_img_queue.append(img_bytes)
#     print("Finish read video")
#     cap.release()


if __name__ == '__main__':
    try:
        # udp_thread = threading.Thread(
        #     target=udp_socket_server, args=("192.168.1.77", 7))
        # udp_thread.daemon = True
        # udp_thread.start()

        udp_socket_server("192.168.1.77", 7)
        # 測試用
        video_thread = threading.Thread(target=read_video)
        video_thread.daemon = True
        video_thread.start()

        # for _ in range(1000):
        #     index = random.randint(1, 3)
        #     x_values = random.sample(range(100), index)  # 从 0 到 99 中随机选择 3 个不重复的数
        #     y_values = random.sample(range(100), index)

        #     result_queue.append({"x": x_values, "y": y_values})

        app.run(debug=True, host="0.0.0.0", port=8100)

    except Exception as e:
        print(e)
    except KeyboardInterrupt:
        udp_thread.join()
        # video_thread.join()
