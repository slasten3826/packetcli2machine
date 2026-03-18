# Packet Machine Render Layer

## Status

Implemented.
Validated on real Linux.

## Why this exists

`packet_cli` already gave us a second manifestation of `Packet`:

- not PSP
- not SDL
- not visual presentation for players

It gave us a text manifestation of the substrate.

Then the old harness step added one more layer:

- not human live play
- but scripted observation
- and snapshot capture for another machine

At that point this stopped being "just a helper script".

It became a new kind of render.

## Core claim

There is a new manifestation layer in the project:

- human render
- machine render

Human render exists to be read by a person.
Machine render exists to be read by another runtime, agent, or LLM.

This is still render, because it does not define the world law itself.
It defines how world law becomes observable.

## What machine render should do

Machine render should:

- launch a real Packet runtime
- drive it through repeatable scripted input
- capture stable state snapshots
- expose enough state for another machine to reason about behavior
- stay close to the real runtime, not a fake stub

## What machine render should not do

Machine render should not:

- invent a fake Packet world
- replace the real runtime with a toy simulation
- become a separate gameplay fork
- duplicate the whole engine in Python

The point is not "machine-friendly fake Packet".
The point is "machine-friendly access to real Packet".

## Current proof

Current proof path already exists:

- `packet_cli`
- `packet/src/render_cli.c`
- `platform/cli/main.c`
- `scripts/packet_cli_harness.py`

This is enough to prove the basic claim:

> Another machine can already observe and drive Packet through a scripted textual manifestation.

## Extraction result

That extraction is now explicit:

- `packet_machine/README.md`
- `packet_machine/packet_machine_render.py`
- `PHASE3_FINAL_STATUS.md`

The important recognition already happened:

> machine-facing manifestation is now a real project entity

## Verification

This extraction pass is successful when:

- the machine render entrypoint works
- it generates reports from real `packet_cli` runs
- post-startup snapshots are populated
- repeated scripted ticks advance `frame`
- the layer can be pointed to as a distinct entity in the repo

## Long-term implication

If this path holds,
then other machines do not need to only read code and docs.

They can:

- read docs
- inspect code
- run machine render
- observe Packet behavior directly

That would make multi-machine development much more practical.
