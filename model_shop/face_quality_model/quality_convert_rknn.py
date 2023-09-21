import os
import urllib
import traceback
import time
import sys
import numpy as np
import cv2
from rknn.api import RKNN


ONNX_MODEL = '/tunm/work/HyperFace/resource/models_raw/_07_pose-quality.onnx'
RKNN_MODEL = '/tunm/work/HyperFace/resource/models_rv1109rv1126/_07_pose-quality.rknn'

DATASET = './data.txt'
QUANTIZE_ON = True

if __name__ == '__main__':
    rknn = RKNN()

    if not os.path.exists(ONNX_MODEL):
        print('model not exist')
        exit(-1)

    print('--> Config model')
    rknn.config(reorder_channel='0 1 2',
                # mean_values=[[127.5, 127.5, 127.5]],
                # std_values=[[127.5, 127.5, 127.5]],
                # mean_values=[[0, 0, 0]],
                # std_values=[[255, 255, 255]],
                # optimization_level=1,
                target_platform='rv1126',
                # output_optimize=1,
                quantize_input_node=QUANTIZE_ON,
                quantized_dtype='dynamic_fixed_point-i16',

                )
    print('done')


    # Load ONNX model
    print('--> Loading model')
    ret = rknn.load_onnx(model=ONNX_MODEL, outputs=["fc1", ], )
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

    name = "p3.jpg"
    img = cv2.imread(name)
    data = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    output = rknn.inference(inputs=[data])
    print(output)
    output = output[0][0]
    pitch, yaw, roll, x1, y1, x2, y2, x3, y3, x4, y4, x5, y5, quality_eye_left, quality_eye_right, quality_eye_nose, quality_mouth_left, right = output
    lmk = (np.asarray([[x1, y1], [x2, y2], [x3, y3], [x4, y4], [x5, y5]]) + 1) * (96 / 2)
    for x, y in lmk.astype(int):
        cv2.circle(img, (x, y), 0, (0, 255, 255), 1)
    print(pitch * 90, yaw * 90, roll * 90)

    cv2.imwrite("p1r.jpg", img)


"""
[array([[ 0.00629412, -0.2769411 , -0.1573529 , -0.3524705 , -0.1195882 ,
         0.05664704, -0.23917641, -0.20141171,  0.09441174, -0.20770583,
         0.35876462,  0.13847055,  0.270647  ,  0.        ,  0.05664704,
         0.08182351,  0.06923527,  0.10699997]], dtype=float32)]
0.5664704320952296 -24.924698173999786 -14.16176050901413
"""