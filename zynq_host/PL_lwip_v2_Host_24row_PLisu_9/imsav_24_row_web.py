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
max_page = 1


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

    while True:
        try:
            data = sock.recv(65535)
            lwip_cnt += 1
        except:
            continue
        # print(data, "\n")

        # Splitting data into 24 rows
        for row in range(24):
            cur_line = int.from_bytes(data[row * 2264:row * 2264 + 4], 'little', signed=False)
            if cur_line < 0 or cur_line >= IM_Y:
                continue
            print("cur_line :　", cur_line, "\n")

            for col in range(IM_X):
                color_location = HEADER_BYTES + col * IM_CHANNEL
                for i in range(IM_CHANNEL):
                    pixel = int.from_bytes(data[4 + row * 2264 + col * 3 + i:4 + row * 2264 + col * 3 + i + 1],
                                           'little', signed=False)
                    image_array[cur_line][col][i] = pixel
            print("last pixel:　", pixel, "\n")

        if cur_line == IM_Y - 1:
            cv2.imwrite(f'PL_image_{page}.bmp', image_array)
            page = page + 1
            bits = ''.join(format(byte, '08b') for byte in data[2164:2264])
            int_values = [int(bits[i:i + 10], 2) for i in range(0, len(bits), 10)]
            # for value in int_values:
            # print(value)
        print("total shake :　", lwip_cnt, "\n")
    print("The client is quitting.")
    # sock.close()
    # cv2.destroyAllWindows()


if __name__ == '__main__':
    main()
