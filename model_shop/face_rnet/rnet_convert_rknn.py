import os
import urllib
import traceback
import time
import sys
import numpy as np
import cv2
from rknn.api import RKNN


PB_MODEL = 'rnet.pb'
RKNN_MODEL = 'rnet.rknn'

DATASET = './data.txt'
QUANTIZE_ON = True
IMG_PATH = "noface.jpg"

if __name__ == '__main__':
    rknn = RKNN()

    if not os.path.exists(PB_MODEL):
        print('model not exist')
        exit(-1)

    print('--> Config model')
    rknn.config(reorder_channel='0 1 2',
                mean_values=[[0, 0, 0]], std_values=[[255, 255, 255]],
                optimization_level=3,
                target_platform='rv1126',
                output_optimize=1,
                quantize_input_node=QUANTIZE_ON)
    print('done')


    # Load ONNX model
    print('--> Loading model')
    ret = rknn.load_tensorflow(tf_pb=PB_MODEL, inputs=['input_1'],
                               outputs=['conv5-1/Softmax', 'conv5-2/BiasAdd', ], input_size_list=[[24, 24, 3]])
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

    img = cv2.imread(IMG_PATH)
    outputs = rknn.inference(inputs=[img])
    print(outputs)
    np.save("rnet_out.npy", outputs)

