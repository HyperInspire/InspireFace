import os
import shutil
import random

import os

# 指定要遍历的文件夹路径
folder_path = '/Users/tunm/software/hyperface_model_shop_quant_data/face_quality/images'

# 打开data.txt文件以写入模式
with open('data.txt', 'w') as file:
    # 使用os.walk遍历文件夹及其子文件夹
    for root, _, files in os.walk(folder_path):
        for file_name in files:
            # 检查文件是否为.jpg文件
            if file_name.endswith('.jpg'):
                # 获取文件夹名（不包含全部路径）
                folder_name = os.path.basename(root)
                # 构建文件路径
                file_path = os.path.join(folder_name, file_name)
                # 将文件路径写入data.txt
                file.write(file_path + '\n')

print('data.txt文件已创建并写入文件路径。')
