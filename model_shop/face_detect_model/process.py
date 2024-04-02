import cv2
import numpy as np

def pre_process(img, input_size=(320, 320)):
    im_ratio = float(img.shape[0]) / img.shape[1]
    model_ratio = float(input_size[1]) / input_size[0]
    if im_ratio>model_ratio:
        new_height = input_size[1]
        new_width = int(new_height / im_ratio)
    else:
        new_width = input_size[0]
        new_height = int(new_width * im_ratio)
    det_scale = float(new_height) / img.shape[0]
    resized_img = cv2.resize(img, (new_width, new_height))
    det_img = np.zeros( (input_size[1], input_size[0], 3), dtype=np.uint8 )
    det_img[:new_height, :new_width, :] = resized_img

    return det_img


if __name__ == '__main__':
    img = cv2.imread("Ben_Chandler_0001.jpg")
    det = pre_process(img)
    print(det.shape)
    cv2.imshow("w", det)
    cv2.waitKey(0)
    cv2.imwrite("Ben_Chandler_0001_norm.jpg", det, )