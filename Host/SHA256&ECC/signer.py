from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric.utils import decode_dss_signature

# Load private key
with open("private_key.pem", "rb") as f:
    private_key = serialization.load_pem_private_key(f.read(), password=None)

# Load firmware (giả lập file nhị phân)
with open(r"C:\Users\ASUS\Desktop\Host_program\SHA256&ECC\Backup_Firmware.bin", "rb") as f:
    firmware = f.read()

# Tạo chữ ký dạng DER (mặc định)
signature_der = private_key.sign(firmware, ec.ECDSA(hashes.SHA256()))

# Giải mã DER → lấy r, s
r, s = decode_dss_signature(signature_der)
r_bytes = r.to_bytes(32, byteorder="big")
s_bytes = s.to_bytes(32, byteorder="big")

# Gộp r || s thành 64 byte signature
signature_raw = r_bytes + s_bytes

# Ghi ra file signature dạng thô (64 byte)
with open("Backup_Firmware_signature_no_metadata.bin", "wb") as f:
    f.write(signature_raw)

print("✅ Đã tạo chữ ký (r||s) 64 byte: signature.bin")
print("Kích thước:", len(signature_raw), "byte")
