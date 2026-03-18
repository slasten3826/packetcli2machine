# Packet CLI Machine Harness

## Status

Implemented.

This document explains how another machine can interact with `packet_cli`
without needing to manually drive a live terminal session.

## Purpose

`packet_cli` is useful for human/dev inspection,
but another LLM or agent benefits from a stable harness that can:

- launch it
- send scripted commands
- capture cleaned textual snapshots
- produce one report file

That harness now exists.

## Files

- `scripts/packet_cli_harness.py`
- `packet_cli`

## Build first

```bash
cd /home/slasten/dev/packet3
make packet_cli
```

## Basic run

```bash
cd /home/slasten/dev/packet3
python3 scripts/packet_cli_harness.py
```

This writes:

- `packet_cli_harness_report.txt`

## Custom scripted run

Example:

```bash
cd /home/slasten/dev/packet3
python3 scripts/packet_cli_harness.py \
  --commands "n,v,SPACE,n,w,n,r,n,QUIT" \
  --output packet_cli_report_custom.txt
```

## Command tokens

The harness accepts:

- single chars:
  - `w`
  - `a`
  - `s`
  - `d`
  - `n`
  - `v`
  - `r`
  - `f`
  - `g`
  - `m`
  - `[`
  - `]`
  - `-`
  - `=`
- named tokens:
  - `SPACE`
  - `TAB`
  - `ENTER`
  - `QUIT`

## What the report contains

The report is plain text and contains:

- one startup snapshot
- one snapshot after each scripted command

Each snapshot is the latest rendered Packet CLI frame after ANSI cleanup.

## Why this matters

This makes `packet_cli` much more machine-friendly:

- another agent does not need to manually play the live TTY
- another agent can inspect repeatable snapshots from scripted interaction
- this is a good first bridge for asking outside machines what they think of the current Packet substrate

## Recommended prompt to another machine

Suggested instruction:

> Read `packet_cli_harness_report.txt` as a sequence of Packet CLI snapshots.  
> Explain what Packet world state seems to be doing, what is already game-like, and what still looks like substrate rather than gameplay.

## Summary

The harness is not a new renderer.

It is a machine-facing interaction layer over the existing CLI manifestation.
