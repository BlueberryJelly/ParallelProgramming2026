#!/usr/bin/env python3

import argparse
import json
import pathlib

import numpy as np

DEFAULT_SIZES = [200, 400, 800, 1200, 1600, 2000]


def generate_matrix(n: int, seed: int) -> np.ndarray:
    rng = np.random.default_rng(seed)
    return rng.uniform(-10.0, 10.0, size = (n, n))

def main() -> None: