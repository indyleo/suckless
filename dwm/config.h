
/* See LICENSE file for copyright and license details. */

/* appearance */
static const unsigned int borderpx = 1; /* border pixel of windows */
static const unsigned int gappx = 8;    /* gaps between windows */
static const unsigned int snap = 16;    /* snap pixel */
static const int swallowfloating =
    0;                        /* 1 means swallow floating windows by default */
static const int showbar = 1; /* 0 means no bar */
static const int topbar = 1;  /* 0 means bottom bar */
const char *wallpaperdir = "~/Pictures/Wallpapers/gruvbox";
static const int wallpaperinterval = 900; /* seconds, 0 to disable timer */
const char *fifopath = "/tmp/dwm.fifo";
const char *fiforeplypath = "/tmp/dwm.fifo.reply";
const char *fonts[] = {
    "MesloLGS Nerd Font Mono:pixelsize=12",
    "NotoColorEmoji:pixelsize=12:antialias=true:autohint=true"};
const int fontslen = LENGTH(fonts); /* external linkage (dropped `static`
                                      * above) so osd.c can build its own
                                      * font set to match the bar's */
/* Gruvbox color variables */
static const char gruvbox_normfgcolor[] = "#ebdbb2"; // light fg
static const char gruvbox_normbgcolor[] = "#282828"; // dark bg
static const char gruvbox_normbordercolor[] =
    "#3c3836"; // slightly lighter than bg

static const char gruvbox_selfgcolor[] = "#282828"; // dark fg for contrast
static const char gruvbox_selbgcolor[] = "#fabd2f"; // bright yellow
static const char gruvbox_selbordercolor[] =
    "#d79921"; // muted yellow for border

static const char gruvbox_hidfgcolor[] = "#928374";     // gruvbox faded fg
static const char gruvbox_hidbgcolor[] = "#1d2021";     // hard contrast bg
static const char gruvbox_hidbordercolor[] = "#3c3836"; // same as norm border

static const char gruvbox_urgfgcolor[] = "#fbf1c7";     // near-white fg
static const char gruvbox_urgbgcolor[] = "#cc241d";     // gruvbox red
static const char gruvbox_urgbordercolor[] = "#fb4934"; // bright red border

/* Gruvbox color scheme table */
static const char *colors[][3] = {
    /*               fg               bg                border               */
    [SchemeNorm] = {gruvbox_normfgcolor, gruvbox_normbgcolor,
                    gruvbox_normbordercolor}, /* normal */
    [SchemeSel] = {gruvbox_selfgcolor, gruvbox_selbgcolor,
                   gruvbox_selbordercolor}, /* selected */
    [SchemeHid] = {gruvbox_hidfgcolor, gruvbox_hidbgcolor,
                   gruvbox_hidbordercolor}, /* hidden */
    [SchemeUrg] = {gruvbox_urgfgcolor, gruvbox_urgbgcolor,
                   gruvbox_urgbordercolor}, /* urgent */
};

const char *spcmd1[] = {"st", "-c", "termsc,Termsc", NULL};
const char *spcmd2[] = {"st", "-c", "lfsc,Lfsc", "-e", "zsh", "-c", "lf", NULL};
const char *spcmd3[] = {"st", "-c", "qalsc,Qalsc", "-e", "qalc", NULL};
const char *spcmd4[] = {"st", "-c",      "wiremixsc,Wiremixsc",
                        "-e", "wiremix", NULL};
const char *spcmd5[] = {"st",  "-c", "gurks,Gurks", "-e",
                        "zsh", "-c", "gurks",       NULL};
const char *spcmd6[] = {"st",  "-c", "discordo,Discordo", "-e",
                        "zsh", "-c", "discordo",          NULL};
const char *spcmd7[] = {"st", "-c", "twitch-tui,Twitch-tui", "-e", "twt", NULL};
const char *spcmd8[] = {"st",  "-c", "musicsc,Musicsc", "-e",
                        "zsh", "-c", "subsonic-tui",    NULL};
static Sp scratchpads[] = {
    {"termsc", spcmd1},     {"lfsc", spcmd2},    {"qalsc", spcmd3},
    {"wiremixsc", spcmd4},  {"gurks", spcmd5},   {"discordo", spcmd6},
    {"twitch-tui", spcmd7}, {"musicsc", spcmd8},
};

static const char *const autostart[] = {
    "/usr/local/bin/autostart.sh", NULL, NULL /* terminate */
};

/* tagging */
const Tag tags[] = {
    {"󰖟", "web"}, {"󰙯", "chat"}, {"", "dev"},
    {"", "game"}, {"󰨇", "vm"},
};
const int tagslen = LENGTH(tags);

static const Rule rules[] = {
    /* xprop(1):
     *	WM_CLASS(STRING) = instance, class
     *	WM_NAME(STRING) = title
     */
    /* class     instance      title           tags mask  isfloating  isterminal
                     noswallow  monitor  w  h   x   y  setpos  center
       forcefullscreen w/h: 0 = keep the client's requested size x/y: only
       applied when setpos=1; offset from the monitor's work-area origin
       (top-left), same idea as Hyprland's `move` setpos: 1 = place at x,y
       instead of dwm's default centering center: 1 = explicitly center (dwm's
       default anyway; mostly for readability, or to force-center a rule that
       would otherwise not match the defaults) forcefullscreen: 1 = go
       fullscreen immediately on open, like Hyprland's `fullscreen` rule */
    {"Gimp", NULL, NULL, 0, 1, 0, 0, -1},
    {"Firefox", NULL, NULL, 1 << 8, 0, 0, -1, -1},
    {"St", NULL, NULL, 0, 0, 1, 0, -1},
    {"Alacritty", NULL, NULL, 0, 0, 1, 0, -1},
    {"org.wezfurlong.wezterm", NULL, NULL, 0, 0, 1, 0, -1},
    {NULL, NULL, "Event Tester", 0, 0, 0, 1, -1}, /* xev */
    {"termsc", NULL, NULL, SPTAG(0), 1, -1},
    {"lfsc", NULL, NULL, SPTAG(1), 1, -1},
    {"qalsc", NULL, NULL, SPTAG(2), 1, -1},
    {"wiremixsc", NULL, NULL, SPTAG(3), 1, -1},
    {"gurks", NULL, NULL, SPTAG(4), 1, -1},
    {"discord", NULL, NULL, SPTAG(5), 1, -1},
    {"twitch-tui", NULL, NULL, SPTAG(6), 1, -1},
    {"musicsc", NULL, NULL, SPTAG(7), 1, -1},
    {"Dragon", NULL, NULL, 0, 1, -1},

    /* Picture-in-Picture: float, pin to a fixed 480x270 size, and place
       it 14px from the left / 12px from the top of the work area —
       translated from the Hyprland rule you posted (size 480,270 /
       move 14,12). class/instance/title are now full PCRE2 regex, so
       ^...$ anchors work exactly like in Hyprland. Note: dwm has no
       "pin" (stay visible across tag switches) concept, so this only
       covers size + move. */
    {NULL, NULL, "^Picture-in-Picture$", 0, 1, 0, 0, -1, 480, 270, 14, 12, 1,
     0},

    /* Steam games auto-fullscreen for every steam_app_NNNNN window
       except steam_app_0 (the Steam client itself reports class
       steam_app_0 in some launch paths) — translated 1:1 from your
       Hyprland negative-lookahead rule: class ^steam_app_(?!0$)[0-9]+$ */
    {"^steam_app_(?!0$)[0-9]+$", NULL, NULL, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0,
     1},

    /* SC2 / Brood War report class "steam_app_default", so the rule
       above already catches them — these title-matching fallbacks are
       here in case Valve ever changes the class string, same as in
       your Hyprland config. */
    {NULL, NULL, "^StarCraft II$", 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 1},
    {NULL, NULL, "^Brood War$", 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 1},
};

/* layout(s) */
static const float mfact = 0.55; /* factor of master area size [0.05..0.95] */
static const int nmaster = 1;    /* number of clients in master area */
static const int resizehints =
    0; /* 1 means respect size hints in tiled resizals */
static const int attachbelow =
    1; /* 1 means attach after the currently active window */
static const int lockfullscreen =
    1; /* 1 will force focus on the fullscreen window */

static const Layout layouts[] = {
    /* symbol   name       arrange function */
    {"", "tile", tile},  /* first entry is default */
    {"", "float", NULL}, /* no layout function means floating behavior */
    {"󰊓", "monocle", monocle},
};

/* key definitions */
#include <X11/XF86keysym.h>
#define MODKEY Mod4Mask
#define ALTKEY Mod1Mask
#define CTRLKEY ControlMask
#define SHIFTKEY ShiftMask
#define TAGKEYS(KEY, TAG)                                                      \
  {MODKEY, KEY, view, {.ui = 1 << TAG}},                                       \
      {MODKEY | CTRLKEY, KEY, toggleview, {.ui = 1 << TAG}},                   \
      {MODKEY | SHIFTKEY, KEY, tag, {.ui = 1 << TAG}},                         \
      {MODKEY | CTRLKEY | SHIFTKEY, KEY, toggletag, {.ui = 1 << TAG}},

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd)                                                             \
  {                                                                            \
    .v = (const char *[]) { "/bin/sh", "-c", cmd, NULL }                       \
  }

/* status bar blocks -- replaces dwmblocks. Each block is a shell command;
 * interval is in seconds (0 = only updates on click, or when something
 * calls statusbar_refresh(), e.g. the OSD block below). BLOCK_BUTTON is
 * set in the environment on click (1-5), same as dwmblocks. Adjust the
 * commands below to match your own `sysctl`/`mediactl` tooling -- these
 * assume a `--status`/`--get` style query flag exists; swap in
 * amixer/brightnessctl/whatever you actually have if not. */
const StatusBlock statusblocks[] = {
    /* icon  cmd                                interval(s) */
    {"", "sysctl vol --status", 0},   /* refreshed by the OSD, see below */
    {"", "sysctl bri --status", 0},   /* refreshed by the OSD, see below */
    {"", "mediactl --source song --status", 5},
    {"", "sysctl bat --status", 30},
    {"", "sysctl wifi --status", 20},
    {"", "sysctl bt --status", 20},
    {"", "date '+%a %d %b  %H:%M'", 15},
};
const int statusblockslen = LENGTH(statusblocks);

/* on-screen-display popups for volume/brightness/mic. changecmd runs
 * first, getcmd is then read back for the level bar (stdout parsed as an
 * int 0-100); blockidx points at the matching statusblocks[] entry above
 * so the bar updates immediately instead of waiting out its interval.
 * Order here defines the OsdTrig indices used in keys[] below. */
static const char *volupcmd[] = {"sysctl", "vol", "-i", "5", NULL};
static const char *voldowncmd[] = {"sysctl", "vol", "-d", "5", NULL};
static const char *voltogglecmd[] = {"sysctl", "vol", "--toggle", NULL};
static const char *volgetcmd[] = {"sysctl", "vol", "--get", NULL};
static const char *briupcmd[] = {"sysctl", "bri", "-i", "5", NULL};
static const char *bridowncmd[] = {"sysctl", "bri", "-d", "5", NULL};
static const char *brigetcmd[] = {"sysctl", "bri", "--get", NULL};
static const char *micupcmd[] = {"sysctl", "mic", "-i", "5", NULL};
static const char *micdowncmd[] = {"sysctl", "mic", "-d", "5", NULL};
static const char *mictogglecmd[] = {"sysctl", "mic", "--toggle", NULL};
static const char *micgetcmd[] = {"sysctl", "mic", "--get", NULL};

enum {
  OsdVolUp,
  OsdVolDown,
  OsdVolToggle,
  OsdBriUp,
  OsdBriDown,
  OsdMicUp,
  OsdMicDown,
  OsdMicToggle
}; /* indices into osds[], referenced from keys[] as {.i = OsdVolUp} etc. */

const OsdItem osds[] = {
    /* label  changecmd       getcmd      statusblocks[] index (-1 = none) */
    {"VOL", volupcmd, volgetcmd, 0},
    {"VOL", voldowncmd, volgetcmd, 0},
    {"VOL", voltogglecmd, volgetcmd, 0},
    {"BRI", briupcmd, brigetcmd, 1},
    {"BRI", bridowncmd, brigetcmd, 1},
    {"MIC", micupcmd, micgetcmd, -1},
    {"MIC", micdowncmd, micgetcmd, -1},
    {"MIC", mictogglecmd, micgetcmd, -1},
};
const int osdslen = LENGTH(osds);

/* commands */
static char dmenumon[2] =
    "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *dmenucmd[] = {"dmenu_run", NULL};

static const Key keys[] = {
    /* modifier                     key        function        argument */

    /* DWM Controls */
    {MODKEY | SHIFTKEY, XK_b, togglebar, {0}},
    {MODKEY, XK_j, focusstackvis, {.i = +1}},
    {MODKEY, XK_k, focusstackvis, {.i = -1}},
    {MODKEY | SHIFTKEY, XK_j, focusstackhid, {.i = +1}},
    {MODKEY | SHIFTKEY, XK_k, focusstackhid, {.i = -1}},
    {MODKEY, XK_space, switchcol, {0}},
    {MODKEY, XK_h, setmfact, {.f = -0.05}},
    {MODKEY, XK_l, setmfact, {.f = +0.05}},
    {MODKEY | ALTKEY, XK_j, movestack, {.i = +1}},
    {MODKEY | ALTKEY, XK_k, movestack, {.i = -1}},
    {MODKEY | SHIFTKEY, XK_equal, incnmaster, {.i = +1}},
    {MODKEY | SHIFTKEY, XK_minus, incnmaster, {.i = -1}},
    {MODKEY | SHIFTKEY, XK_z, zoom, {0}},
    {MODKEY, XK_Tab, view, {0}},
    {MODKEY | ALTKEY | SHIFTKEY, XK_t, setlayout, {.v = &layouts[0]}},
    {MODKEY | ALTKEY | SHIFTKEY, XK_f, setlayout, {.v = &layouts[1]}},
    {MODKEY | ALTKEY | SHIFTKEY, XK_m, setlayout, {.v = &layouts[2]}},
    {MODKEY | ALTKEY, XK_space, cyclelayout, {.i = +1}},
    {MODKEY | ALTKEY | SHIFTKEY, XK_space, cyclelayout, {.i = -1}},
    {MODKEY | SHIFTKEY, XK_f, fullscreen, {0}},
    {MODKEY | SHIFTKEY, XK_space, togglefloating, {0}},
    {MODKEY, XK_comma, focusmon, {.i = -1}},
    {MODKEY, XK_period, focusmon, {.i = +1}},
    {MODKEY | SHIFTKEY, XK_comma, tagmon, {.i = -1}},
    {MODKEY | SHIFTKEY, XK_period, tagmon, {.i = +1}},
    {MODKEY, XK_equal, show, {0}},
    {MODKEY | SHIFTKEY, XK_equal, showall, {0}},
    {MODKEY, XK_minus, hide, {0}},

    /* System Controls */
    {MODKEY | SHIFTKEY, XK_q, quit, {0}},
    {MODKEY | SHIFTKEY, XK_r, quit, {1}},
    {MODKEY, XK_q, killclient, {0}},

    /* Tags */
    TAGKEYS(XK_1, 0) TAGKEYS(XK_2, 1) TAGKEYS(XK_3, 2) TAGKEYS(XK_4, 3)
        TAGKEYS(XK_5, 4)

    /* Wallpaper */
    {MODKEY | SHIFTKEY, XK_w, nextwallpaper, {0}},

    /* Compositor */
    {MODKEY, XK_p, spawn, SHCMD("picom_toggle")},

    /* Applications */
    {MODKEY, XK_Return, spawn, SHCMD("st")},
    {MODKEY, XK_f, spawn, SHCMD("thunar")},
    {MODKEY, XK_b, spawn, SHCMD("librewolf")},
    {MODKEY | SHIFTKEY, XK_d, spawn, SHCMD("vesktop")},
    {MODKEY | SHIFTKEY, XK_g, spawn, SHCMD("signal-desktop")},

    /* Launchers */
    {MODKEY, XK_r, spawn, SHCMD("dmenu_run")},
    {MODKEY, XK_w, spawn, SHCMD("wikibook")},
    {MODKEY, XK_n, spawn, SHCMD("notebook")},
    {MODKEY | SHIFTKEY, XK_c, spawn, SHCMD("clip select")},
    {MODKEY | SHIFTKEY, XK_e, spawn, SHCMD("emoji")},
    {MODKEY | ALTKEY, XK_e, spawn, SHCMD("nerdfont")},
    {MODKEY | SHIFTKEY, XK_p, spawn, SHCMD("power")},
    {MODKEY | ALTKEY, XK_r, spawn, SHCMD("recorder")},

    /* System */
    {MODKEY | SHIFTKEY, XK_l, spawn, SHCMD("slock")},
    {0, XF86XK_MonBrightnessUp, osdtrigger, {.i = OsdBriUp}},
    {0, XF86XK_MonBrightnessDown, osdtrigger, {.i = OsdBriDown}},
    {0, XF86XK_WLAN, spawn, SHCMD("sysctl wifi --toggle")},
    {0, XF86XK_Bluetooth, spawn, SHCMD("sysctl bt --toggle")},

    /* Volume */
    {MODKEY | ALTKEY, XK_Up, osdtrigger, {.i = OsdVolUp}},
    {MODKEY | ALTKEY, XK_Down, osdtrigger, {.i = OsdVolDown}},
    {MODKEY | ALTKEY, XK_m, osdtrigger, {.i = OsdVolToggle}},
    {ALTKEY, XF86XK_AudioRaiseVolume, osdtrigger, {.i = OsdVolUp}},
    {ALTKEY, XF86XK_AudioLowerVolume, osdtrigger, {.i = OsdVolDown}},
    {ALTKEY, XF86XK_AudioMute, osdtrigger, {.i = OsdVolToggle}},

    /* Microphone */
    {MODKEY | SHIFTKEY, XK_Up, osdtrigger, {.i = OsdMicUp}},
    {MODKEY | SHIFTKEY, XK_Down, osdtrigger, {.i = OsdMicDown}},
    {MODKEY | SHIFTKEY, XK_m, osdtrigger, {.i = OsdMicToggle}},
    {SHIFTKEY, XF86XK_AudioRaiseVolume, osdtrigger, {.i = OsdMicUp}},
    {SHIFTKEY, XF86XK_AudioLowerVolume, osdtrigger, {.i = OsdMicDown}},
    {0, XF86XK_AudioMicMute, osdtrigger, {.i = OsdMicToggle}},

    /* Media - Song */
    {MODKEY, XK_Right, spawn, SHCMD("mediactl --source song next")},
    {MODKEY, XK_Left, spawn, SHCMD("mediactl --source song previous")},
    {MODKEY, XK_s, spawn, SHCMD("mediactl --source song play-pause")},
    {MODKEY | SHIFTKEY, XK_Right, spawn,
     SHCMD("mediactl --source song skip 10")},
    {MODKEY | SHIFTKEY, XK_Left, spawn,
     SHCMD("mediactl --source song back 10")},
    {0, XF86XK_AudioNext, spawn, SHCMD("mediactl --source song next")},
    {0, XF86XK_AudioPrev, spawn, SHCMD("mediactl --source song previous")},
    {0, XF86XK_AudioPlay, spawn, SHCMD("mediactl --source song play-pause")},

    /* Media - Browser */
    {MODKEY | ALTKEY, XK_Right, spawn, SHCMD("mediactl --source browser next")},
    {MODKEY | ALTKEY, XK_Left, spawn,
     SHCMD("mediactl --source browser previous")},
    {MODKEY | ALTKEY, XK_s, spawn,
     SHCMD("mediactl --source browser play-pause")},
    {MODKEY | ALTKEY | SHIFTKEY, XK_Right, spawn,
     SHCMD("mediactl --source browser skip 10")},
    {MODKEY | ALTKEY | SHIFTKEY, XK_Left, spawn,
     SHCMD("mediactl --source browser back 10")},
    {ALTKEY, XF86XK_AudioNext, spawn, SHCMD("mediactl --source browser next")},
    {ALTKEY, XF86XK_AudioPrev, spawn,
     SHCMD("mediactl --source browser previous")},
    {ALTKEY, XF86XK_AudioPlay, spawn,
     SHCMD("mediactl --source browser play-pause")},

    /* Screenshots */
    {0, XK_Print, takescreenshot, {.i = ShotSelect}},
    {MODKEY, XK_Print, takescreenshot, {.i = ShotScreen}},
    {MODKEY | SHIFTKEY, XK_Print, takescreenshot, {.i = ShotFull}},
    {MODKEY | CTRLKEY, XK_Print, takescreenshot, {.i = ShotWindow}},
    {MODKEY | ALTKEY, XK_Print, pickcolor, {0}},

    /* Scratch Pads */
    {MODKEY, XK_t, togglescratch, {.ui = 0}}, /* termsc */
    {MODKEY, XK_y, togglescratch, {.ui = 1}}, /* lfsc */
    {MODKEY, XK_z, togglescratch, {.ui = 2}}, /* qalsc */
    {MODKEY, XK_a, togglescratch, {.ui = 3}}, /* wiremixsc */
    {MODKEY, XK_g, togglescratch, {.ui = 4}}, /* gurks */
    {MODKEY, XK_d, togglescratch, {.ui = 5}}, /* discordo */
    {MODKEY, XK_c, togglescratch, {.ui = 6}}, /* twitch-tui */
    {MODKEY, XK_m, togglescratch, {.ui = 7}}, /* musicsc */
    {MODKEY | SHIFTKEY,
     XK_t,
     hideallscratchpads,
     {0}}, /* hide all scratchpads */
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle,
 * ClkClientWin, or ClkRootWin */
static const Button buttons[] = {
    /* click                event mask      button          function argument */
    {ClkLtSymbol, 0, Button1, setlayout, {0}},
    {ClkLtSymbol, 0, Button3, setlayout, {.v = &layouts[2]}},
    {ClkWinTitle, 0, Button1, togglewin, {0}},
    {ClkWinTitle, 0, Button2, zoom, {0}},
    {ClkStatusText, 0, Button1, sigstatusbar, {.i = 1}},
    {ClkStatusText, 0, Button2, sigstatusbar, {.i = 2}},
    {ClkStatusText, 0, Button3, sigstatusbar, {.i = 3}},
    {ClkStatusText, 0, Button4, sigstatusbar, {.i = 4}},
    {ClkStatusText, 0, Button5, sigstatusbar, {.i = 5}},
    {ClkClientWin, MODKEY, Button1, movemouse, {0}},
    {ClkClientWin, MODKEY, Button2, togglefloating, {0}},
    {ClkClientWin, MODKEY, Button3, resizemouse, {0}},
    {ClkTagBar, 0, Button1, view, {0}},
    {ClkTagBar, 0, Button3, toggleview, {0}},
    {ClkTagBar, MODKEY, Button1, tag, {0}},
    {ClkTagBar, MODKEY, Button2, toggletag, {0}},
};
