# DOCS — slock Code Layout & Internals

## File overview

| File | Purpose |
|---|---|
| `slock.c` | Everything: locking, auth, drawing, blur, logo |
| `explicit_bzero.c` | Fallback `explicit_bzero()` for systems that lack it |
| `util.h` | Single `die()` macro |
| `config.h` | Colors, blur settings, logo geometry — compiled in |
| `config.def.h` | Upstream default config — do not edit |
| `config.mk` | Build flags, setuid install |

## Program flow

```
main()
  ├─ validate running as root (setuid)
  ├─ getpwnam() — look up user entry for password hash
  ├─ XOpenDisplay / get screen list (Xinerama)
  ├─ lockscreen() for each screen
  │    ├─ XGrabPointer + XGrabKeyboard (blocks all input)
  │    ├─ capture root window content (for blur)
  │    ├─ apply Gaussian blur to captured pixmap
  │    └─ XMapRaised the lock window over everything
  ├─ drop privileges (setuid → nobody:nobody)
  └─ readpw() — event loop
       ├─ KeyPress → append to password buffer
       ├─ Enter → crypt() / shadow compare → unlock or FAILED state
       ├─ Escape / Ctrl+C → clear buffer → INIT state
       └─ on auth success: XUnmapWindow + exit
```

Privilege drop happens *after* locking — root is only held long enough to
grab the display and check the password hash. The actual running process
drops to `nobody:nobody` while waiting for input.

## Color states

Three visual states, mapped to `colorname[]`:

| State | Constant | Shown when |
|---|---|---|
| `BACKGROUND` | `#ebdbb2` | Initial screen capture color layer |
| `INIT` | `#282828` | Idle / input cleared |
| `INPUT` | `#d79921` | User is typing |
| `FAILED` | `#cc241d` | Wrong password (or `failonclear` triggered) |

The lock window background is set to the current state's color via
`XSetWindowBackground()` + `XClearWindow()` on each state transition, so
the color change is instant.

## Blur

The blur patch captures the root window content with `XGetImage()` before
mapping the lock window, applies a multi-pass box blur approximation of
Gaussian blur (radius set by `blurRadius`), renders it to a Pixmap, and sets
that as the lock window background before mapping it. The blurred image is
static — it's captured once at lock time and doesn't update.

Pixelation mode (`#define PIXELATION`, disabled) is an alternative that
downscales + upscales the captured image instead of blurring, producing a
retro-pixel effect. Only one of blur or pixelation can be active at a time.

## Logo

The logo is drawn as a grid of filled rectangles defined in `rectangles[]`,
scaled by `logosize`, and positioned right-center on the primary monitor.
`logow` and `logoh` define the grid dimensions used for alignment math.
Color uses `SchemeNorm` foreground.

## Password handling

The typed password is held in a stack-allocated buffer and zeroed with
`explicit_bzero()` after each auth attempt (success or failure) — never
stored in heap memory, never in a string that could outlive the auth check.
