import os
import sys
from pathlib import Path
import urllib.request
import ssl
import hashlib

def get_file_hash_sha256(file_path):
    sha256 = hashlib.sha256()
    with open(file_path, 'rb') as f:
        for chunk in iter(lambda: f.read(4096), b''):
            sha256.update(chunk)
    return sha256.hexdigest()

class ResourceManager:
    def __init__(self):
        """Initialize resource manager and create necessary directories"""
        self.user_home = Path.home()
        self.base_dir = self.user_home / '.inspireface'
        self.models_dir = self.base_dir / 'models'
        
        # Create directories
        self.base_dir.mkdir(exist_ok=True)
        self.models_dir.mkdir(exist_ok=True)
        
        # Model URLs
        self._MODEL_LIST = {
            "Pikachu": {
                "url": "https://github.com/HyperInspire/InspireFace/releases/download/v1.x/Pikachu",
                "filename": "Pikachu",
                "md5": "3024649812ac8c921968444d6b57423fe1bbf66c0cb122c0d7f5f2a61a66bb27"
            },
            "Megatron": {
                "url": "https://github.com/HyperInspire/InspireFace/releases/download/v1.x/Megatron",
                "filename": "Megatron",
                "md5": "83d2de0728ca5b311055c87784619086e1043fa3c5d70ed24c262e7971419d8e"
            },
            "Megatron_TRT": {
                "url": "https://github.com/HyperInspire/InspireFace/releases/download/v1.x/Megatron_TRT",
                "filename": "Megatron_TRT",
                "md5": "99a50293213dea469e3b5875a6ed35ebe8341c255a5a4a62bc0894f97ee634e1"
            },
            "Gundam_RK356X": {
                "url": "https://github.com/HyperInspire/InspireFace/releases/download/v1.x/Gundam_RK356X",
                "filename": "Gundam_RK356X",
                "md5": "d1e0aa465865c217c95c46685925cffc15043adad95ffabc23bdde91fc4f17bd"
            },
            "Gundam_RK3588": {
                "url": "https://github.com/HyperInspire/InspireFace/releases/download/v1.x/Gundam_RK3588",
                "filename": "Gundam_RK3588",
                "md5": "eeeb3a82bbdd909d782d4e416cae1112c39e2e75e1e7f5024101f85ecdd1493c"
            }
        }

    def get_model(self, name: str, re_download: bool = False, ignore_verification: bool = False) -> str:
        """
        Get model path. Download if not exists or re_download is True.
        
        Args:
            name: Model name
            re_download: Force re-download if True
            ignore_verification: Skip model hash verification if True
            
        Returns:
            str: Full path to model file
        """
        if name not in self._MODEL_LIST:
            raise ValueError(f"Model '{name}' not found. Available models: {list(self._MODEL_LIST.keys())}")

        model_info = self._MODEL_LIST[name]
        model_file = self.models_dir / model_info["filename"]
        downloading_flag = model_file.with_suffix('.downloading')
        
        # Check if model exists and is complete
        if model_file.exists() and not downloading_flag.exists() and not re_download:
            if ignore_verification:
                print(f"Warning: Model verification skipped for '{name}' as requested.")
                return str(model_file)
                
            current_hash = get_file_hash_sha256(model_file)
            if current_hash == model_info["md5"]:
                return str(model_file)
            else:
                print(f"Model file hash mismatch for '{name}'. Re-downloading...")

        # Start download
        try:
            print(f"Downloading model '{name}'...")
            downloading_flag.touch()

            # Create SSL context and headers
            ssl_context = ssl.create_default_context()
            ssl_context.check_hostname = False
            ssl_context.verify_mode = ssl.CERT_NONE
            headers = {'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36'}
            
            req = urllib.request.Request(model_info["url"], headers=headers)
            
            with urllib.request.urlopen(req, context=ssl_context) as response:
                total_size = int(response.headers.get('content-length', 0))
                block_size = 8192
                downloaded_size = 0
                
                with open(model_file, 'wb') as f:
                    while True:
                        buffer = response.read(block_size)
                        if not buffer:
                            break
                        downloaded_size += len(buffer)
                        f.write(buffer)
                        
                        if total_size > 0:
                            percent = (downloaded_size / total_size) * 100
                            sys.stdout.write(f"\rDownloading {name}: {percent:.1f}%")
                            sys.stdout.flush()
            
            print("\nDownload completed")
            downloading_flag.unlink()  # Remove the downloading flag
            return str(model_file)

        except Exception as e:
            if model_file.exists():
                model_file.unlink()
            if downloading_flag.exists():
                downloading_flag.unlink()
            raise RuntimeError(f"Failed to download model: {e}")
        
# Usage example
if __name__ == "__main__":
    try:
        rm = ResourceManager()
        model_path = rm.get_model("Pikachu")
        print(f"Model path: {model_path}")
        
    except Exception as e:
        print(f"Error: {e}")