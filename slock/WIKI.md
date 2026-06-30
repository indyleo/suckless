# WIKI — slock Configuration Reference

All config lives in `config.h`. Rebuild with `make clean install` after any
change. slock must be reinstalled setuid root after every rebuild.

## Privilege drop

```c
static const char *user  = "nobody";
static const char *group = "nobody";
```

User and group slock drops to after locking the display. `nobody:nobody` is
the safe default — this user has no home directory and no special privileges.

## Lock state colors

```c
static const char *colorname[NUMCOLS] = {
    [BACKGROUND] = "#ebdbb2",  /* screen color before blur is applied */
    [INIT]       = "#282828",  /* idle — waiting for input */
    [INPUT]      = "#d79921",  /* typing — yellow */
    [FAILED]     = "#cc241d",  /* wrong password — red */
};
```

The entire lock window background changes to the current state's color.
`BACKGROUND` is only briefly visible before the blurred screenshot is
composited on top.

## Auth behavior

```c
static const int failonclear = 1;
```

`1` = pressing Escape or Ctrl+C (which clears the input buffer) triggers
the `FAILED` color state, same as a wrong password. Prevents an attacker
from just clearing the field and trying to look like they didn't touch it.
Set to `0` to allow clearing without the red flash.

## Blur

```c
#define BLUR
static const int blurRadius = 5;
```

Remove or comment out `#define BLUR` to disable blurring entirely (lock
window will just show the state color). Increase `blurRadius` for a stronger
blur — values above 10–15 start to look uniform rather than blurred and cost
more CPU at lock time (the capture + blur happens once synchronously before
the window appears).

## Pixelation (alternative to blur)

```c
// #define PIXELATION
static const int pixelSize = 0;
```

Uncomment `#define PIXELATION` and disable `#define BLUR` to use pixelation
instead. `pixelSize` controls the block size — higher = more pixelated.
`0` with `PIXELATION` defined effectively disables it.

## Logo

```c
static const int logosize = 75;  /* scale factor for rectangle grid */
static const int logow    = 12;  /* grid width  — used for right-center alignment */
static const int logoh    = 6;   /* grid height — used for right-center alignment */

static XRectangle rectangles[9] = {
    {0, 3, 1, 3}, {1, 3, 2, 1}, {0, 5, 8, 1}, {3, 0, 1, 5},
    {5, 3, 1, 2}, {7, 3, 1, 2}, {8, 3, 4, 1}, {9, 4, 1, 2}, {11, 4, 1, 2},
};
```

Each `XRectangle` is `{x, y, width, height}` in grid units. All values are
multiplied by `logosize` at draw time. `logow`/`logoh` must accurately
describe the bounding box of your rectangle set for the right-center
positioning math to work — if you change the logo, update these too.

To disable the logo, set `rectangles` to an empty array and `logosize` to 0.
