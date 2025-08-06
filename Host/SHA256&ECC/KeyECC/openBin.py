from cryptography.hazmat.primitives.asymmetric import ec
# Load PEM public key
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.backends import default_backend
with open("public_key.pem", "rb") as f:
    pubkey = serialization.load_pem_public_key(f.read(), backend=default_backend())

# Lấy X, Y
public_numbers = pubkey.public_numbers()
x = public_numbers.x.to_bytes(32, byteorder="big")
y = public_numbers.y.to_bytes(32, byteorder="big")

public_key_raw = x + y  # Dạng 64 byte: X||Y
print("Public key (raw, hex):", public_key_raw.hex())