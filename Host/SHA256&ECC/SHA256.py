import hashlib
import struct
def create_hash_file(firmware_path, out_hash_path):
    with open(firmware_path, 'rb') as f:
        firmware = f.read()

    firmware_len = len(firmware)
    hash32 = hashlib.sha256(firmware).digest()

    # Ghi: 4 byte firmware_len (little-endian) + 32 byte SHA256
    with open(out_hash_path, 'wb') as f:
        f.write(struct.pack('<I', firmware_len))  # 4 bytes
        f.write(hash32)                           # 32 bytes
create_hash_file(
    r"C:\Users\ASUS\Desktop\Host_program\SHA256&ECC\Backup_Firmware.bin",
    r"C:\Users\ASUS\Desktop\Host_program\Backup_Firmware_SHA256_no_metadata.bin"
)

