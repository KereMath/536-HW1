#!/usr/bin/env python3
"""
Debug test: İlk request'te share assignment kontrol
"""
import socket
import time
import os
import subprocess

conn_str = "@/tmp/perfect_gym.sock"

# Start server
os.system("pkill -9 hw1 2>/dev/null")
time.sleep(0.5)

server_proc = subprocess.Popen(
    ["./hw1", conn_str, "1000", "5000", "3"],
    stdout=subprocess.DEVNULL,
    stderr=subprocess.DEVNULL
)
time.sleep(1.0)

try:
    # Client 1 - first request 800ms
    sock1 = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock1.connect(conn_str[1:])

    print("Sending REQUEST 800")
    sock1.send(b"REQUEST 800\n")
    time.sleep(0.3)

    # Get response
    data = sock1.recv(4096).decode()
    print(f"Response: {data}")

    # Wait a bit
    time.sleep(0.5)

    # Send REPORT to check share
    print("\nSending REPORT")
    sock1.send(b"REPORT\n")
    time.sleep(0.3)

    report = sock1.recv(4096).decode()
    print(f"REPORT:\n{report}")

    sock1.send(b"QUIT\n")
    sock1.close()

finally:
    os.system("pkill -9 hw1 2>/dev/null")
