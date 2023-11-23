
import socket
import cv2
import numpy as np
from PIL import Image

IM_X = 720
IM_Y = 480
HEADER_BYTES = 4
BODY_BYTES = IM_X  # * 3
PACKAGESIZE = BODY_BYTES + HEADER_BYTES

def main():
    # Create a UDP socket
    image_array = np.zeros((IM_X, IM_Y), np.uint8)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 65536)
    sock.setblocking(0)

    port = 8
    ip = '192.168.0.77'
    sock.bind((ip, port))
    win_name = "UDP Video"
    
    while True:
        try:
            data = sock.recv(PACKAGESIZE + 100)
        except:
            continue

        # Extract the current column count from the first 4 bytes
        current_column_count = int.from_bytes(data[:HEADER_BYTES], byteorder='big')
        print("Current Column Count:", current_column_count)

        # Divide the remaining data into 8-bit chunks
        data_chunked = data[HEADER_BYTES:]

        # Convert the data into a numpy array
        image_array = np.frombuffer(data_chunked, dtype=np.uint8).reshape((IM_X, IM_Y))

        # Display the received image using OpenCV
        cv2.imshow(win_name, image_array)
        
        # Divide data[HEADER_SIZE+180:] into 10-bit chunks
        additional_data = data[HEADER_BYTES + 180:]
        for i in range(0, len(additional_data), 100):
            chunk = additional_data[i:i + 100]
            chunk_value = int.from_bytes(chunk, byteorder='big')
            if chunk_value != 0:
                print("Match point pos:", chunk_value)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()

now you are a embedded programer, 關於vivado 的AXI DMA，對於單科AXI DMA，請問可以同時讀寫嗎?