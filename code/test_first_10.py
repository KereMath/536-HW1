#!/usr/bin/env python3
"""Test first 10 tests"""
import sys
import os
sys.path.insert(0, os.path.dirname(__file__))
from ultimate_test import *

if __name__ == "__main__":
    os.system("pkill -9 hw1 2>/dev/null")
    time.sleep(0.5)

    tester = GymTester(
        conn_str="@/tmp/perfect_gym.sock",
        q=1000,
        Q=5000,
        k=3
    )

    print("Testing first 10 tests...")
    tests = [
        (tester.test_01_basic_connection, "Test 1"),
        (tester.test_02_basic_request, "Test 2"),
        (tester.test_03_report_command, "Test 3"),
        (tester.test_04_rest_command, "Test 4"),
        (tester.test_05_quit_command, "Test 5"),
        (tester.test_06_share_starts_at_zero, "Test 6"),
        (tester.test_07_share_increases, "Test 7"),
        (tester.test_08_average_share_assignment, "Test 8"),
        (tester.test_09_queue_ordering, "Test 9"),
        (tester.test_10_multiple_tools, "Test 10"),
    ]

    for test_func, name in tests:
        tester.run_test_isolated(test_func, name)

    print(f"\n{'='*70}")
    print(f"RESULTS: {tester.passed}/{tester.total} passed ({tester.failed} failed)")
    print(f"{'='*70}")

    sys.exit(0 if tester.failed == 0 else 1)
