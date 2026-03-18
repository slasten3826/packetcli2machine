# Machine Instruction

## What this folder is

This folder is a standalone machine-facing Packet kit.

It exists so another machine can:

- build a real `packet_cli`
- run a real Packet runtime slice
- observe runtime behavior through a machine-facing render layer

This is not the whole project.
This is not the whole game.
This is not a fake stub.

It is a minimal real Packet CLI slice plus a machine-facing observation layer.

## Current status

Phase 3 is fixed on real Linux.

That means:

- startup snapshot works
- post-startup snapshots work
- repeated `n` advances the runtime
- `frame` and `sweep` can be observed through machine render

## What to read first

Read in this order:

1. `PHASE3_FINAL_STATUS.md`
2. `README.md`
3. `packet_machine_render_layer.md`
4. `packet_machine/README.md`
5. `packet_machine_render_report.txt`

Then inspect source if needed:

- `packet/include/packet/core.h`
- `packet/include/packet/render_cli.h`
- `packet/src/render_cli.c`
- `platform/cli/main.c`
- `include/packet_adventure/runtime_internal.h`

## What to run

If you want to build from source:

```bash
cd packetcli2machine
make
```

If you want the machine-facing path:

```bash
cd packetcli2machine
python3 packet_machine/packet_machine_render.py
```

This writes:

- `packet_machine_render_report.txt`

Recommended verification:

```bash
python3 packet_machine/packet_machine_render.py --commands "n,QUIT"
python3 packet_machine/packet_machine_render.py --commands "n,n,n,n,n,QUIT"
```

## Important note about `packet_cli`

`./packet_cli` expects a real terminal/TTY.

So if you are another machine, agent, or automated tool,
the preferred entrypoint is:

```bash
python3 packet_machine/packet_machine_render.py
```

That script:

- launches `packet_cli` through a PTY
- sends scripted commands
- captures clean textual snapshots
- writes one report file

## Default scripted run

The default command script is:

```text
n,v,SPACE,n,w,n,QUIT
```

Meaning:

- `n` = one tick
- `v` = change render mode
- `SPACE` = toggle auto-run
- `w` = move packet up
- `QUIT` = exit

## What the machine should evaluate

After reading the report and code, answer:

1. Does this behave like a real machine-playable runtime slice?
2. What is already strong here as a separate machine-facing entity?
3. What still depends on local Linux assumptions?
4. What should the next layer above this runtime be?

## Short summary

If you only remember one thing:

> Use `packet_machine/packet_machine_render.py`, not raw `packet_cli`.
