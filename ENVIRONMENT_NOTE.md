# Packet Machine Environment Note

## Status

Known issue.
Not a Packet runtime bug.

## Why this file exists

Some external machine environments can misread Packet CLI behavior.

The common false diagnosis looks like:

- startup frame works
- later frames are empty
- process stays alive
- therefore "runtime hangs"

That conclusion is not canonical.

## What is known

On real local Linux, Phase 3 is validated:

- startup snapshot works
- post-startup snapshots work
- repeated `n` advances `frame`
- repeated `n` advances `sweep`

So if another environment reports startup-only behavior,
the first suspect is the environment,
not the Packet runtime.

## Known source of false reports

PTY behavior in sandboxed environments may differ from normal Linux:

- delayed readability after writes
- different `select()` timing
- unusual buffering
- virtualization-specific PTY semantics

This can make the machine render layer appear broken
while the underlying runtime is still correct.

## Canonical rule

Do not report a runtime hang from startup-only failure alone.

First report:

- your execution environment
- whether PTY is virtualized
- whether you are on local Linux or sandboxed Linux

## Short version

If startup works but later snapshots do not,
that may be an environment/PTy issue,
not a Packet runtime issue.
