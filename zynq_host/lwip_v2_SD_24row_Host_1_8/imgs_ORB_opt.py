import cv2
import numpy as np
import csv

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
    csv_file = open('optical_flow_data.csv', mode='w', newline='')
    csv_writer = csv.writer(csv_file)
    csv_writer.writerow(['Page', 'X_prev', 'Y_prev', 'X_next', 'Y_next'])

    video_output = cv2.VideoWriter('optical_flow_output.mp4', cv2.VideoWriter_fourcc(*'mp4v'), 30, (720, 480))

    prev_img = read_image(1)

    max_page = 5  # 假設最大頁數是60

    if max_page % 5 == 0:
        for page in range(2, max_page + 1):
            next_img = read_image(page)

            good_prev, good_next = sparse_optical_flow(prev_img, next_img)

            optical_flow_img = np.zeros_like(prev_img)
            optical_flow_img = draw_optical_flow(optical_flow_img, good_prev, good_next)

            output_img = cv2.addWeighted(next_img, 1, optical_flow_img, 1, 0)

            # Print and save optical flow data
            for i, (prev, next) in enumerate(zip(good_prev, good_next)):
                x_prev, y_prev = prev.ravel().astype(int)
                x_next, y_next = next.ravel().astype(int)
                print(f"Page: {page}, X_prev: {x_prev}, Y_prev: {y_prev}, X_next: {x_next}, Y_next: {y_next}")
                csv_writer.writerow([page, x_prev, y_prev, x_next, y_next])


            # 若偵測到page上限值mod 10 = 0，再刷新光流畫面、CSV檔案、撥放視頻
            # cv2.imwrite(f'optical_flow_output_{page}.png', output_img)  # 另存為每10張的光流畫面
            video_output.write(output_img)  # 寫入視頻
            csv_file.close()  # 關閉舊的CSV檔案
            csv_file = open('optical_flow_data.csv', mode='w', newline='')  # 開啟新的CSV檔案
            csv_writer = csv.writer(csv_file)
            csv_writer.writerow(['Page', 'X_prev', 'Y_prev', 'X_next', 'Y_next'])
            prev_img = next_img

        if max_page == 60:  # 若保存完視頻大於60張，自動撥放
            cap = cv2.VideoCapture('optical_flow_output.mp4')
            while cap.isOpened():
                ret, frame = cap.read()
                if not ret:
                    break
                cv2.imshow('Optical Flow Video', frame)
                if cv2.waitKey(25) & 0xFF == ord('q'):
                    break

            cap.release()
            cv2.destroyAllWindows()

    csv_file.close()
    video_output.release()

if __name__ == "__main__":
    main()
