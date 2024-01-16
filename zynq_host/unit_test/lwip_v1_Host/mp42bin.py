import cv2
import numpy as np

sec = 0
count = 0
frameRate = 0.05
video_start = 150
video_end = 400
vidcap = cv2.VideoCapture('MLIO.mp4')

def rotate_image(image, angle):
    rows, cols, _ = image.shape
    rotation_matrix = cv2.getRotationMatrix2D((cols / 2, rows / 2), angle, 1)
    rotated_image = cv2.warpAffine(image, rotation_matrix, (cols, rows))
    return rotated_image

def resize_image(image, width, height):
    resized_image = cv2.resize(image, (width, height))
    return resized_image

def crop_center(image, target_width, target_height):
    height, width = image.shape[:2]
    start_x = max(0, (width - target_width) // 2)
    start_y = max(0, (height - target_height) // 2)
    cropped_image = image[start_y:start_y + target_height, start_x:start_x + target_width]
    return cropped_image

def getFrame_BIN8bits(sec, bin_file):
    vidcap.set(cv2.CAP_PROP_POS_MSEC, sec * 1000)
    hasFrames, image = vidcap.read()
    if hasFrames:
        resized_image = resize_image(image, 320, 240)
        cropped_image = crop_center(resized_image, 240, 240)
        gray = cv2.cvtColor(cropped_image, cv2.COLOR_BGR2GRAY)
        # Convert to a numpy array for easier manipulation
        gray_np = np.array(gray)
        # Save the grayscale pixel values as a binary file
        with open(bin_file, 'ab') as file:
            file.write(gray_np.tobytes())  # Write the pixel values as binary data
    return hasFrames, gray

def rot_BIN8bits(rot_bin_file):
    vidcap.set(cv2.CAP_PROP_POS_MSEC, sec * 1000)
    hasFrames, image = vidcap.read()
    if hasFrames:
        # Rotate
        rotated_image = rotate_image(image, 55)  # Rotate by 55 degrees
        cropped_image = crop_center(rotated_image, 240, 240)

        rot_gray = cv2.cvtColor(cropped_image, cv2.COLOR_BGR2GRAY)
        # Convert to a numpy array for easier manipulation
        rot_gray_np = np.array(rot_gray)
        # Save the grayscale pixel values as a binary file
        with open(rot_bin_file, 'ab') as file:
            file.write(rot_gray_np.tobytes())  # Write the pixel values as binary data

bin_file = 'D:\\D_region\\Lab_project\\pycharm\\mp42txt\\pixels.bin'
rot_bin_file = 'D:\\D_region\\Lab_project\\pycharm\\mp42txt\\rot_pixels.bin'

# success, image = getFrame_BIN8bits(sec, bin_file)  # Initialize
success = 1
while success:
    count = count + 1
    sec = sec + frameRate

    if video_end > count > video_start:
        success, image = getFrame_BIN8bits(sec, bin_file)
        rot_BIN8bits(rot_bin_file)
        cv2.imshow('G_slice', image)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cv2.destroyAllWindows()
