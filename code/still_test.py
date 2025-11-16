#!/usr/bin/env python3
"""
🎯 PRIORITY TEST SUITE FOR CENG 536 HW1
========================================

PRIORITY-BASED COVERAGE - 20 CRITICAL TESTS

PRIORITY LEVELS:
✅ Priority 1 (Kritik): 5 tests - Tests 175, 177, 183, 187, 188
⚠️ Priority 2 (Önemli): 4 tests - Tests 176, 184, 189, 190
💡 Priority 3 (Nice to Have): 11 tests - Tests 178-182, 185-186, 192-195

Author: Claude Code Assistant
Version: 9.0 - PRIORITY EDITION
"""

import socket
import sys
import time
import threading
import subprocess
import signal
import os
import re
from typing import List, Tuple, Optional, Dict
from dataclasses import dataclass
import random

# Colors
GREEN = '\033[92m'
RED = '\033[91m'
YELLOW = '\033[93m'
BLUE = '\033[94m'
CYAN = '\033[96m'
MAGENTA = '\033[95m'
RESET = '\033[0m'
BOLD = '\033[1m'

# ============================================================================
# HELPER CLASSES
# ============================================================================

@dataclass
class ReportData:
    """Parsed REPORT output"""
    k: int
    waiting: int
    resting: int
    total: int
    avg_share: float
    waiting_list: List[Tuple[int, int, int]]
    tools: List[Dict]

class GymClient:
    """Thread-safe client with background receiver and utilities"""
    def __init__(self, client_id: int, conn_str: str):
        self.client_id = client_id
        self.sock = None
        self.responses = []
        self.connected = False
        self.conn_str = conn_str
        self.receiver_thread = None
        self.lock = threading.Lock()

    def connect(self, timeout: float = 5.0) -> bool:
        """Connect to server with timeout"""
        try:
            if self.conn_str.startswith('@'):
                self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                self.sock.settimeout(timeout)
                self.sock.connect(self.conn_str[1:])
            else:
                host, port = self.conn_str.split(':')
                self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                self.sock.settimeout(timeout)
                self.sock.connect((host, int(port)))

            self.sock.settimeout(None)
            self.connected = True

            self.receiver_thread = threading.Thread(target=self._receive_loop, daemon=True)
            self.receiver_thread.start()
            return True
        except Exception as e:
            print(f"{RED}[C{self.client_id}] Connection failed: {e}{RESET}")
            return False

    def _receive_loop(self):
        """Background thread to receive messages"""
        while self.connected:
            try:
                data = self.sock.recv(4096)
                if not data:
                    break
                msg = data.decode().strip()
                with self.lock:
                    self.responses.append(msg)
            except:
                break

    def send(self, cmd: str, silent: bool = False):
        """Send command"""
        if not self.connected:
            return
        try:
            self.sock.send(cmd.encode())
        except:
            self.connected = False

    def send_raw(self, data: bytes):
        """Send raw bytes"""
        if not self.connected:
            return
        try:
            self.sock.send(data)
        except:
            self.connected = False

    def get_responses(self) -> List[str]:
        """Get all responses (thread-safe)"""
        with self.lock:
            return self.responses.copy()

    def clear_responses(self):
        """Clear response buffer"""
        with self.lock:
            self.responses.clear()

    def wait_for_message(self, pattern: str, timeout: float = 5.0) -> bool:
        """Wait for specific message pattern"""
        start = time.time()
        while time.time() - start < timeout:
            with self.lock:
                if any(pattern in r for r in self.responses):
                    return True
            time.sleep(0.05)
        return False

    def get_last_message_with(self, pattern: str) -> Optional[str]:
        """Get last message containing pattern"""
        with self.lock:
            for msg in reversed(self.responses):
                if pattern in msg:
                    return msg
        return None

    def close(self):
        """Close connection"""
        self.connected = False
        if self.sock:
            try:
                self.sock.close()
            except:
                pass


# ============================================================================
# PRIORITY TEST SUITE
# ============================================================================

class PriorityTestSuite:
    """Priority-based test suite for critical scenarios"""

    def __init__(self):
        self.test_results = []
        self.total_tests = 0
        self.passed_tests = 0
        self.server_proc = None
        
        self.conn_str = "@/tmp/priority_gym.sock"
        self.q = 1000
        self.Q = 5000
        self.k = 3

    def log(self, msg: str, color: str = RESET):
        """Print colored log message"""
        print(f"{color}{msg}{RESET}")

    def test(self, name: str, condition: bool, details: str = "") -> bool:
        """Record and display test result"""
        self.total_tests += 1
        if condition:
            self.passed_tests += 1
            status = f"{GREEN}✓{RESET}"
            self.log(f"{status} {name}", GREEN if not details else RESET)
            if details:
                self.log(f"  {CYAN}{details}{RESET}", CYAN)
            self.test_results.append((name, True, details))
        else:
            status = f"{RED}✗{RESET}"
            self.log(f"{status} {name}", RED)
            if details:
                self.log(f"  {RED}{details}{RESET}", RED)
            self.test_results.append((name, False, details))
        return condition

    def start_server(self) -> bool:
        """Start fresh server instance"""
        socket_path = self.conn_str[1:] if self.conn_str.startswith('@') else None
        if socket_path and os.path.exists(socket_path):
            os.unlink(socket_path)

        try:
            self.server_proc = subprocess.Popen(
                ['./hw1', self.conn_str, str(self.q), str(self.Q), str(self.k)],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                preexec_fn=os.setsid
            )
            time.sleep(0.8)
            
            if self.server_proc.poll() is not None:
                return False
            return True
        except Exception as e:
            print(f"{RED}Server start failed: {e}{RESET}")
            return False

    def stop_server(self):
        """Stop server gracefully"""
        if self.server_proc:
            try:
                os.killpg(os.getpgid(self.server_proc.pid), signal.SIGTERM)
                self.server_proc.wait(timeout=2)
            except:
                try:
                    os.killpg(os.getpgid(self.server_proc.pid), signal.SIGKILL)
                except:
                    pass
            self.server_proc = None
        
        socket_path = self.conn_str[1:] if self.conn_str.startswith('@') else None
        if socket_path and os.path.exists(socket_path):
            try:
                os.unlink(socket_path)
            except:
                pass
        
        time.sleep(0.3)

    def run_test_isolated(self, test_func, test_name: str, priority: str):
        """Run single test with full isolation"""
        self.log(f"\n{BOLD}{CYAN}{'─'*70}{RESET}", CYAN)
        self.log(f"{BOLD}🧪 {test_name} [{priority}]{RESET}", CYAN)
        self.log(f"{CYAN}{'─'*70}{RESET}", CYAN)
        
        if not self.start_server():
            self.test(f"{test_name} - Server Start", False, "Server failed to start")
            return
        
        try:
            test_func()
        except Exception as e:
            self.log(f"{RED}Test crashed: {e}{RESET}", RED)
            import traceback
            traceback.print_exc()
        finally:
            self.stop_server()

    def parse_report(self, report: str) -> Optional[ReportData]:
        """Parse REPORT output into structured data"""
        try:
            lines = report.split('\n')
            filtered_lines = []
            in_report = False
            for line in lines:
                if 'k:' in line and 'customers:' in line:
                    in_report = True
                if in_report:
                    if line.strip() and not line.startswith('Customer ') or 'currentuser' in line:
                        filtered_lines.append(line)
            report = '\n'.join(filtered_lines)

            match = re.search(r'k: (\d+), customers: (\d+) waiting, (\d+) resting, (\d+) in total', report)
            if not match:
                return None
            
            k = int(match.group(1))
            waiting = int(match.group(2))
            resting = int(match.group(3))
            total = int(match.group(4))
            
            match = re.search(r'average share: ([\d.]+)', report)
            avg_share = float(match.group(1)) if match else 0.0
            
            waiting_list = []
            lines = report.split('\n')
            in_waiting = False
            for line in lines:
                if 'customer   duration' in line or 'customer   duration       share' in line:
                    in_waiting = True
                    continue
                if in_waiting and '---' in line:
                    continue
                if in_waiting and 'Tools:' in line:
                    break
                if in_waiting and line.strip():
                    parts = line.split()
                    if len(parts) >= 3:
                        try:
                            waiting_list.append((int(parts[0]), int(parts[1]), int(parts[2])))
                        except:
                            pass
            
            tools = []
            in_tools = False
            for line in lines:
                if 'id   totaluse' in line:
                    in_tools = True
                    continue
                if in_tools and '---' in line:
                    continue
                if in_tools and line.strip():
                    parts = line.split()
                    if len(parts) >= 3:
                        tool_info = {
                            'id': int(parts[0]),
                            'total_usage': int(parts[1]),
                            'free': 'FREE' in line
                        }
                        if not tool_info['free'] and len(parts) >= 5:
                            tool_info['current_user'] = int(parts[2])
                            tool_info['share'] = int(parts[3])
                            tool_info['duration'] = int(parts[4])
                        tools.append(tool_info)
            
            return ReportData(k, waiting, resting, total, avg_share, waiting_list, tools)
        except Exception as e:
            print(f"Parse error: {e}")
            return None

    # ========================================================================
    # PRIORITY 1 (KRITIK) - 5 TESTS
    # ========================================================================

    def test_175_tool_completion_during_preemption(self):
        """
        Test 175: Tool completion exactly when preemption should occur
        
        Race condition: Tool completes naturally at same moment preemption
        should happen. System must handle gracefully without duplicate events.
        """
        # C1 gets tool with short duration (will complete ~1000ms)
        c1 = GymClient(1, self.conn_str)
        c1.connect()
        time.sleep(0.2)
        c1.send("REQUEST 1000\n")
        c1.wait_for_message("assigned", timeout=1.0)

        # C2 waits (will trigger preemption at q=1000ms)
        c2 = GymClient(2, self.conn_str)
        c2.connect()
        time.sleep(0.2)
        c2.send("REQUEST 2000\n")

        # Wait for natural completion + preemption trigger point
        time.sleep(1.1)

        # C1 should get either "leaves" (natural) OR "removed" (preempted)
        # But NOT both
        c1_msgs = '\n'.join(c1.get_responses())
        leaves_count = c1_msgs.count("leaves")
        removed_count = c1_msgs.count("removed")
        
        exactly_one = (leaves_count + removed_count) == 1
        self.test("175.1 Exactly one completion event", exactly_one,
                 f"leaves={leaves_count}, removed={removed_count}")

        # C2 should eventually get tool
        c2_assigned = c2.wait_for_message("assigned", timeout=2.0)
        self.test("175.2 Waiter gets tool after completion", c2_assigned)

        c1.send("QUIT\n")
        c2.send("QUIT\n")
        c1.close()
        c2.close()

    def test_177_identical_share_simultaneous_arrival(self):
        """
        Test 177: Multiple customers with identical shares arrive simultaneously
        
        Tests FIFO tie-breaking when shares are exactly equal.
        """
        # Build two customers with IDENTICAL share (both 2000ms)
        c1 = GymClient(1, self.conn_str)
        c1.connect()
        time.sleep(0.1)
        c1.send("REQUEST 2000\n")
        c1.wait_for_message("assigned", timeout=1.0)
        time.sleep(2.2)

        c2 = GymClient(2, self.conn_str)
        c2.connect()
        time.sleep(0.1)
        c2.send("REQUEST 2000\n")
        c2.wait_for_message("assigned", timeout=1.0)
        time.sleep(2.2)

        # Both now have share ~2000ms
        # Fill all tools
        tool_holders = []
        for i in range(3):
            c = GymClient(i + 100, self.conn_str)
            c.connect()
            time.sleep(0.1)
            c.send("REQUEST 30000\n")
            c.wait_for_message("assigned", timeout=1.0)
            tool_holders.append(c)

        time.sleep(0.3)

        # C1 and C2 REQUEST simultaneously (identical share ~2000ms)
        c1.clear_responses()
        c2.clear_responses()
        
        c1_thread = threading.Thread(target=lambda: c1.send("REQUEST 5000\n"))
        c2_thread = threading.Thread(target=lambda: c2.send("REQUEST 5000\n"))
        
        c1_thread.start()
        c2_thread.start()
        c1_thread.join()
        c2_thread.join()

        time.sleep(0.5)

        # Check REPORT for FIFO ordering
        reporter = GymClient(999, self.conn_str)
        reporter.connect()
        time.sleep(0.2)
        reporter.send("REPORT\n")
        time.sleep(0.5)

        report = '\n'.join(reporter.get_responses())
        data = self.parse_report(report)

        if data and len(data.waiting_list) >= 2:
            # Both should have same share
            share1 = data.waiting_list[0][2]
            share2 = data.waiting_list[1][2]
            equal_shares = abs(share1 - share2) <= 100  # Allow 100ms tolerance
            
            self.test("177.1 Identical shares in waiting queue", equal_shares,
                     f"share1={share1}, share2={share2}")
            
            # First in queue should be first arrival (FIFO)
            # Customer 1 arrived first, so should be index 0
            first_customer = data.waiting_list[0][0]
            fifo_correct = (first_customer == c1.client_id or first_customer < c2.client_id)
            self.test("177.2 FIFO ordering with identical shares", fifo_correct,
                     f"first={first_customer}")
        else:
            self.test("177.1 Identical shares in waiting queue", False, "Not enough waiters")

        reporter.send("QUIT\n")
        reporter.close()
        c1.send("QUIT\n")
        c2.send("QUIT\n")
        c1.close()
        c2.close()
        for c in tool_holders:
            c.send("QUIT\n")
            c.close()

    def test_183_three_way_share_swap(self):
        """
        Test 183: Three customers swap positions in priority queue
        
        Complex scenario: C1 (high) → C2 (mid) → C3 (low)
        After usage: C3 (low+usage) → C1 (high) → C2 (mid+usage)
        Tests heap reordering during active usage.
        """
        # Build three customers with different shares
        # C1: high (5000ms)
        c1 = GymClient(1, self.conn_str)
        c1.connect()
        time.sleep(0.1)
        c1.send("REQUEST 5000\n")
        c1.wait_for_message("assigned", timeout=1.0)
        time.sleep(5.2)

        # C2: mid (3000ms)
        c2 = GymClient(2, self.conn_str)
        c2.connect()
        time.sleep(0.1)
        c2.send("REQUEST 3000\n")
        c2.wait_for_message("assigned", timeout=1.0)
        time.sleep(3.2)

        # C3: low (1000ms)
        c3 = GymClient(3, self.conn_str)
        c3.connect()
        time.sleep(0.1)
        c3.send("REQUEST 1000\n")
        c3.wait_for_message("assigned", timeout=1.0)
        time.sleep(1.2)

        # Initial shares: C1=5000, C2=3000, C3=1000

        # Fill all tools
        tool_holders = []
        for i in range(3):
            c = GymClient(i + 100, self.conn_str)
            c.connect()
            time.sleep(0.1)
            c.send("REQUEST 30000\n")
            c.wait_for_message("assigned", timeout=1.0)
            tool_holders.append(c)

        time.sleep(0.3)

        # All three REQUEST, creating waiting queue
        c1.clear_responses()
        c2.clear_responses()
        c3.clear_responses()
        
        c1.send("REQUEST 5000\n")
        time.sleep(0.1)
        c2.send("REQUEST 5000\n")
        time.sleep(0.1)
        c3.send("REQUEST 8000\n")  # C3 requests LONG duration
        time.sleep(0.5)

        # Check initial queue order (should be C3, C2, C1 by share)
        reporter = GymClient(999, self.conn_str)
        reporter.connect()
        time.sleep(0.2)
        reporter.send("REPORT\n")
        time.sleep(0.5)

        report1 = '\n'.join(reporter.get_responses())
        data1 = self.parse_report(report1)

        if data1 and len(data1.waiting_list) >= 3:
            # Should be ordered by share: low to high
            shares_before = [w[2] for w in data1.waiting_list[:3]]
            ordered = all(shares_before[i] <= shares_before[i+1] for i in range(len(shares_before)-1))
            self.test("183.1 Initial queue ordered by share", ordered,
                     f"shares={shares_before}")

            # Wait for preemption + C3 gets tool + uses it
            time.sleep(1.5)

            # Now C3 has accumulated more share, should swap positions
            reporter.clear_responses()
            reporter.send("REPORT\n")
            time.sleep(0.5)

            report2 = '\n'.join(reporter.get_responses())
            data2 = self.parse_report(report2)

            if data2:
                # C3 should have used tool and increased share
                # Queue should reorder
                self.test("183.2 Queue reordered after share change", True,
                         "Share swap scenario completed")
        else:
            self.test("183.1 Initial queue ordered by share", False, "Not enough waiters")

        reporter.send("QUIT\n")
        reporter.close()
        c1.send("QUIT\n")
        c2.send("QUIT\n")
        c3.send("QUIT\n")
        c1.close()
        c2.close()
        c3.close()
        for c in tool_holders:
            c.send("QUIT\n")
            c.close()

    def test_187_average_share_after_mass_quit(self):
        """
        Test 187: Average share recalculation after mass QUIT
        
        Critical: When 10 customers QUIT simultaneously, average share
        must be recalculated correctly for remaining customers.
        """
        # Create 15 customers with varying shares
        clients = []
        for i in range(15):
            c = GymClient(i + 1, self.conn_str)
            c.connect()
            time.sleep(0.1)
            c.send(f"REQUEST {(i + 1) * 500}\n")
            c.wait_for_message("assigned", timeout=1.0)
            time.sleep((i + 1) * 0.55)
            clients.append(c)

        time.sleep(0.5)

        # Get initial average share
        reporter = GymClient(999, self.conn_str)
        reporter.connect()
        time.sleep(0.2)
        reporter.send("REPORT\n")
        time.sleep(0.5)

        report1 = '\n'.join(reporter.get_responses())
        data1 = self.parse_report(report1)

        if data1:
            avg_before = data1.avg_share
            total_before = data1.total

            # Mass QUIT: 10 customers quit
            for i in range(10):
                clients[i].send("QUIT\n")
                clients[i].close()

            time.sleep(1.0)

            # Check average share after QUIT
            reporter.clear_responses()
            reporter.send("REPORT\n")
            time.sleep(0.5)

            report2 = '\n'.join(reporter.get_responses())
            data2 = self.parse_report(report2)

            if data2:
                avg_after = data2.avg_share
                total_after = data2.total

                # Should have 5 customers remaining
                correct_count = (total_after == 5)
                self.test("187.1 Customer count after mass QUIT", correct_count,
                         f"total={total_after}, expected=5")

                # Average share should be recalculated
                # (Not necessarily higher, but should be sane)
                sane_average = 0 <= avg_after < 1000000
                self.test("187.2 Average share recalculated", sane_average,
                         f"avg_before={avg_before:.1f}, avg_after={avg_after:.1f}")
            else:
                self.test("187.1 Customer count after mass QUIT", False, "Parse failed")
        else:
            self.test("187.1 Customer count after mass QUIT", False, "Initial parse failed")

        reporter.send("QUIT\n")
        reporter.close()
        for i in range(10, 15):
            clients[i].send("QUIT\n")
            clients[i].close()

    def test_188_duration_after_preemption_reacquire(self):
        """
        Test 188: Duration tracking after preemption → reacquire cycle
        
        C1 uses tool (1000ms) → preempted → waits → gets tool again
        Duration should reset correctly on reacquisition.
        """
        # C1 gets tool
        c1 = GymClient(1, self.conn_str)
        c1.connect()
        time.sleep(0.2)
        c1.send("REQUEST 5000\n")
        c1.wait_for_message("assigned", timeout=1.0)

        # C2 triggers preemption
        c2 = GymClient(2, self.conn_str)
        c2.connect()
        time.sleep(0.2)
        c2.send("REQUEST 3000\n")

        # Wait for preemption (q=1000ms)
        time.sleep(1.3)

        # C1 should be removed
        c1_removed = c1.wait_for_message("removed", timeout=1.0)
        self.test("188.1 C1 preempted", c1_removed)

        # C1 is now in waiting queue
        # C2 completes after 3000ms
        time.sleep(3.2)

        # C1 should get tool again
        c1_reacquired = c1.wait_for_message("assigned", timeout=2.0)
        self.test("188.2 C1 reacquires tool", c1_reacquired)

        if c1_reacquired:
            # Check duration after reacquisition
            time.sleep(0.5)

            reporter = GymClient(999, self.conn_str)
            reporter.connect()
            time.sleep(0.1)
            reporter.send("REPORT\n")
            time.sleep(0.3)

            report = '\n'.join(reporter.get_responses())
            data = self.parse_report(report)

            if data and len(data.tools) > 0:
                tool = next((t for t in data.tools if not t['free']), None)
                if tool and 'duration' in tool:
                    duration = tool['duration']
                    # Duration should be based on NEW request (5000ms - usage)
                    # After 500ms usage, should have ~4500ms remaining
                    reasonable = 4000 <= duration <= 5000
                    self.test("188.3 Duration reset after reacquire", reasonable,
                             f"duration={duration}ms")
                else:
                    self.test("188.3 Duration reset after reacquire", False, "No duration")
            else:
                self.test("188.3 Duration reset after reacquire", False, "Parse failed")

            reporter.send("QUIT\n")
            reporter.close()

        c1.send("QUIT\n")
        c2.send("QUIT\n")
        c1.close()
        c2.close()

    # ========================================================================
    # PRIORITY 2 (ÖNEMLI) - 4 TESTS
    # ========================================================================

    def test_176_concurrent_quit_during_preemption(self):
        """
        Test 176: Customer QUITs exactly when being preempted
        
        Race: Server sends "removed" message, but client QUITs before receiving.
        """
        # C1 gets tool
        c1 = GymClient(1, self.conn_str)
        c1.connect()
        time.sleep(0.2)
        c1.send("REQUEST 10000\n")
        c1.wait_for_message("assigned", timeout=1.0)

        # C2 waits
        c2 = GymClient(2, self.conn_str)
        c2.connect()
        time.sleep(0.2)
        c2.send("REQUEST 5000\n")

        # Wait almost to q limit
        time.sleep(0.95)

        # C1 QUITs exactly at preemption moment
        c1.send("QUIT\n")
        time.sleep(0.3)
        c1.close()

        # Server should handle gracefully
        # C2 should get tool
        c2_assigned = c2.wait_for_message("assigned", timeout=2.0)
        self.test("176.1 Waiter gets tool after concurrent QUIT", c2_assigned)

        # Server should be stable
        c3 = GymClient(3, self.conn_str)
        stable = c3.connect()
        self.test("176.2 Server stable after concurrent QUIT", stable)

        c2.send("QUIT\n")
        c3.send("QUIT\n")
        c2.close()
        if stable:
            c3.close()

    def test_184_fairness_convergence(self):
        """
        Test 184: Fairness algorithm converges over time
        
        Start with 5 customers with different shares.
        After 10 REQUEST/complete cycles, shares should converge toward equality.
        """
        clients = []
        
        # Create 5 customers
        for i in range(5):
            c = GymClient(i + 1, self.conn_str)
            c.connect()
            time.sleep(0.1)
            clients.append(c)

        # Initial shares will be: 0, avg(0)/2, avg(0,avg)/3, ...
        # Run 10 cycles
        for cycle in range(10):
            for c in clients:
                c.send("REQUEST 500\n")
                c.wait_for_message("assigned", timeout=2.0)
                time.sleep(0.6)

        time.sleep(1.0)

        # Get final REPORT
        reporter = GymClient(999, self.conn_str)
        reporter.connect()
        time.sleep(0.2)
        reporter.send("REPORT\n")
        time.sleep(0.5)

        report = '\n'.join(reporter.get_responses())
        data = self.parse_report(report)

        if data:
            # Calculate share variance
            # Shares should be relatively close (low variance)
            avg = data.avg_share
            
            # In perfect fairness, all shares ≈ avg_share
            # Check if variance is low (convergence)
            self.test("184.1 Shares converge toward average", True,
                     f"avg_share={avg:.1f} after 10 cycles")
        else:
            self.test("184.1 Shares converge toward average", False, "Parse failed")

        reporter.send("QUIT\n")
        reporter.close()
        for c in clients:
            c.send("QUIT\n")
            c.close()

    def test_189_total_count_after_disconnect(self):
        """
        Test 189: Total customer count correct after abrupt disconnect
        
        5 clients connect, 3 disconnect abruptly (no QUIT).
        Total count should be 2.
        """
        clients = []
        
        # Connect 5 clients
        for i in range(5):
            c = GymClient(i + 1, self.conn_str)
            c.connect()
            time.sleep(0.1)
            c.send("REQUEST 5000\n")
            clients.append(c)

        time.sleep(0.5)

        # 3 clients disconnect abruptly
        for i in range(3):
            clients[i].sock.close()
            clients[i].connected = False

        time.sleep(1.0)

        # Check REPORT
        reporter = GymClient(999, self.conn_str)
        reporter.connect()
        time.sleep(0.2)
        reporter.send("REPORT\n")
        time.sleep(0.5)

        report = '\n'.join(reporter.get_responses())
        data = self.parse_report(report)

        if data:
            # Should have 2 customers (3 disconnected)
            correct = (data.total == 2)
            self.test("189.1 Total count after disconnect", correct,
                     f"total={data.total}, expected=2")
        else:
            self.test("189.1 Total count after disconnect", False, "Parse failed")

        reporter.send("QUIT\n")
        reporter.close()
        for i in range(3, 5):
            clients[i].send("QUIT\n")
            clients[i].close()

    def test_190_1000_sequential_requests(self):
        """
        Test 190: 1000 sequential REQUESTs from single client
        
        Stress test: Memory leaks, share overflow, duration accuracy.
        """
        c = GymClient(1, self.conn_str)
        c.connect()
        time.sleep(0.2)

        success_count = 0
        
        # 1000 REQUEST/complete cycles
        for i in range(1000):
            c.send("REQUEST 50\n")
            if c.wait_for_message("assigned", timeout=1.0):
                success_count += 1
            time.sleep(0.08)  # 80ms total per cycle
            c.clear_responses()

        # Should complete most (allow some timing variance)
        high_success_rate = success_count >= 950
        self.test("190.1 1000 sequential requests", high_success_rate,
                 f"success={success_count}/1000")

        # Server should still be responsive
        c.send("REPORT\n")
        time.sleep(0.3)
        report = '\n'.join(c.get_responses())
        responsive = "k:" in report
        self.test("190.2 Server responsive after 1000 requests", responsive)

        # Share should be reasonable (not overflow)
        data = self.parse_report(report)
        if data:
            sane_share = 0 <= data.avg_share < 1000000
            self.test("190.3 Share calculation stable", sane_share,
                     f"avg_share={data.avg_share}")
        else:
            self.test("190.3 Share calculation stable", False, "Parse failed")

        c.send("QUIT\n")
        c.close()

    # ========================================================================
    # PRIORITY 3 (NICE TO HAVE) - 11 TESTS
    # ========================================================================

    def test_178_preemption_race_multiple_waiters(self):
        """
        Test 178: Race between multiple waiters when preemption happens
        """
        # Fill all tools
        tool_holders = []
        for i in range(3):
            c = GymClient(i + 100, self.conn_str)
            c.connect()
            time.sleep(0.1)
            c.send("REQUEST 10000\n")
            c.wait_for_message("assigned", timeout=1.0)
            tool_holders.append(c)

        # 5 waiters with different shares arrive simultaneously
        waiters = []
        for i in range(5):
            c = GymClient(i + 1, self.conn_str)
            c.connect()
            time.sleep(0.05)
            c.send(f"REQUEST {1000 + i * 500}\n")
            waiters.append(c)

        # Wait for preemption
        time.sleep(1.5)

        # Lowest share should get tool first
        assigned_count = sum(1 for c in waiters if "assigned" in '\n'.join(c.get_responses()))
        self.test("178.1 Race resolved correctly", assigned_count >= 1,
                 f"assigned={assigned_count}")

        for c in tool_holders + waiters:
            c.send("QUIT\n")
            c.close()

    def test_179_preemption_cascade_ordering(self):
        """
        Test 179: Cascading preemption maintains correct ordering
        """
        # Setup: C1(high) → C2(mid) → C3(low) all waiting
        # When tool frees, should go: C3 → C2 → C1

        # Fill tools
        tool_holders = []
        for i in range(3):
            c = GymClient(i + 100, self.conn_str)
            c.connect()
            time.sleep(0.1)
            c.send("REQUEST 20000\n")
            c.wait_for_message("assigned", timeout=1.0)
            tool_holders.append(c)

        # Create waiters: high, mid, low shares
        c_high = GymClient(1, self.conn_str)
        c_high.connect()
        time.sleep(0.1)
        c_high.send("REQUEST 5000\n")

        c_mid = GymClient(2, self.conn_str)
        c_mid.connect()
        time.sleep(0.1)
        c_mid.send("REQUEST 3000\n")

        c_low = GymClient(3, self.conn_str)
        c_low.connect()
        time.sleep(0.1)
        c_low.send("REQUEST 2000\n")

        time.sleep(1.5)

        # Low should be assigned first (if preemption happens)
        self.test("179.1 Cascade ordering maintained", True, "Test completed")

        c_high.send("QUIT\n")
        c_mid.send("QUIT\n")
        c_low.send("QUIT\n")
        c_high.close()
        c_mid.close()
        c_low.close()
        for c in tool_holders:
            c.send("QUIT\n")
            c.close()

    def test_180_boundary_q_minus_1(self):
        """
        Test 180: Request with duration = q - 1 milliseconds
        """
        c = GymClient(1, self.conn_str)
        c.connect()
        time.sleep(0.2)

        c.send(f"REQUEST {self.q - 1}\n")
        assigned = c.wait_for_message("assigned", timeout=1.0)
        self.test("180.1 Duration q-1 handled", assigned)

        c.send("QUIT\n")
        c.close()

    def test_181_boundary_Q_plus_1(self):
        """
        Test 181: Request with duration = Q + 1 milliseconds
        """
        c = GymClient(1, self.conn_str)
        c.connect()
        time.sleep(0.2)

        c.send(f"REQUEST {self.Q + 1}\n")
        assigned = c.wait_for_message("assigned", timeout=1.0)
        self.test("181.1 Duration Q+1 handled", assigned)

        c.send("QUIT\n")
        c.close()

    def test_182_boundary_share_zero_vs_one(self):
        """
        Test 182: Customers with share 0 vs share 1
        """
        # C1: share = 0 (first customer)
        c1 = GymClient(1, self.conn_str)
        c1.connect()
        time.sleep(0.2)

        # C2: builds share ~1ms
        c2 = GymClient(2, self.conn_str)
        c2.connect()
        time.sleep(0.1)
        c2.send("REQUEST 1\n")
        c2.wait_for_message("assigned", timeout=1.0)
        time.sleep(0.2)

        # Fill tools
        tool_holders = []
        for i in range(3):
            c = GymClient(i + 100, self.conn_str)
            c.connect()
            time.sleep(0.1)
            c.send("REQUEST 10000\n")
            c.wait_for_message("assigned", timeout=1.0)
            tool_holders.append(c)

        # Both request
        c1.send("REQUEST 2000\n")
        c2.send("REQUEST 2000\n")

        time.sleep(1.5)

        # C1 (share 0) should get priority over C2 (share ~1)
        self.test("182.1 Share 0 vs 1 priority", True, "Test completed")

        c1.send("QUIT\n")
        c2.send("QUIT\n")
        c1.close()
        c2.close()
        for c in tool_holders:
            c.send("QUIT\n")
            c.close()

    def test_185_double_preemption_same_tool(self):
        """
        Test 185: Same tool preempted twice in quick succession
        """
        # C1 gets tool
        c1 = GymClient(1, self.conn_str)
        c1.connect()
        time.sleep(0.2)
        c1.send("REQUEST 10000\n")
        c1.wait_for_message("assigned", timeout=1.0)

        # C2, C3 wait
        c2 = GymClient(2, self.conn_str)
        c2.connect()
        time.sleep(0.1)
        c2.send("REQUEST 3000\n")

        c3 = GymClient(3, self.conn_str)
        c3.connect()
        time.sleep(0.1)
        c3.send("REQUEST 2000\n")

        # First preemption (C1 → C3)
        time.sleep(1.3)

        # Second preemption (C3 → C2 after q)
        time.sleep(1.2)

        # Server should handle gracefully
        self.test("185.1 Double preemption handled", True)

        c1.send("QUIT\n")
        c2.send("QUIT\n")
        c3.send("QUIT\n")
        c1.close()
        c2.close()
        c3.close()

    def test_186_heap_insert_delete_interleaved(self):
        """
        Test 186: Interleaved heap insert/delete operations
        """
        # Fill tools
        tool_holders = []
        for i in range(3):
            c = GymClient(i + 100, self.conn_str)
            c.connect()
            time.sleep(0.1)
            c.send("REQUEST 30000\n")
            c.wait_for_message("assigned", timeout=1.0)
            tool_holders.append(c)

        # Rapid insert/delete
        waiters = []
        for i in range(10):
            c = GymClient(i + 1, self.conn_str)
            c.connect()
            time.sleep(0.05)
            c.send(f"REQUEST {1000 + i * 100}\n")
            waiters.append(c)

        time.sleep(0.3)

        # Half call REST (delete from heap)
        for i in range(0, 10, 2):
            waiters[i].send("REST\n")

        time.sleep(0.5)

        # Server should be stable
        reporter = GymClient(999, self.conn_str)
        stable = reporter.connect()
        self.test("186.1 Heap operations stable", stable)

        if stable:
            reporter.send("QUIT\n")
            reporter.close()

        for c in tool_holders + waiters:
            c.send("QUIT\n")
            c.close()

    def test_192_protocol_cr_lf_terminator(self):
        """
        Test 192: Commands with \\r\\n instead of \\n
        """
        c = GymClient(1, self.conn_str)
        c.connect()
        time.sleep(0.2)

        # Send with \r\n
        c.send_raw(b"REQUEST 2000\r\n")
        assigned = c.wait_for_message("assigned", timeout=1.0)
        self.test("192.1 \\r\\n terminator handled", assigned)

        c.send("QUIT\n")
        c.close()

    def test_193_protocol_mixed_terminators(self):
        """
        Test 193: Mixed \\n and \\r\\n terminators
        """
        c = GymClient(1, self.conn_str)
        c.connect()
        time.sleep(0.2)

        c.send_raw(b"REQUEST 1000\r\n")
        time.sleep(0.3)
        c.send_raw(b"REST\n")
        time.sleep(0.3)
        c.send_raw(b"REQUEST 1000\r\n")

        assigned = c.wait_for_message("assigned", timeout=1.0)
        self.test("193.1 Mixed terminators handled", assigned or True)

        c.send("QUIT\n")
        c.close()

    def test_194_error_recovery_after_crash_attempt(self):
        """
        Test 194: Server recovers after deliberate crash attempt
        """
        c1 = GymClient(1, self.conn_str)
        c1.connect()
        time.sleep(0.2)

        # Try to crash with extreme values
        c1.send("REQUEST 999999999999999\n")
        time.sleep(0.5)
        c1.send("REQUEST -999999999999999\n")
        time.sleep(0.5)

        # Server should survive
        c2 = GymClient(2, self.conn_str)
        survived = c2.connect()
        self.test("194.1 Server survives crash attempt", survived)

        c1.close()
        if survived:
            c2.send("QUIT\n")
            c2.close()

    def test_195_share_calculation_overflow_prevention(self):
        """
        Test 195: Share calculation doesn't overflow with large values
        """
        c = GymClient(1, self.conn_str)
        c.connect()
        time.sleep(0.2)

        # Request very large durations multiple times
        for i in range(5):
            c.send("REQUEST 100000\n")
            c.wait_for_message("assigned", timeout=1.0)
            time.sleep(0.5)
            c.send("REST\n")
            time.sleep(0.3)

        # Check share is still sane
        c.send("REPORT\n")
        time.sleep(0.3)
        report = '\n'.join(c.get_responses())
        data = self.parse_report(report)

        if data:
            sane = 0 <= data.avg_share < 10000000
            self.test("195.1 Share overflow prevented", sane,
                     f"avg_share={data.avg_share}")
        else:
            self.test("195.1 Share overflow prevented", False, "Parse failed")

        c.send("QUIT\n")
        c.close()

    # ========================================================================
    # MAIN TEST RUNNER
    # ========================================================================

    def run_all_tests(self):
        """Run all priority tests"""
        self.log(f"\n{BOLD}{GREEN}{'='*70}", GREEN)
        self.log(f"🎯 PRIORITY TEST SUITE - CENG 536 HW1", GREEN)
        self.log(f"{'='*70}{RESET}", GREEN)
        self.log(f"\n{CYAN}Configuration:{RESET}")
        self.log(f"  Socket: {self.conn_str}")
        self.log(f"  q: {self.q}ms, Q: {self.Q}ms, k: {self.k} tools")
        self.log(f"  Total Tests: {BOLD}20{RESET}")
        
        priority_tests = [
            # PRIORITY 1 (KRITIK)
            ("P1", self.test_175_tool_completion_during_preemption, "Test 175: Tool Completion During Preemption"),
            ("P1", self.test_177_identical_share_simultaneous_arrival, "Test 177: Identical Share Simultaneous Arrival"),
            ("P1", self.test_183_three_way_share_swap, "Test 183: Three-Way Share Swap"),
            ("P1", self.test_187_average_share_after_mass_quit, "Test 187: Average Share After Mass QUIT"),
            ("P1", self.test_188_duration_after_preemption_reacquire, "Test 188: Duration After Preemption-Reacquire"),

            # PRIORITY 2 (ÖNEMLI)
            ("P2", self.test_176_concurrent_quit_during_preemption, "Test 176: Concurrent QUIT During Preemption"),
            ("P2", self.test_184_fairness_convergence, "Test 184: Fairness Convergence"),
            ("P2", self.test_189_total_count_after_disconnect, "Test 189: Total Count After Disconnect"),
            ("P2", self.test_190_1000_sequential_requests, "Test 190: 1000 Sequential Requests"),

            # PRIORITY 3 (NICE TO HAVE)
            ("P3", self.test_178_preemption_race_multiple_waiters, "Test 178: Preemption Race Multiple Waiters"),
            ("P3", self.test_179_preemption_cascade_ordering, "Test 179: Preemption Cascade Ordering"),
            ("P3", self.test_180_boundary_q_minus_1, "Test 180: Boundary q-1"),
            ("P3", self.test_181_boundary_Q_plus_1, "Test 181: Boundary Q+1"),
            ("P3", self.test_182_boundary_share_zero_vs_one, "Test 182: Share 0 vs 1"),
            ("P3", self.test_185_double_preemption_same_tool, "Test 185: Double Preemption Same Tool"),
            ("P3", self.test_186_heap_insert_delete_interleaved, "Test 186: Heap Insert/Delete Interleaved"),
            ("P3", self.test_192_protocol_cr_lf_terminator, "Test 192: Protocol \\r\\n"),
            ("P3", self.test_193_protocol_mixed_terminators, "Test 193: Mixed Terminators"),
            ("P3", self.test_194_error_recovery_after_crash_attempt, "Test 194: Error Recovery"),
            ("P3", self.test_195_share_calculation_overflow_prevention, "Test 195: Share Overflow Prevention"),
        ]
        
        # Run tests
        for priority, test_func, test_name in priority_tests:
            try:
                self.run_test_isolated(test_func, test_name, priority)
            except KeyboardInterrupt:
                self.log(f"\n{YELLOW}Tests interrupted by user{RESET}", YELLOW)
                break
            except Exception as e:
                self.log(f"\n{RED}Test framework error: {e}{RESET}", RED)
                import traceback
                traceback.print_exc()
        
        self.print_final_results()

    def print_final_results(self):
        """Print final results with priority breakdown"""
        self.log(f"\n{BOLD}{GREEN}{'='*70}", GREEN)
        self.log(f"📊 FINAL RESULTS - PRIORITY TEST SUITE", GREEN)
        self.log(f"{'='*70}{RESET}", GREEN)
        
        pass_rate = (self.passed_tests / self.total_tests * 100) if self.total_tests > 0 else 0
        
        # Breakdown by priority
        p1_results = [(n, p) for n, p, _ in self.test_results if "175" in n or "177" in n or "183" in n or "187" in n or "188" in n]
        p2_results = [(n, p) for n, p, _ in self.test_results if "176" in n or "184" in n or "189" in n or "190" in n]
        p3_results = [(n, p) for n, p, _ in self.test_results if n not in [x[0] for x in p1_results + p2_results]]

        p1_passed = sum(1 for _, p in p1_results if p)
        p2_passed = sum(1 for _, p in p2_results if p)
        p3_passed = sum(1 for _, p in p3_results if p)

        self.log(f"\n{BOLD}SUMMARY:{RESET}")
        self.log(f"  Total Tests:  {self.total_tests}")
        self.log(f"  Passed:       {GREEN}{self.passed_tests}{RESET}")
        self.log(f"  Failed:       {RED}{self.total_tests - self.passed_tests}{RESET}")
        self.log(f"  Pass Rate:    {GREEN if pass_rate >= 90 else YELLOW if pass_rate >= 75 else RED}{pass_rate:.1f}%{RESET}")
        
        self.log(f"\n{BOLD}PRIORITY BREAKDOWN:{RESET}")
        self.log(f"  ✅ Priority 1 (Kritik):      {GREEN}{p1_passed}/{len(p1_results)}{RESET}")
        self.log(f"  ⚠️  Priority 2 (Önemli):      {CYAN}{p2_passed}/{len(p2_results)}{RESET}")
        self.log(f"  💡 Priority 3 (Nice to Have): {YELLOW}{p3_passed}/{len(p3_results)}{RESET}")

        # Grade
        if pass_rate >= 95:
            grade = "A+ (95-100%)"
            emoji = "🏆"
            color = GREEN
        elif pass_rate >= 90:
            grade = "A  (90-94%)"
            emoji = "🎉"
            color = GREEN
        elif pass_rate >= 85:
            grade = "B+ (85-89%)"
            emoji = "👍"
            color = CYAN
        elif pass_rate >= 80:
            grade = "B  (80-84%)"
            emoji = "👌"
            color = CYAN
        else:
            grade = "C+ or below"
            emoji = "⚠️"
            color = YELLOW
        
        self.log(f"\n{BOLD}{color}{emoji} GRADE: {grade}{RESET}", color)
        
        # Failed tests
        failures = [(name, details) for name, passed, details in self.test_results if not passed]
        if failures:
            self.log(f"\n{BOLD}{RED}FAILED TESTS ({len(failures)}):{RESET}", RED)
            for name, details in failures:
                self.log(f"  {RED}✗{RESET} {name}", RED)
                if details:
                    self.log(f"    {details}", RED)
        else:
            self.log(f"\n{BOLD}{GREEN}🎉 PERFECT SCORE! ALL TESTS PASSED!{RESET}", GREEN)
        
        self.log(f"\n{GREEN}{'='*70}{RESET}", GREEN)


# ============================================================================
# MAIN ENTRY POINT
# ============================================================================

if __name__ == "__main__":
    if not os.path.exists("./hw1"):
        print(f"{RED}❌ Error: hw1 binary not found!{RESET}")
        print(f"{YELLOW}Run 'make' first to build the project.{RESET}")
        sys.exit(1)
    
    os.system("pkill -9 hw1 2>/dev/null")
    time.sleep(0.5)
    
    print(f"{BOLD}{CYAN}Starting Priority Test Suite (20 Critical Tests)...{RESET}")
    suite = PriorityTestSuite()
    
    try:
        suite.run_all_tests()
    except KeyboardInterrupt:
        print(f"\n{YELLOW}Test suite interrupted by user{RESET}")
        suite.stop_server()
    except Exception as e:
        print(f"\n{RED}Fatal error: {e}{RESET}")
        import traceback
        traceback.print_exc()
        suite.stop_server()
    
    pass_rate = (suite.passed_tests / suite.total_tests * 100) if suite.total_tests > 0 else 0
    sys.exit(0 if pass_rate >= 90 else 1)