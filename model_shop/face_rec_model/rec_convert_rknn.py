import os
import urllib
import traceback
import time
import sys
import numpy as np
import cv2
from rknn.api import RKNN


ONNX_MODEL = '/tunm/work/HyperFace/resource/models_raw/_03_r18_Glint360K_fixed.onnx'
# RKNN_MODEL = '/tunm/work/HyperFace/resource/models_rv1109rv1126/_03_r18_Glint360K_fixed.rknn'
RKNN_MODEL = 'rec_r18_u8.rknn'

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
                mean_values=[[0, 0, 0]],
                std_values=[[255, 255, 255]],
                optimization_level=3,
                target_platform='rv1126',
                output_optimize=1,
                quantize_input_node=QUANTIZE_ON,
                # quantized_dtype='dynamic_fixed_point-i16',  # i16在 1.7.1砖汉后1.7.3下跑可以使用
    )
    print('done')


    # Load ONNX model
    print('--> Loading model')
    ret = rknn.load_onnx(model=ONNX_MODEL, outputs=["267", ], )
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

    list_ = ["0.jpg", "1.jpg", "2.jpg"]
    outputs = list()
    for name in list_:
        img = cv2.imread(name)
        img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        output = rknn.inference(inputs=[img])
        outputs.append(output)
    np.save("f.npy", np.asarray(outputs))

"""
0v1: 0.56994116
2v1: 0.04088772
0v2: -0.03256973

"""