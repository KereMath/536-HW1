#!/usr/bin/env python3
"""
Test 56 detailed debugging - Share assignment investigation
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

def send_recv(sock, msg):
    """Send message and receive response"""
    sock.send(msg.encode() + b'\n')
    time.sleep(0.2)
    return sock.recv(4096).decode()

def get_report(sock):
    """Get REPORT"""
    sock.send(b'REPORT\n')
    time.sleep(0.2)
    return sock.recv(4096).decode()

try:
    print("="*70)
    print("TEST 56 SCENARIO - Share Assignment Check")
    print("="*70)

    # Create 6 clients
    clients = []
    for i in range(6):
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.connect(conn_str[1:])
        clients.append(sock)

    # Tool holders
    print("\n[T+0.0s] 3 tool holders arrive (W1, W2, W3)")
    resp1 = send_recv(clients[0], "REQUEST 800")
    resp2 = send_recv(clients[1], "REQUEST 1200")
    resp3 = send_recv(clients[2], "REQUEST 1800")

    print(f"  W1: {resp1.strip()}")
    print(f"  W2: {resp2.strip()}")
    print(f"  W3: {resp3.strip()}")

    time.sleep(0.3)

    # Get REPORT to check shares
    print("\n[T+0.3s] REPORT - Check initial shares")
    report = get_report(clients[0])
    print(report)

    # Waiters arrive
    print("\n[T+0.3s] 3 waiters arrive (W4, W5, W6)")
    resp4 = send_recv(clients[3], "REQUEST 2400")
    resp5 = send_recv(clients[4], "REQUEST 3200")
    resp6 = send_recv(clients[5], "REQUEST 4000")

    print(f"  W4: {resp4.strip()}")
    print(f"  W5: {resp5.strip()}")
    print(f"  W6: {resp6.strip()}")

    time.sleep(0.3)

    # Get REPORT after waiters
    print("\n[T+0.6s] REPORT - After waiters arrive")
    report = get_report(clients[3])
    print(report)

    # Wait for q limit (1000ms)
    print("\n[Waiting 1.5s for q limit to pass...]")
    time.sleep(1.5)

    # Get REPORT after q limit
    print("\n[T+2.1s] REPORT - After q=1000ms passed")
    report = get_report(clients[0])
    print(report)

    # Check tool assignment
    if "W4" in report or "W5" in report or "W6" in report:
        # Check which waiter got tool
        lines = report.split('\n')
        for line in lines:
            if 'Customer' in line and 'leaves' not in line:
                parts = line.split()
                if len(parts) >= 3:
                    cid = parts[0]
                    share = parts[1] if len(parts) > 1 else "?"
                    print(f"\n  Customer {cid} share={share}")

    # Cleanup
    for sock in clients:
        try:
            sock.send(b'QUIT\n')
            sock.close()
        except:
            pass

    print("\n" + "="*70)
    print("KEY FINDINGS:")
    print("="*70)
    print("• Check if W1/W2/W3 have correct shares IMMEDIATELY (not share=0)")
    print("• Check if waiters W4/W5/W6 are assigned shares based on request")
    print("• After q=1000ms, lowest share waiter should get tool")
    print("="*70)

finally:
    os.system("pkill -9 hw1 2>/dev/null")
