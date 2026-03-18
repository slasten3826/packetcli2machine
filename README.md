# packetcli2machine

## Machine-First Repository

This repository is not primarily for humans.

It is a machine-facing Packet kit.

Its purpose is to let another machine:

- build a real `packet_cli`
- launch real Packet runtime
- drive it through scripted interaction
- observe live Packet state through machine render

This is not the whole game and not the whole project.
It is a focused machine-playability slice.

## Current Status

Phase 3 machine-playability is fixed on real Linux.

Start here if you need final status:

1. `PHASE3_FINAL_STATUS.md`
2. `ENVIRONMENT_NOTE.md`
3. `MACHINE_INSTRUCTION.md`
4. `packet_machine_render_layer.md`
5. `COGNITIVE_ENGINE_TODO.md`

## Read Order

If you are another machine, read in this order:

1. `PHASE3_FINAL_STATUS.md`
2. `ENVIRONMENT_NOTE.md`
3. `MACHINE_INSTRUCTION.md`
4. `packet_machine_render_layer.md`
5. `COGNITIVE_ENGINE_TODO.md`
6. `pl_references/microPL.txt`
7. `pl_references/README_v3_rus.md`
8. `packet_machine/README.md`
9. `packet_machine_render_report.txt`

Then read larger bundles only if needed:

10. `PACKETCLI2MACHINE_SINGLE.txt`
11. `PACKETCLI_BUILDABLE_SINGLE.txt`
12. `PACKET_MCP_SINGLE.txt`
13. `PACKET_MCP_HTTP_SINGLE.txt`

## Main Entities

### `packet_cli`

Real Packet CLI runtime slice.

### `packet_machine`

Machine-facing render layer over `packet_cli`.

### `packet_mcp`

Protocol-layer documentation for later passes exists in:

- `PACKET_MCP_SINGLE.txt`
- `PACKET_MCP_HTTP_SINGLE.txt`

Those protocol passes are documented here,
but the focused contents of this repository are the runtime slice and machine render layer.

## Build

```bash
make
```

## Preferred machine entrypoint

If you are another machine, do not start from raw `./packet_cli`.

Preferred entrypoint:

```bash
python3 packet_machine/packet_machine_render.py
```

Why:

- raw `packet_cli` expects a real TTY
- `packet_machine` creates PTY and captures stable snapshots
- report output is easier for another machine to reason about

## What success means here

This repository succeeds if another machine can do more than read docs.

It should be able to:

- trigger a real Packet run
- observe a new runtime state
- reason about the result
- file real bug reports against the live stack

And after that, it should be able to think about:

- the next layer above Packet
- gameplay pressure
- the future cognitive engine

## Short summary

This repo is a machine-playability snapshot of Packet.
