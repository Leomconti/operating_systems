class FAT16Disk:
    def __init__(self, image_path):
        self.image_path = image_path
        self.boot_sector = b''
        self.disk = None

    def read_boot_sector(self):
        try:
            with open(self.image_path, 'rb') as self.disk:
                self.boot_sector = self.disk.read(512)
                # self.print_boot_sector()
                self.display_boot_sector_info()
        except FileNotFoundError:
            print("File not found. Please check the path and try again.")
        except Exception as e:
            print(f"An error occurred: {e}")

    def display_boot_sector_info(self):
        # Retrieve essential values from the boot sector
        bytes_per_block = int.from_bytes(self.boot_sector[0x0b:0x0d], 'little')
        sectors_per_fat = int.from_bytes(self.boot_sector[0x16:0x18], 'little')
        num_fats = self.boot_sector[0x10]
        num_root_dir_entries = int.from_bytes(self.boot_sector[0x11:0x13], 'little')
        reserved_sectors = int.from_bytes(self.boot_sector[0x0e:0x10], 'little')

        # Calculate the start of the root directory
        root_dir_start_sector = reserved_sectors + (num_fats * sectors_per_fat)
        root_dir_bytes = (num_root_dir_entries * 32)
        root_dir_sectors = (root_dir_bytes + bytes_per_block - 1) // bytes_per_block

        print(f"Root Directory starts at sector: {root_dir_start_sector}, spans {root_dir_sectors} sectors")

        # Now read the root directory
        self.list_root_directory(root_dir_start_sector, root_dir_sectors, bytes_per_block)

    def list_root_directory(self, start_sector, num_sectors, bytes_per_sector):
        self.disk.seek(start_sector * bytes_per_sector)
        for _ in range(num_sectors):
            sector_data = self.disk.read(bytes_per_sector)
            for entry in range(0, len(sector_data), 32):
                self.parse_directory_entry(sector_data[entry:entry + 32])

    def parse_directory_entry(self, entry):
        if entry[0] == 0x00:
            return  # No more entries
        if entry[0] == 0xE5:
            return  # Entry is deleted

        filename = entry[:8].decode('ascii').strip() + '.' + entry[8:11].decode('ascii').strip()
        attrs = entry[11]
        if attrs & 0x10:
            print(f"Directory: {filename}")
        else:
            filesize = int.from_bytes(entry[28:32], 'little')
            print(f"File: {filename}, Size: {filesize}")

if __name__ == "__main__":
    disk_image_path = "disco1.img"
    fat16_disk = FAT16Disk(disk_image_path)
    fat16_disk.read_boot_sector()
