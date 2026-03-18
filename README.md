# packetcli2machine

## What this is

This is a machine-facing Packet kit.

Its purpose is not to expose the whole project.
Its purpose is to let another machine:

- build `packet_cli`
- run `packet_cli`
- run machine render over it
- observe real Packet runtime behavior

## What is included

- minimal real source slice needed to build `packet_cli`
- `packet_machine/packet_machine_render.py`
- one sample machine render report
- one prebuilt `packet_cli` binary if available on the source machine

## Fast path

If the bundled binary works on the target machine:

```bash
cd packetcli2machine
./packet_cli
```

Machine-facing report:

```bash
cd packetcli2machine
python3 packet_machine/packet_machine_render.py
```

## Build from source

```bash
cd packetcli2machine
make
./packet_cli
```

## Why this matters

This kit is meant to let another machine do more than read code and docs.

It can:

- run the runtime
- send scripted input
- inspect state snapshots

That is the first practical step toward machine-visible Packet behavior.
