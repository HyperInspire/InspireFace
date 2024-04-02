import os
import random
import shutil
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


# Define the source and destination directories
source_dir = "/Users/tunm/datasets/WIDER_train/images"
dest_dir = "/Users/tunm/software/hyperface_model_shop_quant_data/widerface"  # Change this to your actual destination root directory
images_dir = os.path.join(dest_dir, "images")

# Ensure the destination directories exist
os.makedirs(images_dir, exist_ok=True)

# Prepare to write the image paths to data.txt
data_txt_path = os.path.join(dest_dir, "data.txt")

# Open data.txt file in write mode
with open(data_txt_path, "w") as data_file:
    # Iterate over each subdirectory in the source directory
    for subdir_name in os.listdir(source_dir):
        subdir_path = os.path.join(source_dir, subdir_name)

        # Check if it's a directory
        if os.path.isdir(subdir_path):
            # List all jpg files in this subdirectory
            jpg_files = [f for f in os.listdir(subdir_path) if f.endswith(".jpg")]

            # Randomly select up to 20 jpg files
            selected_files = random.sample(jpg_files, min(len(jpg_files), 20))

            # Copy selected files to the destination directory and write their paths to data.txt
            for file_name in selected_files:
                source_file_path = os.path.join(subdir_path, file_name)
                dest_file_path = os.path.join(images_dir, file_name)

                # Copy the file
                shutil.copy2(source_file_path, dest_file_path)

                # Write the relative path to data.txt
                relative_path = os.path.join("images", file_name)
                data_file.write(relative_path + "\n")

print(f"Selected images have been copied to {images_dir} and paths listed in {data_txt_path}")
