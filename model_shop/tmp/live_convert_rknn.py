import os
import urllib
import traceback
import time
import sys
import numpy as np
import cv2
from rknn.api import RKNN


CAFFE_MODEL = '/tunm/work/HyperFace/resource/models_raw/_03_mfngg.caffemodel'
PROTOTXT = "/tunm/work/HyperFace/resource/models_raw/_03_mfngg.prototxt"
RKNN_MODEL = '/tunm/work/HyperFace/resource/models_rv1109rv1126/_03_mfngg.rknn'

DATASET = './data.txt'
QUANTIZE_ON = True
IMG_PATH = "input.jpg"

if __name__ == '__main__':
    rknn = RKNN()

    if not os.path.exists(CAFFE_MODEL):
        print('model not exist')
        exit(-1)

    print('--> Config model')
    rknn.config(reorder_channel='0 1 2',
                mean_values=[[127.5, 127.5, 127.5]],
                std_values=[[127.5, 127.5, 127.5]],
                # optimization_level=1,
                target_platform='rv1126',
                # output_optimize=1,
                quantize_input_node=QUANTIZE_ON)
    print('done')


    # Load ONNX model
    print('--> Loading model')
    ret = rknn.load_caffe(blobs=CAFFE_MODEL, model=PROTOTXT, proto='caffe',)
    if ret != 0:
        print('Load failed!')
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
        print('Export failed!')
        exit(ret)
    print('done')

    # img = cv2.imread(IMG_PATH)
    # outputs = rknn.inference(inputs=[img])
    # print(len(outputs))
    # np.save("pickle.npy", outputs)

