import os
import urllib
import traceback
import time
import sys
import numpy as np
import cv2
from rknn.api import RKNN


PB_MODEL = '/tunm/work/HyperFace/resource/models_raw/_01_face_landmark_106.pb'
RKNN_MODEL = '/tunm/work/HyperFace/resource/models_rv1109rv1126/_09_face_landmark_106.rknn'

DATASET = './data.txt'
QUANTIZE_ON = True

if __name__ == '__main__':
    rknn = RKNN()

    if not os.path.exists(PB_MODEL):
        print('model not exist')
        exit(-1)

    print('--> Config model')
    rknn.config(reorder_channel='0 1 2',
                mean_values=[[0, 0, 0]],
                std_values=[[255, 255, 255]],
                optimization_level=3,
                target_platform='rv1126',
                output_optimize=1,
                quantize_input_node=QUANTIZE_ON)
    print('done')


    # Load PB model
    print('--> Loading model')
    ret = rknn.load_tensorflow(tf_pb=PB_MODEL, inputs=['input_1'],
                               outputs=['prelu1/add', ], input_size_list=[[112, 112, 3]])
    if ret != 0:
        print('Load failed!')
        exit(ret)
    print('done')

    # Build model
    print('--> Building model')
    ret = rknn.build(do_quantization=QUANTIZE_ON, dataset=DATASET)
    if ret != 0:
        print('Build failed!')
        exit(ret)
    print('done')

    # init runtime environment
    print('--> Init runtime environment')
    ret = rknn.init_runtime()
    # ret = rknn.init_runtime('rk1808', device_id='1808')
    if ret != 0:
        print('Init runtime environment failed')
        exit(ret)
    print('done')

    # Export RKNN model
    print('--> Export RKNN model')
    ret = rknn.export_rknn(RKNN_MODEL)
    if ret != 0:
        print('Export failed!')
        exit(ret)
    print('done')

    img = cv2.imread("crop.png")
    img = cv2.resize(img, (112, 112))
    outputs = rknn.inference(inputs=[img])

    lmk = np.asarray(outputs[0][0]).reshape(-1, 2) * 112
    for x, y in lmk.astype(int):
        cv2.circle(img, (x, y), 0, (0, 0, 255), 1)

    cv2.imwrite("res.jpg", img)



"""



"""