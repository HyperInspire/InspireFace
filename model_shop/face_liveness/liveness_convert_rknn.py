import os
import urllib
import traceback
import time
import sys
import numpy as np
import cv2
from rknn.api import RKNN

"""
用来量化的数据对推理结果有较大的影响，当前采用LFW数据进行原图量化 未进行测试
"""

ONNX_MODEL = '/tunm/work/InspireFace/resource/models_raw/_06_2_7_80x80_MiniFASNetV2.onnx'
RKNN_MODEL = '_06_2_7_80x80_MiniFASNetV2.rknn'

DATASET = './data.txt'
QUANTIZE_ON = True

def softmax(x):
    # 计算指数值
    exp_x = np.exp(x)

    # 计算每个元素的指数值之和
    sum_exp_x = np.sum(exp_x)

    # 计算Softmax值
    softmax_values = exp_x / sum_exp_x

    return softmax_values

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
                optimization_level=3,
                target_platform='rv1126',
                output_optimize=1,
                quantize_input_node=QUANTIZE_ON,
                # quantized_dtype='dynamic_fixed_point-i16'
                )
    print('done')


    # Load ONNX model
    print('--> Loading model')
    ret = rknn.load_onnx(model=ONNX_MODEL, outputs=["556", ], )
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

    list_ = ["fake.jpg", "real.jpg", "ttt.jpg", "live.jpg", "w2.jpg"]
    outputs = list()
    for name in list_:
        img = cv2.imread(name)
        # img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
        output = rknn.inference(inputs=[img])
        output = output[0][0]
        sm = softmax(output)
        print(f"origin: {name} {output}")
        print(f"softmax: {name} {sm}")
        print("---")

'''
--> Init runtime environment
librknn_runtime version 1.7.1 (bd41dbc build: 2021-10-28 16:15:23 base: 1131)
done
--> Export RKNN model
done
fake.jpg [0.3475143  0.6419917  0.01049402]
real.jpg [0.00201952 0.9899775  0.00800296]


U8: 
ot: fake.jpg [ 2.2725     1.1071154 -3.3796153]
sm: fake.jpg [0.7602755  0.23705594 0.00266863]
ot: real.jpg [-2.039423    2.5055768  -0.46615383]
sm: real.jpg [0.01000172 0.9417661  0.04823218]
'''


'''New quant 
origin: fake.jpg [ 5.252926  -4.377438  -0.8754876]
softmax: fake.jpg [9.9775928e-01 6.5555912e-05 2.1751518e-03]
---
origin: real.jpg [-5.1945596  3.968877   1.2256826]
softmax: real.jpg [9.8454810e-05 9.3943536e-01 6.0466200e-02]
---
origin: ttt.jpg [-4.8443646  3.3268528  1.5175118]
softmax: ttt.jpg [2.4283746e-04 8.5907352e-01 1.4068362e-01]
---
origin: live.jpg [-6.4202423  2.217902   4.8443646]
softmax: live.jpg [1.1953859e-05 6.7453817e-02 9.3253422e-01]
---
origin: w2.jpg [-4.2023406  3.0350237  1.1673168]
softmax: w2.jpg [6.2258221e-04 8.6565346e-01 1.3372397e-01]
---

'''