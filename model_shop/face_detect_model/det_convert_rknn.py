import os
import urllib
import traceback
import time
import sys
import numpy as np
import cv2
from rknn.api import RKNN


ONNX_MODEL = '/tunm/work/HyperFace/resource/models_raw/_00_scrfd_500m_bnkps_shape320x320.onnx'
RKNN_MODEL = '/tunm/work/HyperFace/resource/models_rv1109rv1126/_00_scrfd_500m_bnkps_shape320x320.rknn'

DATASET = './data.txt'
QUANTIZE_ON = True
IMG_PATH = "input.jpg"

if __name__ == '__main__':
    rknn = RKNN()

    if not os.path.exists(ONNX_MODEL):
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


    # Load ONNX model
    print('--> Loading model')
    ret = rknn.load_onnx(model=ONNX_MODEL, outputs=["443", "468", "493", "446", "471", "496", "449", "474", "499"])
    if ret != 0:
        print('Load SCRFD failed!')
        exit(ret)
    print('done')

    # Build model
    print('--> Building model')
    ret = rknn.build(do_quantization=QUANTIZE_ON, dataset=DATASET)
    if ret != 0:
        print('Build SCRFD failed!')
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
        print('Export SCRFD failed!')
        exit(ret)
    print('done')

    img = cv2.imread(IMG_PATH)
    outputs = rknn.inference(inputs=[img])
    print(len(outputs))
    np.save("pickle.npy", outputs)

