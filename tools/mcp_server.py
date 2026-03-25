#!/usr/bin/env python3
"""
MCP Server for Ingnomia test infrastructure.

Wraps the CLI test harness and JSON control channel as MCP tools,
so Claude Code agents can build, test, screenshot, and interact
with the game directly from the tool palette.

Setup: Add to .mcp.json in project root:
{
  "mcpServers": {
    "ingnomia-test": {
      "command": "python3",
      "args": ["tools/mcp_server.py"],
      "cwd": "/Users/seanellul/Code/games/Ingnomia"
    }
  }
}
"""

import json
import os
import subprocess

from mcp.server.fastmcp import FastMCP

PROJECT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD_DIR = os.path.join(PROJECT_DIR, "build")
BINARY = os.path.join(BUILD_DIR, "Ingnomia")
DEFAULT_SAVE = os.path.join(PROJECT_DIR, "content", "test_saves", "smoke_test")
REFERENCE_DIR = os.path.join(PROJECT_DIR, "content", "test_reference")

mcp = FastMCP("ingnomia-test")


def get_cpu_count():
    try:
        result = subprocess.run(["sysctl", "-n", "hw.ncpu"], capture_output=True, text=True)
        return result.stdout.strip()
    except Exception:
        return str(os.cpu_count() or 4)


@mcp.tool()
def build_game() -> str:
    """Build Ingnomia from source. Returns success/failure and any compiler warnings or errors."""
    cpu_count = get_cpu_count()
    try:
        result = subprocess.run(
            ["cmake", "--build", BUILD_DIR, "--parallel", cpu_count],
            capture_output=True,
            text=True,
            cwd=PROJECT_DIR,
            timeout=600,
        )
    except subprocess.TimeoutExpired:
        return json.dumps({"success": False, "error": "Build timed out after 600s"})

    output = result.stdout + result.stderr
    lines = output.strip().split("\n")

    warnings = sum(1 for l in lines if "warning:" in l.lower() and "warnings generated" not in l.lower())
    errors = sum(1 for l in lines if "error:" in l.lower())

    return json.dumps({
        "success": result.returncode == 0,
        "warnings": warnings,
        "errors": errors,
        "output": "\n".join(lines[-30:]),
    }, indent=2)


@mcp.tool()
def run_smoke_test(save_path: str = "", ticks: int = 100, screenshot_delay: int = 60) -> str:
    """Full smoke test: build, load a save, run ticks, take a screenshot, and return metrics.
    Uses the default test save unless overridden."""
    if not save_path:
        save_path = DEFAULT_SAVE

    screenshot_path = "/tmp/ingnomia_smoke_test.png"
    metrics_path = "/tmp/ingnomia_smoke_metrics.json"

    # Build first
    build_result = json.loads(build_game())
    if not build_result["success"]:
        return json.dumps({"build": "failed", "build_output": build_result["output"]}, indent=2)

    # Run game
    cmd = [
        BINARY,
        "--test-mode",
        "--load-save", save_path,
        "--screenshot", screenshot_path,
        "--screenshot-delay", str(screenshot_delay),
        "--run-ticks", str(ticks),
        "--metrics-out", metrics_path,
    ]

    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=120, cwd=PROJECT_DIR)
    except subprocess.TimeoutExpired:
        return json.dumps({"build": "success", "test": "timeout", "message": "Game did not exit within 120s"})

    metrics = {}
    if os.path.exists(metrics_path):
        with open(metrics_path) as f:
            metrics = json.load(f)

    metrics["build"] = "success"
    metrics["screenshot_path"] = screenshot_path if os.path.exists(screenshot_path) else None
    metrics["game_stderr"] = result.stderr[-500:] if result.stderr else ""

    return json.dumps(metrics, indent=2)


@mcp.tool()
def take_screenshot(save_path: str, output_path: str = "/tmp/ingnomia_screenshot.png", delay: int = 60) -> str:
    """Load a save game, wait for rendering to stabilize, and take a screenshot.
    Returns the screenshot path (viewable with the Read tool)."""
    cmd = [
        BINARY,
        "--test-mode",
        "--load-save", save_path,
        "--screenshot", output_path,
        "--screenshot-delay", str(delay),
    ]

    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=120, cwd=PROJECT_DIR)
    except subprocess.TimeoutExpired:
        return json.dumps({"error": "timeout"})

    return json.dumps({
        "screenshot": output_path if os.path.exists(output_path) else None,
        "exit_code": result.returncode,
    }, indent=2)


@mcp.tool()
def run_ticks(save_path: str, count: int = 100) -> str:
    """Load a save and advance the game by N ticks. Returns performance metrics as JSON."""
    metrics_path = "/tmp/ingnomia_tick_metrics.json"

    cmd = [
        BINARY,
        "--test-mode",
        "--load-save", save_path,
        "--run-ticks", str(count),
        "--metrics-out", metrics_path,
    ]

    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=120, cwd=PROJECT_DIR)
    except subprocess.TimeoutExpired:
        return json.dumps({"error": "timeout"})

    if os.path.exists(metrics_path):
        with open(metrics_path) as f:
            return json.dumps(json.load(f), indent=2)

    return json.dumps({"error": "no metrics output", "exit_code": result.returncode})


@mcp.tool()
def compare_screenshots(reference: str, current: str, threshold: float = 5.0) -> str:
    """Compare two screenshots for visual regression. Returns diff percentage and generates a diff image."""
    script = os.path.join(PROJECT_DIR, "tools", "compare_screenshots.py")
    diff_out = "/tmp/ingnomia_diff.png"

    cmd = [
        "python3", script,
        reference, current,
        "--threshold", str(threshold),
        "--diff-out", diff_out,
    ]

    result = subprocess.run(cmd, capture_output=True, text=True)

    try:
        return result.stdout
    except Exception:
        return json.dumps({"error": "comparison failed", "output": result.stdout + result.stderr})


@mcp.tool()
def game_command(command: dict, save_path: str = "") -> str:
    """Send a JSON command to a running game instance via the stdin control channel.
    Available commands: get_state, screenshot, advance_ticks, load_save, set_paused,
    set_game_speed, build, spawn_creature, get_metrics, save_game, quit."""
    cmd = [BINARY, "--test-mode"]
    if save_path:
        cmd.extend(["--load-save", save_path])

    cmd_json = json.dumps(command)
    input_data = cmd_json + "\n" + json.dumps({"cmd": "quit"}) + "\n"

    try:
        result = subprocess.run(
            cmd,
            input=input_data,
            capture_output=True,
            text=True,
            timeout=60,
            cwd=PROJECT_DIR,
        )
    except subprocess.TimeoutExpired:
        return json.dumps({"error": "timeout"})

    # Parse responses from stdout (one JSON per line)
    responses = []
    for line in result.stdout.strip().split("\n"):
        line = line.strip()
        if not line:
            continue
        try:
            responses.append(json.loads(line))
        except json.JSONDecodeError:
            continue

    # Return the first non-ready, non-quit response
    for resp in responses:
        if resp.get("status") != "ready":
            return json.dumps(resp, indent=2)

    return json.dumps({"responses": responses} if responses else {"error": "no response", "stderr": result.stderr[-500:]}, indent=2)


if __name__ == "__main__":
    mcp.run()
