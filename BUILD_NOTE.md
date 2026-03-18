# packetcli2machine Build Note

This bundle is a machine-facing Packet CLI kit.

Important:

- it is not the full repository
- it is not intended to rebuild the whole project
- it is intended to rebuild the Packet CLI slice contained here
- it is intended to show the machine render layer and a real sample report

Included for context:

- `core.h`
- `render_cli.h`
- `runtime_internal.h`
- `render_cli.c`
- `main.c`
- `packet_machine/packet_machine_render.py`
- `packet_machine_render_layer.md`
- `packet_machine_render_report.txt`

Why `runtime_internal.h` is included:

- `render_cli.c` currently still depends on Packet Adventure runtime internals
- this reflects the current extraction stage honestly

So this bundle should be read as:

- a buildable Packet CLI runtime slice
- plus enough context to understand the current coupling points

Not as:

- the whole Packet project
