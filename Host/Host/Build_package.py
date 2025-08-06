import os
import struct
from pathlib import Path
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric.utils import decode_dss_signature
import hashlib


# === AES Configuration ===
AES_KEY = bytes.fromhex("2B7E151628AED2A6ABF7158809CF4F3C")
AES_IV  = bytes.fromhex("000102030405060708090A0B0C0D0E0F")


def parse_version_string(version_str: str) -> int:
    parts = version_str.strip().split(".")
    if len(parts) != 4:
        raise ValueError("❌ Version phải có dạng x.x.x.x (4 phần)")
    version_bytes = [int(p) for p in parts]
    if any(b < 0 or b > 255 for b in version_bytes):
        raise ValueError("❌ Mỗi phần version phải nằm trong [0,255]")
    return (version_bytes[0] << 24) | (version_bytes[1] << 16) | (version_bytes[2] << 8) | version_bytes[3]


def generate_final_firmware_and_signatures(raw_firmware_path, private_key_path, version_hex, output_dir, metadata_pad=224):
    # === Bước 1: Mã hóa firmware
    with open(raw_firmware_path, 'rb') as f:
        raw_data = f.read()
    firmware_size_raw = len(raw_data)
    padded_data = pad(raw_data, 16)
    cipher = AES.new(AES_KEY, AES.MODE_CBC, AES_IV)
    encrypted_data = cipher.encrypt(padded_data)
    firmware_size = len(encrypted_data)

    # === Bước 2: Tạo metadata (version + firmware length)
    metadata = struct.pack('<II', version_hex, firmware_size)

    # === Bước 3: Load private key
    with open(private_key_path, 'rb') as f:
        private_key = serialization.load_pem_private_key(f.read(), password=None)

    # === Bước 4: Ký metadata
    signature_metadata_der = private_key.sign(metadata, ec.ECDSA(hashes.SHA256()))
    r_meta, s_meta = decode_dss_signature(signature_metadata_der)
    signature_metadata = r_meta.to_bytes(32, 'big') + s_meta.to_bytes(32, 'big')

    # === Bước 5: Gộp metadata + padding
    full_metadata = metadata + signature_metadata
    if len(full_metadata) > metadata_pad:
        raise ValueError(f"❌ Metadata + chữ ký ({len(full_metadata)} bytes) vượt quá {metadata_pad} bytes")
    padded_metadata = full_metadata + b'\xFF' * (metadata_pad - len(full_metadata))

    # === Bước 6: Gộp final.bin
    final_path = Path(output_dir) / f"{Path(raw_firmware_path).stem}.final.bin"
    with open(final_path, 'wb') as f:
        f.write(padded_metadata)
        f.write(encrypted_data)

    # === Bước 7: Tạo SHA256 của firmware mã hóa
    sha256_hash = hashlib.sha256(raw_data).digest()
    hash_path = Path(output_dir) / f"{Path(raw_firmware_path).stem}.sha256.bin"
    with open(hash_path, 'wb') as f:
        f.write(struct.pack('<I', firmware_size_raw))  # 4 bytes
        f.write(sha256_hash)                       # 32 bytes

    # === Bước 8: Ký ECC của firmware mã hóa
    signature_fw_der = private_key.sign(raw_data, ec.ECDSA(hashes.SHA256()))
    r_fw, s_fw = decode_dss_signature(signature_fw_der)
    signature_fw = r_fw.to_bytes(32, 'big') + s_fw.to_bytes(32, 'big')
    signature_path = Path(output_dir) / f"{Path(raw_firmware_path).stem}.signature.bin"
    with open(signature_path, 'wb') as f:
        f.write(signature_fw)

    # === In thông tin
    print(f"\n✅ Đã tạo 3 file đầu ra trong: {Path(output_dir).resolve()}")
    print(f"📦 1. {final_path.name}         ({os.path.getsize(final_path)} bytes)")
    print(f"🔐 2. {hash_path.name}         ({os.path.getsize(hash_path)} bytes)")
    print(f"✍️ 3. {signature_path.name}     ({os.path.getsize(signature_path)} bytes)")
    print(f"\n🧩 [metadata {metadata_pad} bytes] + [firmware mã hóa {firmware_size} bytes]")
    print(f"🔢 Phiên bản: 0x{version_hex:08X} ({version_hex >> 24}.{(version_hex >> 16) & 0xFF}.{(version_hex >> 8) & 0xFF}.{version_hex & 0xFF})")


# === Người dùng nhập vào
if __name__ == "__main__":
    raw_fw = input("📄 Nhập đường dẫn firmware gốc (.bin): ").strip('" ')
    if not os.path.exists(raw_fw):
        print("❌ File firmware không tồn tại.")
        exit(1)

    version_str = input("🔢 Nhập version (dạng x.x.x.x, ví dụ: 1.2.3.4): ").strip()
    try:
        version_hex = parse_version_string(version_str)
    except Exception as e:
        print(f"❌ Lỗi: {e}")
        exit(1)

    private_key_path = r"C:\Users\ASUS\Desktop\Host_program\SHA256&ECC\KeyECC\private_key.pem"
    if not os.path.exists(private_key_path):
        print("❌ File private key không tồn tại.")
        exit(1)

    output_dir = Path(raw_fw).parent
    generate_final_firmware_and_signatures(raw_fw, private_key_path, version_hex, output_dir)
