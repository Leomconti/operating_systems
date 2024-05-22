import datetime
import os
from io import BufferedRandom

"""
Blocks / Sectors should be interpreted as the same thing, different resources use different names.
Here we call them sectors. We'll use these names interchangeably.
http://www.maverick-os.dk/FileSystemFormats/FAT16_FileSystem.html
https://www.tavi.co.uk/phobos/fat.html
"""


class FAT16Disk:
    def __init__(self, image_path):
        self.image_path = image_path
        self.boot_sector = b""
        self.disk: BufferedRandom = self.open_disk(image_path)  # we open the disk image upon class instantiation
        self.bytes_per_sector = 0
        self.sectors_per_cluster = 0
        self.sectors_per_fat = 0
        self.root_dir_start_sector = 0
        self.data_start_sector = 0
        self.reserved_sectors = 1  # Most cases it should be exactly 1, but we'll calculate it anyways later
        self.files = []

    @staticmethod
    def open_disk(image_path):
        """
        Here we just open the disk as a byte array
        """
        try:
            return open(image_path, "rb+")
        except FileNotFoundError:
            print("File not found. Please check the path and try again.")
            raise FileNotFoundError
        except Exception as e:
            print(f"An error occurred: {e}")
            raise e

    def read_boot_sector(self):
        """
        Read the first 512 bytes of the disk image and store it in the boot_sector.
        Then we call the parse_boot_sector.
        """
        self.disk.seek(0)
        self.boot_sector = self.disk.read(512)
        self.parse_boot_sector()

    def parse_boot_sector(self):
        """
        Parse the boot sector and store the relevant information to the class.
        Everything related to the boot block mapping, that is in the slides.
        """
        self.bytes_per_sector = int.from_bytes(self.boot_sector[0x0B:0x0D], "little")
        self.sectors_per_cluster = self.boot_sector[0x0D]
        self.sectors_per_fat = int.from_bytes(self.boot_sector[0x16:0x18], "little")
        num_fats = self.boot_sector[0x10]
        num_root_dir_entries = int.from_bytes(self.boot_sector[0x11:0x13], "little")
        self.reserved_sectors = int.from_bytes(self.boot_sector[0x0E:0x10], "little")

        self.root_dir_start_sector = self.reserved_sectors + (
            num_fats * self.sectors_per_fat
        )  # (size of FAT)*(nro of FATs)+FAT REGION ( reserved sectors, should be 1! ) https://arc.net/l/quote/syrfiuts

        root_dir_bytes = num_root_dir_entries * 32  # 32 is the max, so we just multiply by 32
        root_dir_sectors = (root_dir_bytes + self.bytes_per_sector - 1) // self.bytes_per_sector

        self.data_start_sector = self.root_dir_start_sector + root_dir_sectors

        print(f"Data region starts at sector: {self.data_start_sector}")
        self.list_root_directory()

    def list_root_directory(self):
        """
        For this activity we won't go inside other directories, we'll just list the files in the root directory.
        And read each dir it has, without "recursively" going into them.
        """
        self.disk.seek(self.root_dir_start_sector * self.bytes_per_sector)  # Find the root directory
        for _ in range(32):  # Each of the entries in a directory list is 32 byte long
            entry = self.disk.read(32)  # Then we look 32 bytes to see what they have.
            self.parse_directory_entry(entry)  # entry is a file basically, or subdir, whatever

        # Now read the contents of the files
        self.read_files_content()

    def parse_directory_entry(self, entry):
        if entry[0] == 0x00:
            return  # No more entries
        if entry[0] == 0xE5:
            return  # Entry is deleted
        attrs = entry[11]
        if attrs & 0x08:  # Volume label, skip it
            return

        try:
            filename = entry[:8].decode("latin1").strip() + "." + entry[8:11].decode("latin1").strip()
            filename = filename.strip(".").lower()
        except UnicodeDecodeError:
            filename = "invalid_filename"  # just so we don't break the program for some ascii / latin issue

        first_cluster = int.from_bytes(entry[26:28], "little")
        filesize = int.from_bytes(entry[28:32], "little")

        self.print_file_metadata(entry, filename)

        # If the attribute is 0x10, it means it's a sub dir. https://arc.net/l/quote/orrpinhh
        if attrs & 0x10:
            print(f"Directory: {filename}")
        else:
            print(f"File: {filename}, Size: {filesize}, First Cluster: {first_cluster}")
            self.files.append((filename, first_cluster, filesize))

    def get_attributes_string(self, attrs):
        """Return a string describing the file attributes."""
        attributes = []
        if attrs & 0x01:
            attributes.append("Read-Only")
        if attrs & 0x02:
            attributes.append("Hidden")
        if attrs & 0x04:
            attributes.append("System")
        if attrs & 0x08:
            attributes.append("Volume Label")
        if attrs & 0x10:
            attributes.append("Directory")
        if attrs & 0x20:
            attributes.append("Archive")
        return ", ".join(attributes) if attributes else "None"

    def parse_date(self, date_bytes):
        """Parse a date from FAT16 format."""
        if len(date_bytes) < 2:
            return "Unknown"
        date_value = int.from_bytes(date_bytes, "little")
        year = ((date_value >> 9) & 0x7F) + 1980
        month = (date_value >> 5) & 0x0F
        day = date_value & 0x1F
        return f"{year:04}-{month:02}-{day:02}"

    def parse_time(self, time_bytes):
        """Parse a time from FAT16 format."""
        if len(time_bytes) < 2:
            return "Unknown"
        time_value = int.from_bytes(time_bytes, "little")
        hour = (time_value >> 11) & 0x1F
        minute = (time_value >> 5) & 0x3F
        second = (time_value & 0x1F) * 2
        return f"{hour:02}:{minute:02}:{second:02}"

    def print_file_metadata(self, entry, filename):
        """Print metadata for a given file entry."""
        attrs = entry[11]
        created_time = self.parse_time(entry[14:16])
        created_date = self.parse_date(entry[16:18])
        last_access_date = self.parse_date(entry[18:20])

        print(f"Metadata for file: {filename}")
        print(f"  Attributes: {self.get_attributes_string(attrs)}")
        print(f"  Created: {created_date} {created_time}")
        print(f"  Last Accessed: {last_access_date}")

    def read_files_content(self) -> None:
        """
        Passes through all files and calls the read_file_content method for each file.
        This is just a print method
        """
        for filename, first_cluster, filesize in self.files:
            print(f"Reading content of file: {filename}")
            self.read_file_content(first_cluster, filesize)

    def read_file_content(self, cluster, size) -> None:
        """
        Read the content of a file and print it to the terminal.
        """
        cluster_offset = ((cluster - 2) * self.sectors_per_cluster) + self.data_start_sector
        self.disk.seek(cluster_offset * self.bytes_per_sector)
        content = self.disk.read(size)
        # Currently this decodes text, if we want based on the image extension we can do so
        print(
            f"Content of file:\n{content.decode('latin1', errors='replace')}"
        )  # errors replace just in case it gets bugged by encoding / decoding

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
        clusters_needed = (size + (self.bytes_per_sector * self.sectors_per_cluster) - 1) // (
            self.bytes_per_sector * self.sectors_per_cluster
        )
        free_clusters = []
        # FAT starts immediately after reserved sectors
        self.disk.seek(self.bytes_per_sector)

        # Read entire FAT into memory for simplicity, assuming enough memory
        fat = self.disk.read(self.sectors_per_fat * self.bytes_per_sector)
        for cluster_number in range(2, len(fat) // 2):  # Cluster numbers start at 2
            entry = int.from_bytes(fat[2 * cluster_number : 2 * cluster_number + 2], "little")
            if entry == 0:  # Free cluster
                free_clusters.append(cluster_number)
                if len(free_clusters) == clusters_needed:
                    return free_clusters

        raise Exception("Not enough free clusters.")

    def insert_file(self, file_path):
        """Insert a file into our disk"""
        print("Inside Insert file")
        with open(file_path, "rb") as file:
            file_data = file.read()

        free_entry_index = self.find_free_directory_entry()
        print("found free directory entry:", free_entry_index)
        free_clusters = self.find_free_clusters(len(file_data))
        print("Found free clusters:", free_clusters)

        # Write file data to the first cluster and chain the rest
        for i, cluster in enumerate(free_clusters):
            cluster_offset = ((cluster - 2) * self.sectors_per_cluster + self.data_start_sector) * self.bytes_per_sector
            self.disk.seek(cluster_offset)
            # Basically, we split the file data into chunks of the size of a cluster
            start = i * self.bytes_per_sector * self.sectors_per_cluster
            end = (i + 1) * self.bytes_per_sector * self.sectors_per_cluster
            self.disk.write(file_data[start:end])  # write the chunk

        # Update entries to chain clusters
        for i in range(len(free_clusters) - 1):
            self.update_fat_entry(free_clusters[i], free_clusters[i + 1])
        self.update_fat_entry(
            free_clusters[-1], 0xFFFF
        )  # Mark the last one as the eof https://arc.net/l/quote/xnbyctou

        # Set file attributes
        file_attributes = 0x20  # Archive attribute

        # Set creation and access dates/times to now
        now = datetime.datetime.now()
        creation_time = self.create_time(now.hour, now.minute, now.second)
        creation_date = self.create_date(now.year, now.month, now.day)
        access_date = creation_date

        # update the directory entry, with metadata etc
        self.update_directory_entry(
            free_entry_index,
            file_path,
            free_clusters[0],
            len(file_data),
            file_attributes,
            creation_time,
            creation_date,
            access_date,
        )

    def update_fat_entry(self, cluster, value):
        """Update a FAT entry."""
        print("Updating fat entry")
        fat_offset = self.reserved_sectors * self.bytes_per_sector + cluster * 2
        self.disk.seek(fat_offset)
        self.disk.write(value.to_bytes(2, "little"))

    def update_directory_entry(
        self, index, file_path, start_cluster, size, attrs, creation_time, creation_date, access_date
    ):
        """Update a directory entry with a new file."""
        print("Updating directory entry")
        directory_offset = self.root_dir_start_sector * self.bytes_per_sector + index * 32
        self.disk.seek(directory_offset)
        filename, ext = os.path.splitext(os.path.basename(file_path))
        filename = filename.upper()[:8].ljust(8)
        ext = ext.replace(".", "").upper()[:3].ljust(3)
        entry = bytearray(
            filename.encode("latin1")
            + ext.encode("latin1")
            + attrs.to_bytes(1, "little")  # write the file attributes
            + b"\x00" * 8  # Reserved/Unused space
            + creation_time
            + creation_date
            + access_date
            + start_cluster.to_bytes(2, "little")
            + size.to_bytes(4, "little")
            + b"\x00" * 2  # Reserved/Unused space
        )
        self.disk.write(entry)

    def find_file(self, file_name):
        """Find a file in the FAT16 disk image and return its directory entry index, first cluster, and size."""
        file_name = file_name.lower()

        self.disk.seek(self.root_dir_start_sector * self.bytes_per_sector)
        for i in range(512):
            entry = self.disk.read(32)
            if entry[0] == 0x00:
                break  # No more entries
            if entry[0] == 0xE5:
                continue  # Entry is deleted

            attrs = entry[11]
            if attrs & 0x08:
                continue  # Skip volume label

            try:
                filename = entry[:8].decode("latin1").strip() + "." + entry[8:11].decode("latin1").strip()
                filename = filename.strip(".").lower()
            except UnicodeDecodeError:
                continue

            if filename == file_name:
                first_cluster = int.from_bytes(entry[26:28], "little")
                filesize = int.from_bytes(entry[28:32], "little")
                return i, first_cluster, filesize

        raise FileNotFoundError(f"File {file_name} not found in the root directory.")

    def rename_file(self, old_name, new_name):
        """
        Update a directory entry with a new file name.
        In other words, rename file from a directory
        """
        try:
            index, _, _ = self.find_file(old_name)
        except FileNotFoundError:
            print(f"File {old_name} not found in the root directory. Ending rename operation...")
            return

        directory_offset = self.root_dir_start_sector * self.bytes_per_sector + index * 32
        self.disk.seek(directory_offset)

        filename, ext = os.path.splitext(new_name)
        filename = filename.upper()[:8].ljust(8)  # make exactly 8 characters
        ext = (
            ext.replace(".", "").upper()[:3].ljust(3)  # Max 3 chars
        )  # this is just to make sure extension also is updated in case the user renames with a different extension. We copy the one from the filename, and standardize it.

        # read the current entry
        entry = bytearray(
            self.disk.read(32)
        )  # this is so we can mutate the array and therefore update the filename and ext

        # Update the name and extension in the entry
        entry[:8] = filename.encode("latin1")
        entry[8:11] = ext.encode("latin1")

        self.disk.seek(directory_offset)  # go to the offset we started
        self.disk.write(entry)  # this basically overwrites the old entry with the new one, updated

    def delete_file(self, file_name):
        """
        Delete a file from the disk image, by filename.
        We just need to remove the reference from the table and mark its clusters as free.
        """
        print("Deleting file...")
        try:
            index, first_cluster, _ = self.find_file(file_name)
            # mark the directory entry as deleted
            self.mark_directory_entry_deleted(index)
            # free the clusters used by the file
            self.free_clusters(first_cluster)
        except FileNotFoundError as e:
            print(e)

    def mark_directory_entry_deleted(self, index):
        """Mark a directory entry as deleted."""
        directory_offset = self.root_dir_start_sector * self.bytes_per_sector + index * 32
        self.disk.seek(directory_offset)
        self.disk.write(b"\xe5")  # Mark the first byte of the entry as 0xE5 (deleted)

    def free_clusters(self, cluster):
        """Free the clusters used by a file in the FAT."""
        while cluster < 0xFFF8:  # While the cluster is not the end-of-file marker
            fat_offset = self.reserved_sectors * self.bytes_per_sector + cluster * 2
            self.disk.seek(fat_offset)
            next_cluster = int.from_bytes(self.disk.read(2), "little")
            self.disk.seek(fat_offset)
            self.disk.write(b"\x00\x00")  # Mark the cluster as free
            cluster = next_cluster

    def create_time(self, hour, minute, second):
        """Create a FAT16 time value."""
        return ((hour << 11) | (minute << 5) | (second // 2)).to_bytes(2, "little")

    def create_date(self, year, month, day):
        """Create a FAT16 date value."""
        year = year - 1980
        return ((year << 9) | (month << 5) | day).to_bytes(2, "little")


if __name__ == "__main__":
    disk_image_path = "disk_new_new.img"
    fat16_disk = FAT16Disk(disk_image_path)  # first one always
    fat16_disk.read_boot_sector()  # Needs to come here before inserting a file
    # fat16_disk.insert_file("abacata.txt")
    # fat16_disk.read_boot_sector()  # Needs to come here before inserting a file
    # fat16_disk.rename_file("abacata.txt", "abacaxxx.txt")
    # fat16_disk.delete_file("abacaxxx.txt")
    # fat16_disk.delete_file("texto.txt")
    # fat16_disk.insert_file("nao_leo.txt")
    # fat16_disk.insert_file("profile.jpeg")
    # fat16_disk.insert_file("banana.txt")
    # print("Renaming file")
    # fat16_disk.rename_file
