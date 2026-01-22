"""
Throughput tests for WiFi/BLE Coexistence.

Tests are parametrized to run across all discovered profiles.
Each profile is built into its own build directory and tested separately.

Usage:
    # Build all profiles:
    make build-all

    # Run all tests:
    source .venv/bin/activate
    pytest pytest_throughput.py -v

    # Run specific profile:
    pytest pytest_throughput.py -v -k balanced
"""

import logging
from typing import Callable

import pytest
from pytest_embedded_idf import IdfDut

from conftest import PROFILES, parse_throughput_output


@pytest.mark.esp32c5
@pytest.mark.parametrize('config', PROFILES, indirect=True)
@pytest.mark.timeout(300)
def test_throughput(
    dut: IdfDut,
    config: str,
    record_result: Callable[[str, object], None],
) -> None:
    """Run WiFi throughput tests and log results for each profile.

    The firmware runs throughput tests with and without BLE active.
    Results are parsed and recorded from the test summary output.
    """
    logging.info(f'Testing profile: {config}')

    # Wait for test suite to start
    dut.expect('Starting speed test suite', timeout=60)

    # Wait for completion
    dut.expect('Test suite complete.', timeout=240)

    # Parse results from captured output
    output = dut.pexpect_proc.before
    if isinstance(output, bytes):
        output = output.decode('utf-8', errors='replace')

    results = parse_throughput_output(output)
    assert results, 'No test results found in output'

    # Record all results for summary
    for name, data in results.items():
        record_result(name, data)
        if isinstance(data, dict):
            assert data['passed'], f'{name} test failed'

    logging.info(f'Profile {config} completed successfully')
