#!/usr/bin/env python3

import serial
import time
import sys
import argparse
import subprocess
import os
from pathlib import Path

class GD32Flasher:
    ACK = 0x79
    NACK = 0x1F
    
    CMD_GET = 0x00
    CMD_GET_VERSION = 0x01
    CMD_GET_ID = 0x02
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
            timeout=self.timeout
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
            
    def enter_bootloader(self):
        """Enter ROM bootloader mode using DTR/RTS"""
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
        for i in range(attempts):
            self.port.write(bytes([0x7F]))
            time.sleep(0.1)
            resp = self.port.read(1)
            if resp and resp[0] == self.ACK:
                return True
        return False
    
    def _send_command(self, cmd):
        self.port.write(bytes([cmd, cmd ^ 0xFF]))
        resp = self.port.read(1)
        return resp and resp[0] == self.ACK
    
    def _wait_ack(self):
        resp = self.port.read(1)
        return resp and resp[0] == self.ACK
    
    def get_version(self):
        if not self._send_command(self.CMD_GET_VERSION):
            return None
        version = self.port.read(1)[0]
        self.port.read(2)
        self._wait_ack()
        return version
    
    def get_id(self):
        if not self._send_command(self.CMD_GET_ID):
            return None
        n = self.port.read(1)[0]
        chip_id = self.port.read(n + 1)
        self._wait_ack()
        return chip_id.hex()
    
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
            
        addr_bytes = address.to_bytes(4, 'big')
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
            print(f"\r{prefix}  Progress: {progress}% ({offset}/{len(data)} bytes)", end='', flush=True)
        
        print(f"\n{prefix}  Flash complete!")
        return True
    
    def run(self, address):
        print(f"Starting execution at 0x{address:08X}...")
        if not self._send_command(self.CMD_GO):
            return False
            
        addr_bytes = address.to_bytes(4, 'big')
        checksum = 0
        for b in addr_bytes:
            checksum ^= b
        self.port.write(addr_bytes + bytes([checksum]))
        
        return self._wait_ack()


def start_monitor(port, baud=115200, reset=True):
    """Start a UART monitor on the specified port with auto-reconnect"""
    print(f"\n=== Starting UART Monitor on {port} at {baud} baud ===")
    print("Press Ctrl+C to exit\n")
    
    ser = None
    first_connect = True
    
    try:
        while True:
            # Try to connect/reconnect
            if ser is None or not ser.is_open:
                try:
                    ser = serial.Serial(port, baud, timeout=0.1)
                    
                    if first_connect:
                        print("[Connected]")
                    else:
                        print("\n[Reconnected]")
                    
                    # Set normal boot mode
                    ser.dtr = True   # BOOT0 = low (normal boot from flash)
                    
                    if reset and first_connect:
                        print("[Resetting board...]\n")
                        ser.rts = True   # Assert reset
                        time.sleep(0.1)
                        ser.rts = False  # Release reset - board boots
                    else:
                        ser.rts = False
                    
                    first_connect = False
                    
                except serial.SerialException:
                    # Port not available, wait briefly and retry
                    time.sleep(0.05)
                    continue
            
            # Read data
            try:
                data = ser.read(1024)
                if data:
                    try:
                        text = data.decode('utf-8', errors='replace')
                        print(text, end='', flush=True)
                    except:
                        print(data.hex(), end=' ', flush=True)
            except (serial.SerialException, OSError):
                # Device disconnected
                print("\n[Disconnected - waiting for reconnect...]", flush=True)
                try:
                    ser.close()
                except:
                    pass
                ser = None
                
    except KeyboardInterrupt:
        print("\n\nMonitor stopped.")
    finally:
        if ser and ser.is_open:
            ser.close()

def main():
    parser = argparse.ArgumentParser(description='Flash GD32 board')
    parser.add_argument('--port', '-p', default='/dev/ttyUSB0', help='Serial port')
    parser.add_argument('--baud', '-b', type=int, default=57600, help='Baud rate for flashing')
    parser.add_argument('--monitor', action='store_true', help='Start UART monitor')
    parser.add_argument('--get-version', action='store_true', help='Read bootloader version')
    parser.add_argument('--get-id', action='store_true', help='Read chip ID')
    parser.add_argument('--mass-erase', action='store_true', help='Erase entire flash')
    parser.add_argument('--flash', metavar='FILE', help='Flash binary file')
    parser.add_argument('--address', '-a', type=lambda x: int(x, 0),
                        default=0x08000000,
                        help='Flash start address (default: 0x08000000)')
    parser.add_argument('--label', default='', help='Optional label for flash output')

    args = parser.parse_args()

    if args.monitor:
        time.sleep(0.5)
        start_monitor(args.port, args.baud)
        return 0

    needs_bootloader = any([
        args.get_version,
        args.get_id,
        args.mass_erase,
        args.flash
    ])

    if not needs_bootloader:
        parser.print_help()
        return 0

    flasher = GD32Flasher(args.port, args.baud)
    flasher.open()

    try:
        if not flasher.enter_bootloader():
            return 1

        if args.get_version:
            version = flasher.get_version()
            if version is None:
                print("Failed to read bootloader version")
            else:
                print(f"Bootloader version: 0x{version:02X}")

        if args.get_id:
            chip_id = flasher.get_id()
            if chip_id is None:
                print("Failed to read chip ID")
            else:
                print(f"Chip ID: {chip_id}")

        if args.mass_erase:
            if not flasher.erase_all():
                return 1

        if args.flash:
            if not flasher.flash_file(args.flash, args.address, args.label):
                return 1

    finally:
        flasher.close()

    return 0
    
if __name__ == '__main__':
    sys.exit(main())
