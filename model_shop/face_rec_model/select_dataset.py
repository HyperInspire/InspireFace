import os
import shutil
import random

source_root = '/Users/tunm/software/hyperface_model_shop_quant_data/face_rec/celeba_align_1k'  # Replace with your source directory path
destination_root = '/Users/tunm/software/hyperface_model_shop_quant_data/face_rec/celeba_align_200'  # Replace with your desired destination path

# If the destination directory exists, remove it
if os.path.exists(destination_root):
    shutil.rmtree(destination_root)

# Ensure the destination directory is created
os.makedirs(destination_root)

all_files = []

# Get all image files from the source directories
for filename in os.listdir(source_root):
    if filename.endswith('.jpg'):  # Assuming you're dealing with jpg images. Modify as needed.
        all_files.append(os.path.join(source_root, filename))

# Randomly select 200 images
selected_files = random.sample(all_files, 200)

# Copy the selected images to the destination directory and collect relative paths for data.txt
relative_paths = []
for filepath in selected_files:
    shutil.copy(filepath, destination_root)
    relative_path = os.path.join("images", os.path.basename(filepath))
    relative_paths.append(relative_path)

# Write the relative paths to data.txt
with open("data.txt", "w") as f:
    for path in relative_paths:
        f.write(path + "\n")

print(f"Copied 200 images to {destination_root} and created data.txt")
