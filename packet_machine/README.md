# Packet Machine

## What this is

`packet_machine` is the first explicit machine-facing manifestation layer for `Packet`.

It does not replace:

- `Packet`
- `Packet Adventure`
- `packet_cli`

It sits beside them.

## Purpose

This layer exists so another machine can:

- launch a real Packet runtime
- send scripted input
- capture clean textual state snapshots
- reason about real Packet behavior

## Current first implementation

Current implementation is a machine render wrapper over `packet_cli`.

That means:

- runtime stays real
- presentation stays textual
- observation becomes machine-friendly

## Why this matters

Without this layer,
another machine mostly reads:

- code
- docs
- screenshots
- reports

With this layer,
another machine can also observe:

- repeatable Packet runtime behavior

## First entrypoint

Use:

```bash
cd /home/slasten/dev/packet3
make packet_cli
python3 packet_machine/packet_machine_render.py
```

This writes a plain-text report containing a startup snapshot and one snapshot after each scripted command.

## Scope

This is not yet a full standalone Packet runtime export.

It is the first explicit machine-facing render/entity in the project.
