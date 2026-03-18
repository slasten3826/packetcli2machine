# packetcli2machine Build Note

This bundle is a machine-facing Packet CLI kit.

Important:

- it is not the full repository
- it is not intended to rebuild the whole project
- it is intended to show the Packet CLI layer, the harness, and a real sample report

Included for context:

- `core.h`
- `render_cli.h`
- `runtime_internal.h`
- `render_cli.c`
- `main.c`
- `packet_cli_harness.py`
- `packet_cli_machine_harness.md`
- `packet_cli_harness_report.txt`

Why `runtime_internal.h` is included:

- `render_cli.c` currently still depends on Packet Adventure runtime internals
- this reflects the current extraction stage honestly

So this bundle should be read as:

- evidence of a living Packet CLI manifestation
- plus enough context to understand the current coupling points

Not as:

- a complete standalone rebuild package
