#!/usr/bin/env python3
"""Test 86-87 after fixes"""
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

    print("="*70)
    print("Testing 86-87 (Fixed)")
    print("="*70)

    tester.run_test_isolated(tester.test_86_multi_hop_preemption_4_levels, "Test 86")
    tester.run_test_isolated(tester.test_87_preemption_with_q_limit_enforcement, "Test 87")

    print("\n" + "="*70)
    print(f"RESULTS: {tester.passed}/{tester.total} passed")
    print("="*70)

    sys.exit(0 if tester.failed == 0 else 1)
