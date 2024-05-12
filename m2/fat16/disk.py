class FAT16Disk:
    def __init__(self, image_path):
        self.image_path = image_path
        self.boot_sector = None
        self.disk = None
        self.bytes_per_sector = 0
        self.sectors_per_cluster = 0
        self.sectors_per_fat = 0
        self.root_dir_start_sector = 0
        self.data_start_sector = 0

    def open_disk(self):
        try:
            self.disk = open(self.image_path, 'rb')
        except FileNotFoundError:
            print("File not found. Please check the path and try again.")
        except Exception as e:
            print(f"An error occurred: {e}")

    def read_boot_sector(self):
        if not self.disk:
            self.open_disk()
        self.disk.seek(0)
        self.boot_sector = self.disk.read(512)
        self.parse_boot_sector()

    def parse_boot_sector(self):
        self.bytes_per_sector = int.from_bytes(self.boot_sector[0x0b:0x0d], 'little')
        self.sectors_per_cluster = self.boot_sector[0x0d]
        self.sectors_per_fat = int.from_bytes(self.boot_sector[0x16:0x18], 'little')
        num_fats = self.boot_sector[0x10]
        num_root_dir_entries = int.from_bytes(self.boot_sector[0x11:0x13], 'little')
        reserved_sectors = int.from_bytes(self.boot_sector[0x0e:0x10], 'little')

        self.root_dir_start_sector = reserved_sectors + (num_fats * self.sectors_per_fat)
        root_dir_bytes = num_root_dir_entries * 32
        root_dir_sectors = (root_dir_bytes + self.bytes_per_sector - 1) // self.bytes_per_sector

        self.data_start_sector = self.root_dir_start_sector + root_dir_sectors

        print(f"Data region starts at sector: {self.data_start_sector}")
        self.list_root_directory()

    def list_root_directory(self):
        self.disk.seek(self.root_dir_start_sector * self.bytes_per_sector)
        for _ in range(32):  # Maximum number of entries in root directory
            entry = self.disk.read(32)
            self.parse_directory_entry(entry)

    def parse_directory_entry(self, entry):
        if entry[0] == 0x00:
            return  # No more entries
        if entry[0] == 0xE5:
            return  # Entry is deleted

        attrs = entry[11]
        if attrs & 0x08:  # Volume label, skip it
            return

        filename = entry[:8].decode('ascii').strip() + '.' + entry[8:11].decode('ascii').strip()
        first_cluster = int.from_bytes(entry[26:28], 'little')
        filesize = int.from_bytes(entry[28:32], 'little')

        if attrs & 0x10:
            print(f"Directory: {filename}")
        else:
            print(f"File: {filename}, Size: {filesize}, First Cluster: {first_cluster}")
            self.read_file_content(first_cluster, filesize)

    def read_file_content(self, cluster, size):
        cluster_offset = ((cluster - 2) * self.sectors_per_cluster) + self.data_start_sector
        self.disk.seek(cluster_offset * self.bytes_per_sector)
        content = self.disk.read(size)
        print(f"Content of file:\n{content.decode('ascii', errors='replace')}")

if __name__ == "__main__":
    disk_image_path = "disco1.img"
    fat16_disk = FAT16Disk(disk_image_path)
    fat16_disk.read_boot_sector()
