# edge_barrier

A tiny X11 tool that makes the mouse cursor **resist** crossing from one monitor to another. To get to the next screen you have to push the cursor against the border with some force; casual movements and overshoots stay on the current monitor.

It uses the XFixes **PointerBarrier** on the X server side, so:

- no windows are created, nothing covers scrollbars, window edges or panels;
- no polling: the program sleeps until the cursor actually hits the barrier (about 0% CPU);
- the barrier is removed automatically by the X server when the program exits, so the cursor can never get stuck.

Works with any X11 window manager (developed and tested with Awesome WM). It does **not** work on Wayland.

## How it works

1. A barrier segment is created on the border between two monitors.
2. When the cursor presses against it, the X server sends an `XI_BarrierHit` event (XInput 2.3) with the attempted movement `dx` and the time `dtime` since the previous hit.
3. The program sums up `dx`. If the sum reaches the threshold, it calls `XIBarrierReleasePointer` and the cursor passes through. If you pause longer than the configured pause, the accumulated force is reset.

## Requirements

- X11 with XInput 2.3 or newer
- A C compiler and development headers for `libX11`, `libXfixes`, `libXi`

| Distro | Packages |
|---|---|
| Debian / Ubuntu | `libx11-dev libxfixes-dev libxi-dev` |
| Arch | `libx11 libxfixes libxi` |
| Fedora | `libX11-devel libXfixes-devel libXi-devel` |

## Build

```
gcc edge_barrier.c -o edge_barrier -lX11 -lXfixes -lXi -lm
```

Optionally install it somewhere in your `PATH`:

```
install -m 755 edge_barrier ~/.local/bin/
```

## Usage

```
edge_barrier [options] x y1 y2
```

### Arguments

| Argument | Description |
|---|---|
| `x` | x coordinate of the border between two monitors |
| `y1`, `y2` | shared vertical span of the border (top and bottom) |

### Options

| Option | Description | Default |
|---|---|---|
| `-t px` | Total distance in pixels the mouse must be pushed against the barrier to cross it. Merely resting the cursor on the border does nothing; the mouse has to keep moving towards it. | `150` |
| `-p ms` | Pause between hits. If you stop pushing for longer than this, the accumulated force is reset. | `300` |
| `-d dir` | `both`, `right` or `left`: direction in which the barrier resists. `right` resists moving from the left monitor to the right one, `left` the opposite. | `both` |
| `-h` | Show help | |

Defaults live in the header of `edge_barrier.c` (`default_threshold`, `default_reset_ms`, `default_direction`).

### Examples

```
edge_barrier 1920 0 1080
edge_barrier -t 200 1920 0 1080
edge_barrier -t 200 -p 500 1920 0 1080
edge_barrier -d right 1920 0 1080
```

## Finding the coordinates for your monitors

Run:

```
xrandr --listmonitors
```

Example output:

```
Monitors: 2
 0: +*DVI-I-0 1920/520x1080/290+0+0  DVI-I-0
 1: +DP-4 1920/600x1080/330+1920+0  DP-4
```

Each line has the form `width/mm x height/mm + x_offset + y_offset`. For two monitors side by side:

- `x` = right edge of the left monitor = `x_offset + width` (here `0 + 1920 = 1920`)
- `y1` = the lower of the two top edges = `max(y_offset_1, y_offset_2)`
- `y2` = the higher of the two bottom edges = `min(y_offset_1 + height_1, y_offset_2 + height_2)`

For the monitors above: `edge_barrier 1920 0 1080`.

If the monitors have different sizes or offsets, the barrier only covers the part where they actually touch.

## Multiple monitors

Run one instance per border. Each has its own force counter and its own settings:

```
edge_barrier -t 100 1920 0 1080
edge_barrier -t 250 -p 500 3840 0 1080
edge_barrier -d right 5760 0 1080
```

## Autostart in Awesome WM

In `rc.lua`:

```lua
awful.spawn.once("edge_barrier -t 150 1920 0 1080")
```

`awful.spawn.once` does not start a second copy when Awesome restarts. After changing arguments, run `pkill edge_barrier` and restart Awesome.

Other window managers or display managers: just start the command from `~/.xinitrc`, `~/.xprofile` or your autostart mechanism.

## Tuning

- **Too weak?** Increase `-t` (try 150 to 300).
- **Hard to push through with small hand movements?** Increase `-p` so that short pauses do not reset the force.
- **Annoying in one direction only?** Use `-d right` or `-d left`.

## Limitations

- X11 only.
- Only vertical barriers are supported (monitors arranged horizontally). For monitors stacked vertically, the barrier would need to be horizontal, which requires small changes in the `XFixesCreatePointerBarrier` call.
- Dragging windows with the mouse across the border is also subject to the barrier.
- If the program is started without `DISPLAY` set, it exits with code 2.

## Exit codes

| Code | Meaning |
|---|---|
| `0` | Help shown |
| `1` | Wrong arguments or unknown direction |
| `2` | Cannot open X display |

## License

[MIT](LICENSE)
