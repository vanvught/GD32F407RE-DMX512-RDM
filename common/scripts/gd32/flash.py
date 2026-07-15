#!/usr/bin/env python3

import argparse
import json
import sys
import time
from pathlib import Path

import serial

def load_gd32_db(filename="gd32.json"):
    path = Path(__file__).resolve().parent / filename

    if not path.is_file():
        print(f"Warning: device database '{path}' not found.")
        return {}

    try:
        with path.open("r", encoding="utf-8") as f:
            return json.load(f)
    except json.JSONDecodeError as e:
        print(f"Error parsing '{path}': {e}")
    except OSError as e:
        print(f"Error reading '{path}': {e}")

    return {}

def normalize_chip_id(chip_id):
    chip_id = str(chip_id).lower()
    if chip_id.startswith("0x"):
        chip_id = chip_id[2:]
    return chip_id.zfill(4)


def db_get_family(db, chip_id):
    chip_id = normalize_chip_id(chip_id)

    for key, value in db.items():
        if normalize_chip_id(key) == chip_id:
            return value

    return None

def lookup_device(db, chip_id, flash_size_kb, flasher=None):
    family = db_get_family(db, chip_id)
    if family is None:
        return {
            "identifier": "Unknown",
            "series": "Unknown",
            "part_number": "Unknown",
        }

    identifier = None
    device = family

    if flasher is not None:
        identifier = flasher.get_identifier()

    identifiers = family.get("identifiers", {})
    if identifier is not None and isinstance(identifiers, dict):
        device = identifiers.get(identifier, family)

    series = device.get("series", family.get("series", "Unknown"))
    flash = device.get("flash", family.get("flash", {}))
    part_number = flash.get(str(flash_size_kb), "Unknown")

    return {
        "identifier": identifier or "Unknown",
        "series": series,
        "part_number": part_number,
    }


class GD32Flasher:
    ACK = 0x79
    NACK = 0x1F

    CMD_GET = 0x00
    CMD_GET_VERSION = 0x01
    CMD_GET_ID = 0x02
    CMD_GET_IDENTIFIER = 0x06
    CMD_READ_MEMORY = 0x11
    CMD_GO = 0x21
    CMD_WRITE_MEMORY = 0x31
    CMD_ERASE = 0x43
    CMD_EXTENDED_ERASE = 0x44

    def __init__(self, port, baud=57600, timeout=5):
        self.port_name = port
        self.baud = baud
        self.timeout = timeout
        self.port = None

    def open(self):
        self.port = serial.Serial(
            self.port_name,
            self.baud,
            parity=serial.PARITY_EVEN,
            timeout=self.timeout,
        )
        self.port.dtr = False
        self.port.rts = False
        time.sleep(0.1)

    def close(self):
        if self.port:
            self.port.dtr = False
            self.port.rts = True
            time.sleep(0.1)
            self.port.rts = False
            self.port.close()
            print("Done. Reset the MCU manually before the next bootloader session.")

    def enter_bootloader(self):
        print("Entering bootloader mode...")

        for dtr_state in [True, False]:
            self.port.dtr = dtr_state
            time.sleep(0.05)

            self.port.rts = True
            time.sleep(0.1)
            self.port.rts = False
            time.sleep(0.3)

            self.port.reset_input_buffer()

            if self._try_sync():
                return True

        print("  FAILED: No bootloader response")
        return False

    def _try_sync(self, attempts=3):
        original_timeout = self.port.timeout

        try:
            for attempt in range(1, attempts + 1):
                self.port.reset_input_buffer()

                print(f"  Synchronization attempt {attempt}")
                self.port.write(b"\x7F")
                self.port.flush()

                deadline = time.monotonic() + 1.0

                while time.monotonic() < deadline:
                    self.port.timeout = max(
                        deadline - time.monotonic(),
                        0.01
                    )

                    response = self.port.read(1)
                    if not response:
                        break

                    value = response[0]
                    print(f"  Synchronization RX: 0x{value:02X}")

                    if value == self.ACK:
                        return True

                    if value == self.NACK:
                        break

                    if value == 0x7F:
                        # Possible local echo.
                        continue

                time.sleep(0.05)

            return False

        finally:
            self.port.timeout = original_timeout

    def _send_command(self, cmd):
        command = bytes([cmd, cmd ^ 0xFF])

        self.port.write(command)
        self.port.flush()

        response = self.port.read(1)

        if not response:
            print(f"  Command 0x{cmd:02X}: timeout")
            return False

        if response[0] == self.ACK:
            return True

        if response[0] == self.NACK:
            print(f"  Command 0x{cmd:02X}: NACK")
        else:
            print(
                f"  Command 0x{cmd:02X}: unexpected response"
            )

        return False

    def _wait_ack(self):
        resp = self.port.read(1)
        return resp and resp[0] == self.ACK

    def get_version(self):
        if not self._send_command(self.CMD_GET_VERSION):
            return None

        version_data = self.port.read(1)
        if len(version_data) != 1:
            return None

        version = version_data[0]
        self.port.read(2)
        self._wait_ack()
        return version

    def get_id(self):
        if not self._send_command(self.CMD_GET_ID):
            return None

        n_data = self.port.read(1)
        if len(n_data) != 1:
            return None

        n = n_data[0]
        chip_id = self.port.read(n + 1)
        if len(chip_id) != n + 1:
            return None

        self._wait_ack()
        return chip_id.hex()

    def get_identifier(self):
        """Return the four-character GD32 device identifier.

        Command 0x06 may return more than four payload bytes on newer
        devices. The first four bytes contain the printable identifier;
        any remaining bytes are vendor-specific extension data.
        """
        if not self._send_command(self.CMD_GET_IDENTIFIER):
            return None

        length_data = self.port.read(1)
        if len(length_data) != 1:
            print("  Identifier: timeout while reading payload length")
            return None

        length = length_data[0]
        if length < 4 or length > 32:
            print(f"  Identifier: invalid payload length {length}")
            return None

        payload = self.port.read(length)
        if len(payload) != length:
            print(
                f"  Identifier: expected {length} payload bytes, "
                f"received {len(payload)}"
            )
            return None

        if not self._wait_ack():
            print("  Identifier: missing final ACK")
            return None

        identifier_data = payload[:4]
        if not all(0x20 <= value <= 0x7E for value in identifier_data):
            print(
                "  Identifier: first four bytes are not printable ASCII: "
                + identifier_data.hex(" ").upper()
            )
            return None

        identifier = identifier_data.decode("ascii")

        if length > 4:
            extension = payload[4:]
            print(
                f"  Identifier payload: {payload.hex(' ').upper()} "
                f"(extension: {extension.hex(' ').upper()})"
            )

        return identifier

    def read_memory(self, address, length):
        if not 1 <= length <= 256:
            raise ValueError("length must be 1..256")

        if not self._send_command(self.CMD_READ_MEMORY):
            return None

        addr_bytes = address.to_bytes(4, "big")
        checksum = 0
        for b in addr_bytes:
            checksum ^= b

        self.port.write(addr_bytes + bytes([checksum]))

        if not self._wait_ack():
            return None

        n = length - 1
        self.port.write(bytes([n, n ^ 0xFF]))

        if not self._wait_ack():
            return None

        data = self.port.read(length)
        if len(data) != length:
            return None

        return data

    def get_flash_size_kb(self, family):
        if family in (0x0410, 0x0414, 0x0440, 0x0418):
            flash_size_addr = 0x1FFFF7E0
        elif family == 0x0419:
            flash_size_addr = 0x1FFF7A22
        else:
            flash_size_addr = 0x1FFF77DE

        data = self.read_memory(flash_size_addr, 2)
        if data is None or len(data) != 2:
            return None

        return int.from_bytes(data, "little")
        
    def get_uid(self, family):
        if family == 0x0419:
            uid_addr = 0x1FFF7A10
        else:
            uid_addr = 0x1FFFF7E8
    
        data = self.read_memory(uid_addr, 12)
        if data is None or len(data) != 12:
            return None
    
        return data.hex().upper()

    def erase_all(self):
        print("Erasing flash...")

        if not self._send_command(self.CMD_EXTENDED_ERASE):
            if not self._send_command(self.CMD_ERASE):
                print("  Erase command not supported")
                return False
            self.port.write(bytes([0xFF, 0x00]))
        else:
            self.port.write(bytes([0xFF, 0xFF, 0x00]))

        self.port.timeout = 30
        result = self._wait_ack()
        self.port.timeout = self.timeout

        print("  Erase " + ("OK" if result else "FAILED"))
        return result

    def write_memory(self, address, data):
        if not self._send_command(self.CMD_WRITE_MEMORY):
            return False

        addr_bytes = address.to_bytes(4, "big")
        checksum = 0
        for b in addr_bytes:
            checksum ^= b

        self.port.write(addr_bytes + bytes([checksum]))

        if not self._wait_ack():
            return False

        n = len(data) - 1
        checksum = n
        for b in data:
            checksum ^= b

        self.port.write(bytes([n]) + data + bytes([checksum]))

        return self._wait_ack()
        
    def read_u32(self, address):
        data = self.read_memory(address, 4)
        if data is None or len(data) != 4:
            return None
        return int.from_bytes(data, "little")


    def write_u32(self, address, value):
        return self.write_memory(address, value.to_bytes(4, "little"))


    def probe_register_bit(self, address, bit):
        mask = 1 << bit

        original = self.read_u32(address)
        if original is None:
            return None

        if not self.write_u32(address, original | mask):
            return None

        changed = self.read_u32(address)

        # Always restore original register value.
        self.write_u32(address, original)

        if changed is None:
            return None

        return bool(changed & mask)
        
    def flash_file(self, filename, address, label=""):
        data = Path(filename).read_bytes()
        prefix = f"[{label}] " if label else ""

        print(f"{prefix}Flashing {len(data)} bytes to 0x{address:08X}...")
        print(f"{prefix}File: {filename}")

        chunk_size = 256
        offset = 0

        while offset < len(data):
            chunk = data[offset:offset + chunk_size]
            current_addr = address + offset

            if not self.write_memory(current_addr, chunk):
                print(f"\n{prefix}  Write failed at 0x{current_addr:08X}")
                return False

            offset += len(chunk)
            progress = offset * 100 // len(data)
            print(
                f"\r{prefix}  Progress: {progress}% ({offset}/{len(data)} bytes)",
                end="",
                flush=True,
            )

        print(f"\n{prefix}  Flash complete!")
        return True

    def run(self, address):
        print(f"Starting execution at 0x{address:08X}...")

        if not self._send_command(self.CMD_GO):
            return False

        addr_bytes = address.to_bytes(4, "big")
        checksum = 0
        for b in addr_bytes:
            checksum ^= b

        self.port.write(addr_bytes + bytes([checksum]))

        return self._wait_ack()


def print_device_info(flasher, gd32_db):
    chip_id = flasher.get_id()
    if chip_id is None:
        print("Failed to read chip ID")
        return False

    family = int(chip_id, 16)
    size_kb = flasher.get_flash_size_kb(family)
    if size_kb is None:
        print("Failed to read flash size")
        return False

    device = lookup_device(gd32_db, chip_id, size_kb, flasher)

    print(f"Chip ID     : {chip_id}")
    print(f"Identifier  : {device['identifier']}")

    if device:
        print(f"Series      : {device['series']}")
        print(f"Flash size  : {size_kb} KB")
        print(f"Part number : {device['part_number']}")
    else:
        print("Series      : Unknown")
        print(f"Flash size  : {size_kb} KB")
        print("Part number : Unknown")

    return True


def start_monitor(port, baud=115200, reset=True):
    """Start a UART monitor on the specified port with auto-reconnect."""
    print(f"\n=== Starting UART Monitor on {port} at {baud} baud ===")
    print("Press Ctrl+C to exit\n")

    ser = None
    first_connect = True

    try:
        while True:
            if ser is None or not ser.is_open:
                try:
                    ser = serial.Serial(port, baud, timeout=0.1)

                    if first_connect:
                        print("[Connected]")
                    else:
                        print("\n[Reconnected]")

                    ser.dtr = True   # BOOT0 = low, normal boot from flash

                    if reset and first_connect:
                        print("[Resetting board...]\n")
                        ser.rts = True
                        time.sleep(0.1)
                        ser.rts = False
                    else:
                        ser.rts = False

                    first_connect = False

                except serial.SerialException:
                    time.sleep(0.05)
                    continue

            try:
                data = ser.read(1024)
                if data:
                    try:
                        text = data.decode("utf-8", errors="replace")
                        print(text, end="", flush=True)
                    except UnicodeError:
                        print(data.hex(), end=" ", flush=True)
            except (serial.SerialException, OSError):
                print("\n[Disconnected - waiting for reconnect...]", flush=True)
                try:
                    ser.close()
                except serial.SerialException:
                    pass
                ser = None

    except KeyboardInterrupt:
        print("\n\nMonitor stopped.")
    finally:
        if ser and ser.is_open:
            ser.close()


def main():
    parser = argparse.ArgumentParser(description="Flash GD32 board")
    parser.add_argument("--port", "-p", default="/dev/ttyUSB0", help="Serial port")
    parser.add_argument("--baud", "-b", type=int, default=57600, help="Baud rate for flashing")
    parser.add_argument("--monitor", action="store_true", help="Start UART monitor")
    parser.add_argument("--get-version", action="store_true", help="Read bootloader version")
    parser.add_argument("--get-id", action="store_true", help="Read chip ID")
    parser.add_argument("--get-identifier", action="store_true", help="Read device identifier")
    parser.add_argument("--get-uid", action="store_true", help="Read chip UID")
    parser.add_argument("--get-size", action="store_true", help="Read flash size, series and part number")
    parser.add_argument("--db", default="gd32.json", help="GD32 JSON database file")
    parser.add_argument("--mass-erase", action="store_true", help="Erase entire flash")
    parser.add_argument("--flash", metavar="FILE", help="Flash binary file")
    parser.add_argument(
        "--address",
        "-a",
        type=lambda x: int(x, 0),
        default=0x08000000,
        help="Flash start address (default: 0x08000000)",
    )
    parser.add_argument("--label", default="", help="Optional label for flash output")
    parser.add_argument("--go", action="store_true", help="Start execution after flashing")
    parser.add_argument(
        "--go-address",
        type=lambda x: int(x, 0),
        default=0x08000000,
        help="Execution start address for --go (default: 0x08000000)",
    )

    args = parser.parse_args()

    if args.monitor:
        time.sleep(0.5)
        start_monitor(args.port, args.baud)
        return 0

    needs_bootloader = any([
        args.get_version,
        args.get_id,
        args.get_identifier,
        args.get_uid,
        args.get_size,
        args.mass_erase,
        args.flash,
        args.go,
    ])

    if not needs_bootloader:
        parser.print_help()
        return 0

    gd32_db = load_gd32_db(args.db)

    flasher = GD32Flasher(args.port, args.baud)
    flasher.open()

    try:
        if not flasher.enter_bootloader():
            return 1

        chip_id_already_printed = False

        if args.get_version:
            version = flasher.get_version()
            if version is None:
                print("Failed to read bootloader version")
                return 1
            print(f"Bootloader version: 0x{version:02X}")
            
        if args.get_id and not args.get_size:
            chip_id = flasher.get_id()
            if chip_id is None:
                print("Failed to read chip ID")
                return 1
            print(f"Chip ID: {chip_id}")
            chip_id_already_printed = True

        if args.get_identifier and not args.get_size:
            identifier = flasher.get_identifier()
            if identifier is None:
                print("Failed to read device identifier")
                return 1
            print(f"Identifier: {identifier}")

        if args.get_size:
            if not print_device_info(flasher, gd32_db):
                return 1
            chip_id_already_printed = True
            
        if args.get_uid:
            chip_id = flasher.get_id()
            if chip_id is None:
                print("Failed to read chip ID")
                return False
        
            chip_uid = flasher.get_uid(int(chip_id, 16))
            print(f"Chip UID    : {chip_uid}")

        if args.mass_erase:
            if not flasher.erase_all():
                return 1

        if args.flash:
            if not flasher.flash_file(args.flash, args.address, args.label):
                return 1

        if args.go:
            if not flasher.run(args.go_address):
                return 1

        # Keep variable useful for future extension and avoid lint warnings in strict editors.
        _ = chip_id_already_printed

    finally:
        flasher.close()

    return 0


if __name__ == "__main__":
    sys.exit(main())
