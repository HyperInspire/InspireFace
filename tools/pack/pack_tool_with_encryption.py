import  struct
import numpy as np;
from ctypes import create_string_buffer

import os
import binascii
# from cv2 import  dnn


# dnn.
def encryption(password,data):
    password = bytearray(password)
    data = bytearray(data)
    for i in range(len(data)):
        data[i] = data[i]^password[i%len(password)]
    return str(data)


def packageModel(model_folder, output, key, magic_number=1128):
    # int magic num 1127 HyperLPR model
    # int model nums
    # int int  prototxt caffemodel 1
    # int int  prototxt caffemodel 2
    # ......
    # byte  xxxxxxxx

    model_data = []
    print "loading model"
    _key = open(key,'rb+')
    _data_key = _key.read()
    _key.close()

    template =""" 
#ifndef MODEL_LOADER_CONST_H_
#define MODEL_LOADER_CONST_H_
namespace Model{
%s
}
#endif //MODEL_LOADER_CONST_H_
    """

    subtxt = ""
    for filename in sorted(os.listdir(model_folder)):
        if filename.endswith(".prototxt"):
            name,ext  = filename.split(".")
            prototxt = name + ".prototxt"
            caffemodel = name + ".caffemodel"
            f_p  = file(os.path.join(model_folder,prototxt),"rb")
            print   "   "+name
            #try:
            #    dnn.shrinkCaffeModel(os.path.join(model_folder,caffemodel),os.path.join(model_folder,caffemodel))
            #except:
            #    print "can't shrink model %s"%(caffemodel)

            f_c = file(os.path.join(model_folder,caffemodel), "rb")
            model_data.append([f_p.read(),f_c.read(),name])

        if filename.endswith(".pbtxt"):
            name,ext  = filename.split(".")
            prototxt = name + ".pbtxt"
            caffemodel = name + ".pb"
            f_p  = file(os.path.join(model_folder,prototxt),"rb")
            print   "   "+name
            # dnn.shrinkCaffeModel(os.path.join(model_folder,caffemodel),os.path.join(model_folder,caffemodel))
            f_c = file(os.path.join(model_folder,caffemodel), "rb")
            model_data.append([f_p.read(),f_c.read(),name])

        if filename.endswith(".param"):
            name,ext  = filename.split(".")
            prototxt = name + ".param"
            caffemodel = name + ".bin"
            f_p  = file(os.path.join(model_folder,prototxt),"rb")
            print   "   "+name
            f_c = file(os.path.join(model_folder,caffemodel), "rb")
            model_data.append([f_p.read(),f_c.read(),name])

        if filename.endswith(".onnx"):
            name,ext  = filename.split(".")
            caffemodel = name + ".onnx"
            f_p  = "Null_prototxt"
            print   "   "+name
            f_c = file(os.path.join(model_folder,caffemodel), "rb")
            model_data.append([f_p,f_c.read(),name])
        if filename.endswith(".mnn"):
            name,ext  = filename.split(".")
            caffemodel = name + ".mnn"
            f_c = file(os.path.join(model_folder,caffemodel), "rb")
            model_data.append([b"None",f_c.read(),name])

        if filename.endswith(".tflite"):
            name,ext  = filename.split(".")
            caffemodel = name + ".tflite"
            f_c = file(os.path.join(model_folder,caffemodel), "rb")
            model_data.append([b"None",f_c.read(),name])

        if filename.endswith(".rknn"):
            name,ext  = filename.split(".")
            caffemodel = name + ".rknn"
            f_c = file(os.path.join(model_folder,caffemodel), "rb")
            model_data.append([b"None",f_c.read(),name])

    offset = 0
    header = [magic_number, len(model_data)]
    header_format = '<II'+len(model_data)*"II"
    length =  0 ;
    for a,b,name in model_data:
        length+= len(a)+len(b)
        header+=[len(a),len(b)]

    buffer_data_set = create_string_buffer(2 * 4 + len(model_data)*8 + length)

    struct.pack_into( header_format,buffer_data_set,offset,*header)
    offset += struct.calcsize(header_format)
    # print model_data
    print "Header",header
    print "===================================================================================================="
    indexdata = open(output+".index", 'w');
    for i,(a,b,name) in enumerate(model_data):
        a = encryption(_data_key,a)
        b = encryption(_data_key,b)
        size = len(a) + len(b)
        len_data_format = '<' + str(size) + 'B'
        print "id ",i,"      ",str(size)+" Bytes of "+name
        subtxt += "const static int {0} = {1};\n".format(name,i);
        # a = '12143324324'b
        data = a+b
        data = bytearray(data)
        # print type(data)
        struct.pack_into(len_data_format,buffer_data_set,offset,*data);

        offset += struct.calcsize(len_data_format)
        indexdata.write("%s\n"%name)
    indexdata.close()
    header = template%(subtxt)
    f = open("model_helper_const.h", "w")
    f.write(header)
    f.close()

    filedata = open(output, 'wb');
    filedata.write(buffer_data_set);
    filedata.close()
    print "Make successfully!"
    print "output filename "+output

import sys
args  = sys.argv

print(args)
if len(args) == 5:
    print(int(args[4]))
    packageModel(args[1], args[2], args[3], int(args[4]))
elif len(args) == 4:
    packageModel(args[1], args[2], args[3])
else:
    print("Usage: python[2.7] pack_tool.py folder package_name key [magic_number]")
    exit(0)

# packageModel(args[1],args[2] ,args[3])

