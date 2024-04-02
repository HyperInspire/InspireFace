import os
import tarfile
import click
import tqdm


def remove_suffix(filename):
    """移除文件名的后缀"""
    return os.path.splitext(filename)[0]

@click.command()
@click.argument('folder_path')
@click.argument('output_filename')
@click.option('--rm-suffix', is_flag=True, default=True, help='Remove file suffixes in the archive.')
def make_tar(folder_path, output_filename, rm_suffix):
    """
    打包指定文件夹为tar文件。FOLDER_PATH 是要打包的文件夹路径，
    OUTPUT_FILENAME 是输出的tar文件名。
    """
    with tarfile.open(output_filename, "w") as tar:
        print("packing....")
        for root, dirs, files in os.walk(folder_path):
            for file in tqdm.tqdm(files):
                file_path = os.path.join(root, file)
                arcname = os.path.relpath(file_path, start=folder_path)
                if rm_suffix:
                    # 分割文件名和目录名
                    dirs, filename = os.path.split(arcname)
                    # 去除文件后缀
                    filename = remove_suffix(filename)
                    # 重新拼接目录和处理后的文件名
                    arcname = os.path.join(dirs, filename)
                tar.add(file_path, arcname=arcname)

if __name__ == '__main__':
    make_tar()
