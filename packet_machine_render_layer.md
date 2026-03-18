# Packet Machine Render Layer

## Status

Working hypothesis.
First implementation pass follows immediately after this document.

## Why this exists

`packet_cli` already gave us a second manifestation of `Packet`:

- not PSP
- not SDL
- not visual presentation for players

It gave us a text manifestation of the substrate.

Then `packet_cli_harness.py` added one more step:

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

## Current first proof

Current proof path already exists:

- `packet_cli`
- `packet/src/render_cli.c`
- `platform/cli/main.c`
- `scripts/packet_cli_harness.py`

This is enough to prove the basic claim:

> Another machine can already observe Packet indirectly through a scripted textual manifestation.

## Next extraction step

We now want to make this explicit.

Not:

- random script in `scripts/`

But:

- an explicit machine-facing layer
- with its own README
- with its own entrypoint
- with its own purpose

## Expected first shape

First shape should stay minimal:

- `packet_machine/README.md`
- `packet_machine/packet_machine_render.py`

This layer can start as a thin wrapper over existing CLI manifestation.

That is acceptable.

The important step is not maximal power.
The important step is architectural recognition:

> machine-facing manifestation is now a real project entity

## Verification

This extraction pass is successful when:

- the new machine render entrypoint works
- it can generate a report from a real `packet_cli` run
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
