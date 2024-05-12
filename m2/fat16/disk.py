import os


class FAT16Disk:
    def __init__(self, image_path):
        self.image_path = image_path
        self.boot_sector = b''
        self.disk = None
        self.bytes_per_sector = 0
        self.sectors_per_cluster = 0
        self.sectors_per_fat = 0
        self.root_dir_start_sector = 0
        self.data_start_sector = 0
        self.reserved_sectors = 0

    def open_disk(self):
        try:
            self.disk = open(self.image_path, 'rb+')
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
        self.reserved_sectors = int.from_bytes(self.boot_sector[0x0e:0x10], 'little')

        self.root_dir_start_sector = self.reserved_sectors + (num_fats * self.sectors_per_fat)
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
    
    def find_free_directory_entry(self):
        """Find a free directory entry in the root directory."""
        self.disk.seek(self.root_dir_start_sector * self.bytes_per_sector)
        for i in range(512):  # Assumes a maximum of 512 entries in the root directory
            entry = self.disk.read(32)
            if entry[0] in (0x00, 0xE5):  # 0x00 = unused, 0xE5 = previously deleted
                return i  # Return the index of the free entry
        raise Exception("No free directory entries available.")

    def find_free_clusters(self, size):
        """Find enough free clusters to store the file of a given size."""
        clusters_needed = (size + (self.bytes_per_sector * self.sectors_per_cluster) - 1) // (self.bytes_per_sector * self.sectors_per_cluster)
        free_clusters = []
        # FAT starts immediately after reserved sectors
        self.disk.seek(self.bytes_per_sector * (self.reserved_sectors))

        # Read entire FAT into memory for simplicity, assuming enough memory
        fat = self.disk.read(self.sectors_per_fat * self.bytes_per_sector)
        for cluster_number in range(2, len(fat) // 2):  # Cluster numbers start at 2
            entry = int.from_bytes(fat[2 * cluster_number:2 * cluster_number + 2], 'little')
            if entry == 0:  # Free cluster
                free_clusters.append(cluster_number)
                if len(free_clusters) == clusters_needed:
                    return free_clusters

        raise Exception("Not enough free clusters.")

    def insert_file(self, file_path):
        """Insert a file into the FAT16 disk image."""
        print("Inside Insert file")
        with open(file_path, 'rb') as file:
            file_data = file.read()

        free_entry_index = self.find_free_directory_entry()
        print("found free directory entry:", free_entry_index)
        free_clusters = self.find_free_clusters(len(file_data))
        print("Found free clusters:", free_clusters)

        # Write file data to the first cluster and chain the rest
        for i, cluster in enumerate(free_clusters):
            cluster_offset = ((cluster - 2) * self.sectors_per_cluster + self.data_start_sector) * self.bytes_per_sector
            self.disk.seek(cluster_offset)
            self.disk.write(file_data[i * self.bytes_per_sector * self.sectors_per_cluster:(i + 1) * self.bytes_per_sector * self.sectors_per_cluster])

        # Update FAT entries to chain clusters
        for i in range(len(free_clusters) - 1):
            self.update_fat_entry(free_clusters[i], free_clusters[i + 1])
        self.update_fat_entry(free_clusters[-1], 0xFFFF)  # Mark the last cluster in the chain

        # Update the directory entry
        self.update_directory_entry(free_entry_index, file_path, free_clusters[0], len(file_data))

    def update_fat_entry(self, cluster, value):
        """Update a FAT entry."""
        print("Updating fat entry")
        fat_offset = self.reserved_sectors * self.bytes_per_sector + cluster * 2
        self.disk.seek(fat_offset)
        self.disk.write(value.to_bytes(2, 'little'))

    def update_directory_entry(self, index, file_path, start_cluster, size):
        """Update a directory entry with a new file."""
        print("Updating directory entry")
        directory_offset = self.root_dir_start_sector * self.bytes_per_sector + index * 32
        self.disk.seek(directory_offset)
        filename, ext = os.path.splitext(os.path.basename(file_path))
        filename:str = filename.upper()[:8].ljust(8)
        ext = ext.replace('.', '').upper()[:3].ljust(3)
        entry = bytearray(filename.encode('ascii') + ext.encode('ascii') + b'\x20' + b'\x00' * 14 + start_cluster.to_bytes(2, 'little') + size.to_bytes(4, 'little') + b'\x00' * 6)
        self.disk.write(entry)

if __name__ == "__main__":
    disk_image_path = "disco1.img"
    fat16_disk = FAT16Disk(disk_image_path)
    fat16_disk.read_boot_sector()
    print("Inserting file")
    # fat16_disk.insert_file ("leo.txt")
