#!/usr/bin/env python3
"""Upload firmware using UDP control and the macOS/BSD system TFTP client."""
from __future__ import annotations

import os
import re
import shutil
import subprocess
import sys
import time

sys.dont_write_bytecode = True
import udp_send

PORT = 10501
BUFLEN = 512
TIMEOUT_SEC = 1.0
STATE_TIMEOUT_SEC = 30.0
REBOOT_TIMEOUT_SEC = 60.0
TRANSFER_TIMEOUT_SEC = 120.0
POLL_INTERVAL_SEC = 0.5
REBOOT_SETTLE_SEC = 1.0
BOOTLOADER = "Bootloader TFTP"


def _udp_cmd(ip: str, cmd: str) -> str:
    try:
        _, reply = udp_send.send_and_maybe_recv(
            ip, cmd.encode("utf-8"), port=PORT, local_port=PORT,
            timeout_sec=TIMEOUT_SEC, buf_len=BUFLEN,
        )
    except (TimeoutError, ConnectionResetError, ConnectionRefusedError):
        return ""
    return reply.decode("utf-8", errors="replace").strip() if reply else ""


def _poll(ip, query, predicate, description, timeout=STATE_TIMEOUT_SEC, tick=None):
    deadline = time.monotonic() + timeout
    last = ""
    while time.monotonic() < deadline:
        if tick is not None:
            _udp_cmd(ip, tick)
        reply = _udp_cmd(ip, query)
        if reply:
            last = reply
            if predicate(reply):
                return reply
        time.sleep(min(POLL_INTERVAL_SEC, max(0, deadline - time.monotonic())))
    raise RuntimeError(f"Timeout waiting for {description}; last reply: {last or '(none)'}")


def _valid_list(ip, reply):
    fields = reply.strip("[]").split(",")
    return len(fields) >= 5 and fields[0] == ip and bool(fields[1])


def _wait_list(ip, bootloader=None, timeout=STATE_TIMEOUT_SEC):
    def matches(reply):
        if not _valid_list(ip, reply):
            return False
        mode = reply.strip("[]").split(",")[1]
        return bootloader is None or (mode == BOOTLOADER) == bootloader

    description = {None: "device list", True: BOOTLOADER, False: "application firmware"}[bootloader]
    print(f"Waiting for {description} (?list#)...", flush=True)
    reply = _poll(ip, "?list#", matches, description, timeout)
    print(f"[{reply}]", flush=True)
    return reply


def _ensure_tftp(ip, enabled):
    expected = "tftp:On" if enabled else "tftp:Off"
    reply = _poll(ip, "?tftp#", lambda r: r == expected, expected,
                  tick="!tftp#1" if enabled else "!tftp#0")
    print(f"[{reply}]", flush=True)


def _reboot_and_wait_list(ip, bootloader):
    print("Rebooting...", flush=True)
    _udp_cmd(ip, "?reboot##")
    time.sleep(REBOOT_SETTLE_SEC)
    return _wait_list(ip, bootloader, REBOOT_TIMEOUT_SEC)


def _run_tftp_put(ip, filename, client):
    print("tftp...", flush=True)
    expected = os.path.getsize(filename)
    # Use cwd so directory names containing spaces do not enter the TFTP parser.
    directory, basename = os.path.split(os.path.abspath(filename))
    result = subprocess.run(
        [client, ip], input=f"binary\nput {basename}\nquit\n",
        text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        cwd=directory, timeout=TRANSFER_TIMEOUT_SEC,
        env={**os.environ, "LC_ALL": "C"},
    )
    output = result.stdout or ""
    if output:
        print(output, end="" if output.endswith("\n") else "\n", flush=True)
    sent = re.search(r"\bSent\s+(\d+)\s+bytes\b", output, re.IGNORECASE)
    if (result.returncode != 0 or not sent or int(sent.group(1)) != expected
            or re.search(r"timeout|timed out|\berror\b|\bfailed\b", output, re.IGNORECASE)):
        raise RuntimeError("TFTP transfer not confirmed; leaving device in bootloader mode.")


def main(argv):
    if len(argv) != 3:
        print(f"Usage: {argv[0]} ip_address file", file=sys.stderr)
        return 2
    ip, filepath = argv[1:]
    try:
        if not os.path.isfile(filepath) or os.path.getsize(filepath) == 0:
            raise RuntimeError(f"Missing or empty firmware file: {filepath}")
        if any(c.isspace() for c in os.path.basename(filepath)):
            raise RuntimeError("Firmware basename must not contain whitespace.")
        client = shutil.which("tftp")
        if not client:
            raise RuntimeError("System 'tftp' client not found in PATH.")
        print(filepath, flush=True)
        _ensure_tftp(ip, True)
        reply = _wait_list(ip)
        if reply.strip("[]").split(",")[1] != BOOTLOADER:
            _reboot_and_wait_list(ip, True)
            _ensure_tftp(ip, True)
        # Reconfirm bootloader mode immediately before uploading.
        _wait_list(ip, True)
        _run_tftp_put(ip, filepath, client)
        _ensure_tftp(ip, False)
        _reboot_and_wait_list(ip, False)
        version = _poll(ip, "?version#", bool, "firmware version")
        print(version)
        return 0
    except (RuntimeError, OSError, subprocess.TimeoutExpired) as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        print("\nInterrupted.", file=sys.stderr)
        return 130


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
