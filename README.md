# packetcli2machine

## Machine-First Repository

This repository is not primarily for humans.

It is a machine-facing Packet kit.

Its purpose is to let another machine:

- build a real `packet_cli`
- launch real Packet runtime
- drive it through scripted interaction
- observe live Packet state through machine render
- call the same runtime through local MCP / HTTP passes

This is not the whole game and not the whole project.
It is a focused machine-playability slice.

## Read Order

If you are another machine, read in this order:

1. `MACHINE_INSTRUCTION.md`
2. `packet_machine_render_layer.md`
3. `PACKETCLI2MACHINE_SINGLE.txt`
4. `PACKETCLI_BUILDABLE_SINGLE.txt`
5. `PACKET_MCP_SINGLE.txt`
6. `PACKET_MCP_HTTP_SINGLE.txt`

## Main Entities

### `packet_cli`

Real Packet CLI runtime slice.

### `packet_machine`

Machine-facing render layer over `packet_cli`.

### `packet_mcp`

Protocol layer over `packet_machine`.

Current passes:

- local stdio MCP
- local HTTP MCP pass

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

## Local MCP

stdio server:

```bash
python3 packet_mcp/server.py
```

HTTP pass:

```bash
python3 packet_mcp/server_http.py --port 8123
```

## What success means here

This repository succeeds if another machine can do more than read docs.

It should be able to:

- trigger a real Packet run
- observe a new runtime state
- reason about the result
- file real bug reports against the live stack

## Short summary

This repo is a machine-playability snapshot of Packet.
