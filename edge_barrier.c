#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/XInput2.h>

#define default_threshold  150      // the total path of the mouse against the barrier, px
#define default_reset_ms   300      // pause between strokes, after which the force is reset
#define default_direction  "both"   // both | right | left

static void print_help(FILE *out, const char *prog) {
    fprintf(out,
        "usage: %s [options] x y1 y2\n"
        "\n"
        "Sticky pointer barrier between monitors (X11, XFixes + XInput 2.3).\n"
        "\n"
        "arguments:\n"
        "  x          x coordinate of the border between monitors\n"
        "  y1, y2     shared vertical span of the border\n"
        "\n"
        "options:\n"
        "  -t px      total distance in pixels the mouse must be pushed\n"
        "             against the barrier to cross it (default: %d)\n"
        "  -p ms      pause between hits; after it the accumulated force\n"
        "             is reset (default: %d)\n"
        "  -d dir     both | right | left, direction the barrier resists\n"
        "             (default: %s)\n"
        "  -h         show this help\n"
        "\n"
        "example:\n"
        "  %s -t 200 -d right -p 500 1920 0 1080\n",
        prog, default_threshold, default_reset_ms, default_direction, prog);
}

// directions in XFixes
static int direction_mask(const char *name) {
    if (!strcmp(name, "both"))  return 0;
    if (!strcmp(name, "right")) return BarrierNegativeX;  // right only
    if (!strcmp(name, "left"))  return BarrierPositiveX;  // left only
    return -1;
}

int main(int argc, char **argv) {
    double threshold = default_threshold;
    int reset_ms = default_reset_ms;
    const char *dir_name = default_direction;
    int opt;

    while ((opt = getopt(argc, argv, "ht:p:d:")) != -1) {
        if (opt == 'h') {
            print_help(stdout, argv[0]);
            return 0;
        }
        else if (opt == 't') threshold = atof(optarg);
        else if (opt == 'p') reset_ms = atoi(optarg);
        else if (opt == 'd') dir_name = optarg;
        else return 1;
    }
    if (argc - optind < 3) {
        print_help(stderr, argv[0]);
        return 1;
    }

    int x = atoi(argv[optind]);
    int y1 = atoi(argv[optind + 1]);
    int y2 = atoi(argv[optind + 2]);
    int directions = direction_mask(dir_name);
    if (directions < 0) {
        fprintf(stderr, "unknown direction: %s\n", dir_name);
        return 1;
    }

    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "cannot open display\n");
        return 2;
    }
    Window root = DefaultRootWindow(dpy);

    int xi_opcode, ev, err;
    XQueryExtension(dpy, "XInputExtension", &xi_opcode, &ev, &err);
    int major = 2, minor = 3;
    XIQueryVersion(dpy, &major, &minor);

    unsigned char bits[XIMaskLen(XI_LASTEVENT)] = {0};
    XIEventMask mask = { XIAllMasterDevices, sizeof(bits), bits };
    XISetMask(bits, XI_BarrierHit);
    XISelectEvents(dpy, root, &mask, 1);

    XFixesCreatePointerBarrier(dpy, root, x, y1, x, y2, directions, 0, NULL);

    double acc = 0;
    for (;;) {
        XEvent e;
        XNextEvent(dpy, &e);
        if (e.xcookie.type != GenericEvent || e.xcookie.extension != xi_opcode
            || !XGetEventData(dpy, &e.xcookie)) continue;

        if (e.xcookie.evtype == XI_BarrierHit) {
            XIBarrierEvent *h = e.xcookie.data;
            if (h->dtime > reset_ms) acc = 0;
            acc += fabs(h->dx);
            if (acc >= threshold) {
                XIBarrierReleasePointer(dpy, h->deviceid, h->barrier, h->eventid);
                XFlush(dpy);
                acc = 0;
            }
        }
        XFreeEventData(dpy, &e.xcookie);
    }
}
