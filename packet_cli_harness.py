#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import pty
import select
import subprocess
import sys
import time
from pathlib import Path


CLEAR_SEQ = "\x1b[H\x1b[2J"
DEFAULT_BINARY = "./packet_cli"


def strip_ansi(text: str) -> str:
    out: list[str] = []
    i = 0
    n = len(text)

    while i < n:
        ch = text[i]
        if ch == "\x1b":
            i += 1
            if i < n and text[i] == "[":
                i += 1
                while i < n and not ("@" <= text[i] <= "~"):
                    i += 1
                if i < n:
                    i += 1
            continue
        out.append(ch)
        i += 1

    return "".join(out)


def token_to_chars(token: str) -> str:
    mapping = {
        "SPACE": " ",
        "TAB": "\t",
        "ENTER": "\n",
        "QUIT": "q",
    }
    key = token.strip()
    upper = key.upper()

    if upper in mapping:
        return mapping[upper]
    if len(key) == 1:
        return key
    raise ValueError(f"Unsupported command token: {token}")


def read_available(fd: int, quiet_ms: int, max_ms: int) -> str:
    chunks: list[bytes] = []
    deadline = time.time() + (quiet_ms / 1000.0)
    hard_deadline = time.time() + (max_ms / 1000.0)

    while True:
        if time.time() >= hard_deadline:
            break
        timeout = max(0.0, deadline - time.time())
        ready, _, _ = select.select([fd], [], [], timeout)
        if not ready:
            break
        try:
            data = os.read(fd, 65536)
        except OSError:
            break
        if not data:
            break
        chunks.append(data)
        deadline = time.time() + (quiet_ms / 1000.0)

    return b"".join(chunks).decode("utf-8", errors="replace")


def extract_latest_snapshot(raw_text: str) -> str:
    parts = raw_text.split(CLEAR_SEQ)
    latest = parts[-1] if parts else raw_text
    latest = strip_ansi(latest).replace("\r", "")
    return latest.strip("\n")


def write_report(
    out_path: Path,
    binary: str,
    commands: list[str],
    snapshots: list[tuple[str, str]],
) -> None:
    with out_path.open("w", encoding="utf-8") as f:
        f.write("PACKET_CLI_HARNESS_REPORT\n")
        f.write(f"binary: {binary}\n")
        f.write(f"commands: {','.join(commands)}\n")
        f.write(f"snapshots: {len(snapshots)}\n\n")

        for index, (label, snap) in enumerate(snapshots):
            f.write(f"=== SNAPSHOT {index:02d}: {label} ===\n")
            f.write(snap)
            f.write("\n\n")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Run packet_cli through a PTY and collect clean textual snapshots."
    )
    parser.add_argument(
        "--binary",
        default=DEFAULT_BINARY,
        help="Path to packet_cli binary. Default: ./packet_cli",
    )
    parser.add_argument(
        "--commands",
        default="n,v,SPACE,n,w,n,QUIT",
        help="Comma-separated command tokens. Supports single chars plus SPACE, TAB, ENTER, QUIT.",
    )
    parser.add_argument(
        "--startup-ms",
        type=int,
        default=150,
        help="Initial quiet read window after startup.",
    )
    parser.add_argument(
        "--between-ms",
        type=int,
        default=180,
        help="Quiet read window after each command.",
    )
    parser.add_argument(
        "--max-read-ms",
        type=int,
        default=600,
        help="Hard cap for one read window, useful when scripted commands enable auto-run.",
    )
    parser.add_argument(
        "--output",
        default="packet_cli_harness_report.txt",
        help="Where to write the final report.",
    )
    args = parser.parse_args()

    binary = args.binary
    commands = [token.strip() for token in args.commands.split(",") if token.strip()]
    out_path = Path(args.output)

    master_fd, slave_fd = pty.openpty()
    proc = subprocess.Popen(
        [binary],
        stdin=slave_fd,
        stdout=slave_fd,
        stderr=slave_fd,
        close_fds=True,
    )
    os.close(slave_fd)

    snapshots: list[tuple[str, str]] = []

    try:
        initial_raw = read_available(master_fd, args.startup_ms, args.max_read_ms)
        snapshots.append(("startup", extract_latest_snapshot(initial_raw)))

        for token in commands:
            chars = token_to_chars(token)
            os.write(master_fd, chars.encode("utf-8"))
            raw = read_available(master_fd, args.between_ms, args.max_read_ms)
            snapshots.append((token, extract_latest_snapshot(raw)))
            if token.upper() == "QUIT":
                break

        try:
            proc.wait(timeout=1.0)
        except subprocess.TimeoutExpired:
            proc.terminate()
            try:
                proc.wait(timeout=1.0)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait(timeout=1.0)
    finally:
        try:
            os.close(master_fd)
        except OSError:
            pass

    write_report(out_path, binary, commands, snapshots)
    print(f"Wrote {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
