#!/usr/bin/env python3
"""Deterministic QoS1 loss simulation for the T-003 acceptance boundary."""

from __future__ import annotations

import argparse
import random


def simulate(loss_percent: int, seed: int) -> tuple[int, int]:
    rng = random.Random(seed + loss_percent)
    delivered = 0
    expired = 0
    for request_number in range(256):
        issued = 1000
        expires = issued + 2000
        deadline = issued + 80
        attempts = 1
        sequence = request_number * 4 + 1
        while True:
            if deadline >= expires or attempts > 3:
                expired += 1
                break
            if rng.randrange(100) >= loss_percent:
                delivered += 1
                break
            if attempts >= 3:
                expired += 1
                break
            attempts += 1
            sequence += 1
            remaining = expires - deadline
            delay = min(80, remaining // 2)
            if delay <= 0:
                expired += 1
                break
            deadline += delay
        if attempts > 3:
            raise AssertionError("more than three QoS1 attempts")
        if sequence < request_number * 4 + 1 or deadline > expires:
            raise AssertionError("retry mutated immutable lifetime boundary")
    return delivered, expired


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--loss", default="0,1,5,20,50")
    args = parser.parse_args()
    rates = [int(item) for item in args.loss.split(",") if item]
    if any(rate < 0 or rate > 100 for rate in rates):
        parser.error("loss rates must be in 0..100")
    for rate in rates:
        delivered, expired = simulate(rate, args.seed)
        print(f"loss={rate}% delivered={delivered} expired={expired}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
