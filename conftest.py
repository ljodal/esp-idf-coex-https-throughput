"""
pytest-embedded configuration for WiFi/BLE Coexistence throughput testing.

Profiles are auto-discovered from sdkconfig.* files.
Tests are skipped if build directory doesn't exist.
Results are saved to test_results.json and printed at end of run.

Usage:
    # Build all profiles:
    make build-all

    # Run tests:
    source .venv/bin/activate
    pytest pytest_throughput.py -v

    # Run specific profile:
    pytest pytest_throughput.py -v -k balanced
"""

import json
import re
from datetime import datetime
from pathlib import Path
from typing import Callable

import pytest


def discover_profiles() -> list[str]:
    """Auto-discover profiles from sdkconfig.* files."""
    project_dir = Path(__file__).parent
    profiles = []
    for f in project_dir.glob('sdkconfig.*'):
        name = f.name.replace('sdkconfig.', '')
        # Skip non-profile files
        if name not in ('defaults', 'old', 'local') and not name.startswith('ci'):
            profiles.append(name)
    return sorted(profiles)


# Auto-discovered profiles
PROFILES = discover_profiles()

# Session-wide results storage
_results: dict[str, dict] = {}


def pytest_terminal_summary(
    terminalreporter: pytest.TerminalReporter,
    exitstatus: int,
    config: pytest.Config,
) -> None:
    """Print throughput summary using pytest's terminal writer."""
    if not _results:
        return

    terminalreporter.write_sep("=", 'throughput results', green=True, bold=True)

    # Use pytest's terminal writer for proper formatting
    tw = terminalreporter._tw

    # Print table header
    tw.line(f'{"Profile":<15} {"Test":<20} {"Throughput":>15}   {"Status"}')
    tw.sep('-')

    for profile, data in sorted(_results.items()):
        first = True
        for key, value in data.items():
            if isinstance(value, dict) and 'throughput' in value:
                profile_col = profile if first else ''
                name = value.get('name', key)
                status = 'PASS' if value.get('passed') else 'FAIL'
                tw.line(f'{profile_col:<15} {name:<20} {value["throughput"]:>12.2f} Mbit/s  {status}')
                first = False
        if 'min_free_heap' in data:
            profile_col = profile if first else ''
            tw.line(f'{profile_col:<15} {"Min free heap":<20} {data["min_free_heap"]:>12} bytes')


@pytest.fixture(scope='session')
def target() -> str:
    """Target chip for pytest-embedded."""
    return 'esp32c5'


@pytest.fixture
def config(request: pytest.FixtureRequest) -> str:
    """Get config name from test parametrization."""
    return request.param


@pytest.fixture
def build_dir(config: str) -> str:
    """Build directory for the current profile.

    Skips test if build directory doesn't exist.
    """
    project_dir = Path(__file__).parent
    build_path = project_dir / f'build_{config}'

    if not build_path.exists():
        pytest.skip(f"Build directory not found: build_{config} (run 'make PROFILE={config} build')")

    return str(build_path)


@pytest.fixture
def record_result(config: str) -> Callable[[str, object], None]:
    """Record a result for the current profile."""
    if config not in _results:
        _results[config] = {}

    def _record(key: str, value: object) -> None:
        _results[config][key] = value

    return _record


def parse_throughput_output(output: str) -> dict:
    """Parse throughput test results from serial output.

    Handles ESP-IDF log format: I (timestamp) TAG: message
    Returns dict with test results and heap info.
    """
    results = {}

    # Strip ESP-IDF log prefix: "I (12345) tag: " -> content
    def strip_log_prefix(line: str) -> str:
        match = re.match(r'^[IWEDV]\s*\(\d+\)\s*[\w_]+:\s*(.*)$', line)
        return match.group(1) if match else line

    # Pattern for summary table lines (after stripping prefix):
    # "WiFi only                           5.94         OK"
    summary_pattern = re.compile(r'^(.+?)\s{2,}([\d.]+|-)\s+(OK|FAIL)\s*$')

    in_summary = False
    for line in output.split('\n'):
        line = strip_log_prefix(line.strip())

        if 'TEST SUMMARY' in line:
            in_summary = True
            continue

        if in_summary and line.startswith('===') and results:
            in_summary = False
            continue

        if in_summary and line and not line.startswith('-'):
            match = summary_pattern.match(line)
            if match:
                name = match.group(1).strip()
                speed = float(match.group(2)) if match.group(2) != '-' else 0.0
                status = match.group(3)
                # Use original name, just normalize for dict key
                key = name.lower().replace(' ', '_')
                results[key] = {'name': name, 'throughput': speed, 'passed': status == 'OK'}

        heap_match = re.search(r'Minimum free heap:\s*(\d+)\s*bytes', line)
        if heap_match:
            results['min_free_heap'] = int(heap_match.group(1))

    return results
