#!/usr/bin/env python3
"""Simple preemption test with debug output visible"""

import socket
import time
import select

def send_cmd(sock, cmd):
    sock.sendall((cmd + "\n").encode())

def recv_with_timeout(sock, timeout=2.0):
    """Receive all available data with timeout"""
    sock.setblocking(0)
    ready = select.select([sock], [], [], timeout)
    data = b""
    if ready[0]:
        try:
            while True:
                chunk = sock.recv(4096)
                if not chunk:
                    break
                data += chunk
                # Quick check if more data available
                ready = select.select([sock], [], [], 0.05)
                if not ready[0]:
                    break
        except:
            pass
    sock.setblocking(1)
    return data.decode('utf-8', errors='ignore')

print("="*70)
print("PREEMPTION TEST WITH DEBUG OUTPUT")
print("="*70)
print("\nScenario:")
print("  1. C1 builds high share (~2000ms)")
print("  2. C1 requests 10000ms")
print("  3. C2 requests 5000ms (share=0, should wait)")
print("  4. After q+700ms, C1 should be preempted")
print()

# Connect C1
print("[C1] Connecting...")
c1 = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
c1.connect("\0/tmp/perfect_gym.sock")
time.sleep(0.3)

# C1 builds share
print("[C1] Building share with 2000ms request...")
send_cmd(c1, "REQUEST 2000")
msg = recv_with_timeout(c1, 1.0)
print(f"[C1] <- {msg.strip()}")
time.sleep(2.5)  # Wait for completion

# C1 requests again (high share ~2000)
print(f"\n[C1] Requesting 10000ms (now has high share)...")
send_cmd(c1, "REQUEST 10000")
msg = recv_with_timeout(c1, 1.0)
print(f"[C1] <- {msg.strip()}")

# Connect C2
print("\n[C2] Connecting...")
c2 = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
c2.connect("\0/tmp/perfect_gym.sock")
time.sleep(0.2)

# C2 requests (low share = 0)
print("[C2] Requesting 5000ms (share=0, should be queued)...")
send_cmd(c2, "REQUEST 5000")
msg = recv_with_timeout(c2, 0.5)
if msg:
    print(f"[C2] <- {msg.strip()}")

# Now wait q + 700ms = 1700ms
print(f"\n⏱️  Waiting 1.7 seconds (q=1000ms + 700ms buffer)...")
time.sleep(1.7)

# Check if C1 got "removed" message
print("\n[C1] Checking for 'removed' message...")
msg = recv_with_timeout(c1, 0.5)
if "removed" in msg:
    print(f"✅ [C1] <- {msg.strip()}")
    print("✅ PREEMPTION WORKED!")
else:
    print(f"❌ [C1] No 'removed' message (got: {msg.strip() if msg else 'nothing'})")
    print("❌ PREEMPTION FAILED!")

# Check if C2 got assigned
print("\n[C2] Checking for 'assigned' message...")
msg = recv_with_timeout(c2, 1.0)
if "assigned" in msg:
    print(f"✅ [C2] <- {msg.strip()}")
else:
    print(f"❌ [C2] No assignment yet (got: {msg.strip() if msg else 'nothing'})")

# Cleanup
print("\n[C1] Disconnecting...")
send_cmd(c1, "QUIT")
c1.close()

print("[C2] Disconnecting...")
send_cmd(c2, "QUIT")
c2.close()

print("\n" + "="*70)
print("TEST COMPLETE")
print("="*70)
