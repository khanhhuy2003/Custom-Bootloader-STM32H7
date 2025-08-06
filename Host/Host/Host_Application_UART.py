import struct
import serial
import time
import os
import zlib
import threading
import sys
import queue
from datetime import datetime
from serial.tools import list_ports
from colorama import init, Fore, Style
from tqdm import tqdm

# Initialize colorama for colored output
init(autoreset=True)

# === UART Configuration ===
COM_PORT = None
BAUDRATE = 115200
TIMEOUT = 3
serial_port = None
listen_thread = None
running = True
serial_lock = threading.Lock()
message_queue = queue.Queue()  # Queue for STM32 messages

# === Command code ===
BL_CHECK_CONNECTION = 0x50
BL_WRITE_MEM = 0x51
BL_JUMP = 0x52
BL_CHECK_MEM = 0x53
BL_DEL_MEM = 0x54
BL_JUMP_TO_CODE = 0x55
USER_JUMP_TO_BOOTLOADER = 0x56
SEND_HASH_AND_VERIFY = 0x58
PREVIOUS_VERSIONS_IN_STM32 =0x60

# === Helper Functions for Pretty Printing ===
def print_success(msg):
    print(f"{Fore.GREEN}{Style.BRIGHT}✅ {msg}{Style.RESET_ALL}")

def print_error(msg):
    print(f"{Fore.RED}{Style.BRIGHT}❌ {msg}{Style.RESET_ALL}")

def print_warning(msg):
    print(f"{Fore.YELLOW}{Style.BRIGHT}⚠️ {msg}{Style.RESET_ALL}")

def print_info(msg):
    print(f"{Fore.CYAN}{Style.BRIGHT}📬 {msg}{Style.RESET_ALL}")

# === Print STM32 messages from queue ===
def format_version(version_int):
    """Chuyển uint32 version (vd: 0x02010000) thành chuỗi '2.1.0.0'"""
    major = (version_int >> 24) & 0xFF
    minor = (version_int >> 16) & 0xFF
    patch = (version_int >> 8) & 0xFF
    build = version_int & 0xFF
    return f"{major}.{minor}.{patch}.{build}"
def print_queued_messages():
    while not message_queue.empty():
        msg = message_queue.get()
        
        # Nếu là số nguyên (version dưới dạng int)
        if msg.isdigit():
            try:
                version_int = int(msg)
                version_str = format_version(version_int)
                print_success(f"📦 Version saved: {version_str} ({version_int})")
                continue
            except:
                pass

        # Nếu là hex dạng 0x....
        if msg.startswith("0x"):
            try:
                version_int = int(msg, 16)
                version_str = format_version(version_int)
                print_success(f"📦 Version saved: {version_str} ({msg})")
                continue
            except:
                pass

        # Còn lại xử lý bình thường
        if "HardFault" in msg or "BusFault" in msg:
            print_error(f"STM32 Fault: {msg}")
        elif "Erase User code status flag OK" in msg:
            print_success(f"STM32: {msg}")
        else:
            print_info(f"STM32: {msg}")

# === Automatic scan COM port ===
def auto_select_com_port():
    ports = list_ports.comports()
    available_ports = [port.device for port in ports]

    if not available_ports:
        print_error("No COM ports found.")
        sys.exit(1)

    print(f"{Fore.CYAN}{Style.BRIGHT}🔌 Available COM ports:{Style.RESET_ALL}")
    for i, p in enumerate(available_ports):
        print(f"  {i + 1}. {p}")

    if len(available_ports) == 1:
        print_success(f"Selected COM port: {available_ports[0]}")
        return available_ports[0]

    while True:
        try:
            choice = int(input(f"{Fore.CYAN}👉 Select COM port (1-{len(available_ports)}): {Style.RESET_ALL}"))
            if 1 <= choice <= len(available_ports):
                return available_ports[choice - 1]
        except:
            pass
        print_error("Invalid selection. Please try again.")

# === UART listener thread ===
def uart_listener():
    global serial_port, running, serial_lock
    while running:
        try:
            if serial_port and serial_port.is_open:
                with serial_lock:
                    if serial_port.in_waiting > 0:
                        resp = serial_port.readline()
                        if resp:
                            try:
                                decoded = resp.decode('utf-8').strip()
                                message_queue.put(decoded)  # Add to queue
                            except UnicodeDecodeError:
                                message_queue.put(f"Raw: {resp.hex()}")
        except serial.SerialException as e:
            message_queue.put(f"UART error in listener: {e}")
            time.sleep(1)
        except Exception as e:
            message_queue.put(f"Unexpected error in listener: {e}")
            time.sleep(1)

# === Send command via UART ===
def send_command(cmd_code, data: bytes):
    global serial_port, serial_lock
    ltf = 1 + len(data) + 4  # cmd + data + CRC
    frame = bytes([ltf]) + bytes([cmd_code]) + data
    crc = zlib.crc32(frame)
    frame += struct.pack('<I', crc)

    try:
        with serial_lock:
            if not serial_port or not serial_port.is_open:
                serial_port = serial.Serial(
                    COM_PORT,
                    BAUDRATE,
                    timeout=TIMEOUT,
                    parity=serial.PARITY_NONE,
                    stopbits=serial.STOPBITS_ONE,
                    bytesize=serial.EIGHTBITS
                )
                serial_port.reset_input_buffer()
                serial_port.reset_output_buffer()

            print_info(f"Sending command (code: 0x{cmd_code:02X})...")
            serial_port.write(frame)
            time.sleep(0.01)
            resp = serial_port.readline()
            if resp:
                try:
                    decoded = resp.decode('utf-8').strip()
                    print_success(f"Command response: {decoded}")
                except UnicodeDecodeError:
                    print_success(f"Command response (raw): {resp.hex()}")
            else:
                print_warning("No response from STM32 for command.")
    except serial.SerialException as e:
        print_error(f"UART error: {e}")
        if serial_port and serial_port.is_open:
            serial_port.close()
            serial_port = None
    except Exception as e:
        print_error(f"Unexpected error in send_command: {e}")

# === Send firmware ===
def send_firmware(firmware_path, start_addr):
    if not os.path.exists(firmware_path):
        print_error("Firmware file does not exist.")
        return

    with open(firmware_path, 'rb') as f:
        firmware = f.read()

    total_len = len(firmware)
    print_info(f"Firmware size: {total_len} bytes")

    chunk_size = 224
    bytes_sent = 0
    total_chunks = (total_len + chunk_size - 1) // chunk_size

    with tqdm(total=total_len, unit='B', unit_scale=True, desc="Uploading firmware") as pbar:
        chunk_index = 0
        while bytes_sent < total_len:
            addr = start_addr + bytes_sent
            chunk = firmware[bytes_sent:bytes_sent + chunk_size]

            if len(chunk) < chunk_size:
                chunk += b'\xFF' * (chunk_size - len(chunk))

            if addr % 32 != 0:
                print_error(f"Address not 32-byte aligned: 0x{addr:08X}")
                return

            addr_bytes = struct.pack('<I', addr)
            size_bytes = struct.pack('<B', chunk_size)
            payload = addr_bytes + size_bytes + chunk

            print_info(f"[{chunk_index:03}] Sending chunk at 0x{addr:08X} ({len(chunk)} bytes)")
            send_command(BL_WRITE_MEM, payload)

            bytes_sent += chunk_size
            chunk_index += 1
            pbar.update(len(chunk))
            time.sleep(0.005)

    print_success("Firmware uploaded successfully.")

# === Send signature and hash ===
def send_signature_and_hash(hash_path, signature_path, app_addr):
    if not os.path.exists(signature_path) or not os.path.exists(hash_path):
        print_error("Hash or signature file does not exist.")
        return

    with open(hash_path, 'rb') as f:
        data = f.read()
    firmware_len = struct.unpack('I', data[:4])[0]
    hash32 = data[4:36]

    with open(signature_path, 'rb') as f:
        signature = f.read()
    if len(signature) != 64:
        print_error("Invalid signature (must be 64 bytes).")
        return

    payload = struct.pack('<I', app_addr) + struct.pack('<I', firmware_len) + hash32 + signature
    print_info("Sending hash and signature for verification...")
    send_command(SEND_HASH_AND_VERIFY, payload)

def select_app_slot():
    while True:
        slot = input(f"{Fore.CYAN}Enter app slot (1 or 2): {Style.RESET_ALL}").strip()
        if slot == "1":
            return 1, 0x08020000  # APP1
        elif slot == "2":
            return 2, 0x08060000  # APP2
        else:
            print_error("Invalid slot. Please enter 1 or 2.")
# === Interactive Menu ===
def menu():
    global running, serial_port, listen_thread
    try:
        while True:
            # Print queued STM32 messages before menu
            print_queued_messages()

            # Print menu with timestamp
            print(f"\n{Fore.CYAN}{Style.BRIGHT}===== UART BOOTLOADER MENU ({datetime.now().strftime('%Y-%m-%d %H:%M:%S')}) =====")
            print(f"{'Option':<9} {'Command':<25} {'Description'}")
            print(f"{'-' * 60}")
            print(f"{'1':<9} {'CHECK_CONNECTION':<25} Check host-bootloader connection")
            print(f"{'2':<9} {'WRITE_MEM_SEG_1':<25} Update firmware to STM32")
            print(f"{'3':<9} {'DEL_MEM':<25} Erase STM32 flash memory")
            print(f"{'4':<9} {'CHECK_MEM_USAGE':<25} Check flash memory usage")
            print(f"{'5':<9} {'JUMP_TO_CODE':<25} Execute user application")
            print(f"{'6':<9} {'USER_JUMP_TO_BOOTLOADER':<25} Return to bootloader")
            print(f"{'7':<9} {'Authentication':<25} Verify firmware integrity")
            print(f"{'8':<9} {'PREVIOUS VERSION IN STM32':<25} Check the previous version used to be saved in STM32")
            print(f"{'9999':<9} {'Exit':<25} Exit the program")
            print(f"{Style.RESET_ALL}")

            # Input with timeout to avoid blocking
            choice = input(f"{Fore.CYAN}👉 Enter choice: {Style.RESET_ALL}")
            print_queued_messages()  # Print any new STM32 messages after input

            if choice == "1":
                print_info("Checking connection...")
                send_command(BL_CHECK_CONNECTION, b'')

            elif choice == "2":
                try:
                    firmware_path = input(f"{Fore.CYAN}Enter firmware file path (.bin): {Style.RESET_ALL}").strip()
                    if not os.path.exists(firmware_path):
                        print_error("Firmware file does not exist.")
                        continue

                    slot, addr = select_app_slot()
                    print_info(f"Selected APP{slot} at 0x{addr:08X}")
                    send_firmware(firmware_path, addr)

                except Exception as e:
                    print_error(f"Invalid input or error: {e}")


            elif choice == "3":
                slot, addr = select_app_slot()
                print_info(f"Selected APP{slot} at 0x{addr:08X}")
                addr_bytes = struct.pack('<I', addr)
                print(addr_bytes)
                send_command(BL_DEL_MEM, addr_bytes)

            elif choice == "4":
                print_info("Checking flash memory usage...")
                send_command(BL_CHECK_MEM, b'')

            elif choice == "5":
                print_info("Preparing to jump to user code...")
                send_command(BL_JUMP_TO_CODE, b'')

            elif choice == "6":
                print_info("Requesting return to bootloader...")
                send_command(USER_JUMP_TO_BOOTLOADER, b'')

            elif choice == "7":
                slot, addr = select_app_slot()
                hash_path = input(f"{Fore.CYAN}Enter hash file path (.bin): {Style.RESET_ALL}").strip()
                signature_path = input(f"{Fore.CYAN}Enter signature file path (.bin): {Style.RESET_ALL}").strip()
                print_info(f"Selected APP{slot} at 0x{addr:08X}")
                send_signature_and_hash(hash_path, signature_path, addr)
            elif choice == "8":
                send_command(PREVIOUS_VERSIONS_IN_STM32, b'')

            elif choice == "9999":
                print_success("Exiting program...")
                running = False
                if listen_thread:
                    listen_thread.join()
                if serial_port and serial_port.is_open:
                    serial_port.close()
                break
            else:
                print_error("Invalid choice. Please try again.")
    except KeyboardInterrupt:
        print_success("\nInterrupted by user. Exiting...")
        running = False
        if listen_thread:
            listen_thread.join()
        if serial_port and serial_port.is_open:
            serial_port.close()

if __name__ == "__main__":
    COM_PORT = auto_select_com_port()
    listen_thread = threading.Thread(target=uart_listener, daemon=True)
    listen_thread.start()
    menu()