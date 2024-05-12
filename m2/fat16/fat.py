import os
import struct


class F16DiskImage:
    def __init__(self, image_path):
        self.image_path = image_path
        self.files = {}
        self._load_image()

    def _load_image(self):
        BOOT_BLOCK = 512 
        CLUSTER_SIZE = 4096
        FAT_SIZE = 9  # Number of sectors per FAT
        RESERVED_SECTORS = 1  # Typically the boot sector
        NUM_FATS = 2  # Number of FAT copies
        ROOT_DIR_ENTRIES = 512
        ROOT_DIR_SECTORS = (ROOT_DIR_ENTRIES * 32 + (SECTOR_SIZE - 1)) // SECTOR_SIZE

        # Calculate start of data region
        data_start_sector = RESERVED_SECTORS + NUM_FATS * FAT_SIZE + ROOT_DIR_SECTORS

        try:
            with open(self.image_path, 'rb') as f:
                # Seek to the beginning of the data region
                f.seek(data_start_sector * SECTOR_SIZE)
                
                cluster_number = 0
                while True:
                    # Read one cluster at a time
                    cluster_data = f.read(CLUSTER_SIZE)
                    if not cluster_data:
                        break
                    
                    print(f"Cluster {cluster_number}:")
                    print(cluster_data)
                    cluster_number += 1
                    break

        except FileNotFoundError:
            print(f"Error: The disk image {self.image_path} could not be found.")
        except Exception as e:
            print(f"Error reading disk image: {e}")



    def _save_image(self):
        pass

    def list_files(self):
        return list(self.files.keys())
    
    def read_file(self, filename):
        try:
            return self.files[filename]['content']
        except KeyError:
            return "File not found."

    def write_file(self, filename, content):
        self.files[filename] = {'content': content, 'size': len(content), 'date': '2024-05-12', 'permissions': 'rw-'}
        self._save_image()

    def delete_file(self, filename):
        try:
            del self.files[filename]
            self._save_image()
            return "File deleted."
        except KeyError:
            return "File not found."

    def rename_file(self, old_name, new_name):
        if old_name in self.files:
            self.files[new_name] = self.files.pop(old_name)
            self._save_image()
            return "File renamed."
        else:
            return "Original file not found."

    def get_metadata(self, filename):
        try:
            file_info = self.files[filename]
            return {key: file_info[key] for key in ['size', 'date', 'permissions']}
        except KeyError:
            return "File not found."


if __name__ == "__main__":
    disk_image = F16DiskImage("disco1.img")
    print("Listing all files:")
    print(disk_image.list_files())

    # Example of reading a file
    # print("Reading contents of 'example.txt':")
    # print(disk_image.read_file('example.txt'))