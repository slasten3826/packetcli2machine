#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "packet/core.h"
#include "packet/render_cli.h"

static struct termios g_cli_termios_saved;
static int g_cli_termios_valid = 0;

typedef struct {
    uint32_t app_mask;
    int quit_requested;
    int auto_run_toggled;
    int step_once;
    int cycle_mode;
} PaCliInput;

static void pa_cli_restore_terminal(void) {
    if (g_cli_termios_valid) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_cli_termios_saved);
        g_cli_termios_valid = 0;
    }
}

static int pa_cli_setup_terminal(void) {
    struct termios raw;

    if (tcgetattr(STDIN_FILENO, &g_cli_termios_saved) != 0) {
        return -1;
    }

    g_cli_termios_valid = 1;
    raw = g_cli_termios_saved;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0) {
        g_cli_termios_valid = 0;
        return -1;
    }

    atexit(pa_cli_restore_terminal);
    return 0;
}

static PaCliInput pa_cli_poll_input(void) {
    PaCliInput input = {0u, 0, 0, 0, 0};
    fd_set readfds;
    struct timeval timeout = {0, 0};
    char ch;
    ssize_t nread;

    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    if (select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout) <= 0) {
        return input;
    }

    for (;;) {
        nread = read(STDIN_FILENO, &ch, 1);
        if (nread <= 0) {
            if (nread < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                break;
            }
            break;
        }

        switch (ch) {
            case 'w': case 'W': input.app_mask |= PA_INPUT_UP; break;
            case 's': case 'S': input.app_mask |= PA_INPUT_DOWN; break;
            case 'a': case 'A': input.app_mask |= PA_INPUT_LEFT; break;
            case 'd': case 'D': input.app_mask |= PA_INPUT_RIGHT; break;
            case 'm': case 'M': input.app_mask |= PA_INPUT_MAP_TOGGLE; break;
            case 'r': case 'R': input.app_mask |= PA_INPUT_REGEN; break;
            case 'f': case 'F': input.app_mask |= PA_INPUT_SW_TOGGLE; break;
            case 'g': case 'G': input.app_mask |= PA_INPUT_SW_STEP; break;
            case '[': input.app_mask |= PA_INPUT_SW_SLOWER; break;
            case ']': input.app_mask |= PA_INPUT_SW_FASTER; break;
            case '-': case '_': input.app_mask |= PA_INPUT_PARAM_DEC; break;
            case '=': case '+': input.app_mask |= PA_INPUT_PARAM_INC; break;
            case ' ': input.auto_run_toggled = 1; break;
            case 'n': case 'N': input.step_once = 1; break;
            case 'v': case 'V': input.cycle_mode = 1; break;
            case 'x': case 'X':
            case 'q': case 'Q':
                input.quit_requested = 1;
                break;
            default:
                break;
        }
    }

    return input;
}

static void pa_cli_sleep_ms(long ms) {
    struct timespec req;

    req.tv_sec = ms / 1000L;
    req.tv_nsec = (ms % 1000L) * 1000000L;
    nanosleep(&req, NULL);
}

int main(void) {
    PaApp app;
    PaCliRenderMode mode = PA_CLI_MODE_COMPOSITE;
    int auto_run = 0;
    long sleep_ms = 120L;

    pa_app_init(&app);

    if (pa_cli_setup_terminal() != 0) {
        fprintf(stderr, "cli terminal setup failed\n");
        return 1;
    }

    for (;;) {
        PaCliInput input;

        pa_cli_render_frame(stdout, &app, mode, auto_run);
        fprintf(stdout, "controls: WASD move, -= field, m map, r regen, f toggle SW, g step SW, [ ] speed\n");
        fprintf(stdout, "dev: space auto-run, n single tick, v cycle view, q/x quit\n");

        input = pa_cli_poll_input();
        if (input.quit_requested) {
            break;
        }

        if (input.auto_run_toggled) {
            auto_run = !auto_run;
        }
        if (input.cycle_mode) {
            mode = (PaCliRenderMode)(((int)mode + 1) % 3);
        }

        if (auto_run || input.step_once || input.app_mask != 0u) {
            pa_app_set_input(&app, input.app_mask);
            pa_app_update(&app);
        } else {
            pa_app_set_input(&app, 0u);
        }

        if (app.quit_requested) {
            break;
        }
        pa_cli_sleep_ms(sleep_ms);
    }

    return 0;
}
