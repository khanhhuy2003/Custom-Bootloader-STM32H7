In this project, I designed and implemented a **custom bootloader** and **PC host programs** that meet industrial standards for **security** and **reliability**.

## Bootloader Features
- **Connection check** between bootloader and host, including display of all previously stored firmware versions in flash.
- **OTA firmware update** via **UART**, with chunked transfer support.
- **Memory usage inspection** to display FLASH utilization across predefined segments.
- **Secure Boot Implementation**:
  - **AES-128 encryption/decryption** for firmware confidentiality.
  - **SHA-256 hashing** for integrity verification.
  - **ECC digital signature verification** (using ECDSA with P-256 curve) for authenticity.

##  PC Host Tools

### 1. Firmware Packager Tool

- Reads a firmware binary file and generates a secure package.
- Applies:
  - ECC digital signature (using private key)
  - SHA-256 hashing
  - AES-128 encryption in CBC mode
- Outputs encrypted and signed firmware ready for OTA.

### 2. Bootloader Communication Tool

- Communicates with the bootloader over UART.
- Sends the firmware package in secure chunks.
- Handles command codes: check connection, write memory, jump to application, etc.
- Displays status messages and progress.

