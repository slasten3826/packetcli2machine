CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -pedantic -D_POSIX_C_SOURCE=200809L -Iinclude -Ipacket/include

TARGET = packet_cli
OBJS = \
	platform/cli/main.o \
	src/runtime/app.o \
	src/world_runtime/world_runtime.o \
	src/manifestation_runtime/manifestation_runtime.o \
	src/packet_runtime/packet_runtime.o \
	src/ui_runtime/ui_render.o \
	src/rendering/render_primitives.o \
	src/rendering/font_renderer.o \
	packet/src/render_cli.o

.DEFAULT_GOAL := $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@

platform/cli/main.o: platform/cli/main.c packet/include/packet/core.h packet/include/packet/render_cli.h
	$(CC) $(CFLAGS) -c $< -o $@

src/runtime/app.o: src/runtime/app.c include/packet_adventure/app.h include/packet_adventure/render_target.h include/packet_adventure/render_primitives.h include/packet_adventure/font_renderer.h include/packet_adventure/runtime_internal.h
	$(CC) $(CFLAGS) -c $< -o $@

src/world_runtime/world_runtime.o: src/world_runtime/world_runtime.c include/packet_adventure/runtime_internal.h include/packet_adventure/app.h
	$(CC) $(CFLAGS) -c $< -o $@

src/manifestation_runtime/manifestation_runtime.o: src/manifestation_runtime/manifestation_runtime.c include/packet_adventure/runtime_internal.h include/packet_adventure/app.h
	$(CC) $(CFLAGS) -c $< -o $@

src/packet_runtime/packet_runtime.o: src/packet_runtime/packet_runtime.c include/packet_adventure/runtime_internal.h include/packet_adventure/app.h
	$(CC) $(CFLAGS) -c $< -o $@

src/ui_runtime/ui_render.o: src/ui_runtime/ui_render.c include/packet_adventure/ui_render.h include/packet_adventure/runtime_internal.h include/packet_adventure/app.h
	$(CC) $(CFLAGS) -c $< -o $@

src/rendering/render_primitives.o: src/rendering/render_primitives.c include/packet_adventure/render_target.h include/packet_adventure/render_primitives.h
	$(CC) $(CFLAGS) -c $< -o $@

src/rendering/font_renderer.o: src/rendering/font_renderer.c include/packet_adventure/render_target.h include/packet_adventure/render_primitives.h include/packet_adventure/font_renderer.h
	$(CC) $(CFLAGS) -c $< -o $@

packet/src/render_cli.o: packet/src/render_cli.c packet/include/packet/render_cli.h packet/include/packet/core.h include/packet_adventure/runtime_internal.h
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -f $(TARGET) $(OBJS)
