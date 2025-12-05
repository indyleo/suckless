/* See LICENSE file for copyright and license details. */

/* appearance */
static const unsigned int borderpx = 1; /* border pixel of windows */
static const unsigned int gappx = 8;    /* gaps between windows */
static const unsigned int snap = 16;    /* snap pixel */
static const int swallowfloating =
    0;                        /* 1 means swallow floating windows by default */
static const int showbar = 1; /* 0 means no bar */
static const int topbar = 1;  /* 0 means bottom bar */
static const char *fonts[] = {
    "MesloLGS Nerd Font Mono:pixelsize=12",
    "NotoColorEmoji:pixelsize=12:antialias=true:autohint=true"};
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

/* Gruvbox color scheme table */
static const char *colors[][3] = {
    /*               fg               bg                border               */
    [SchemeNorm] = {gruvbox_normfgcolor, gruvbox_normbgcolor,
                    gruvbox_normbordercolor}, /* normal */
    [SchemeSel] = {gruvbox_selfgcolor, gruvbox_selbgcolor,
                   gruvbox_selbordercolor}, /* selected */
    [SchemeHid] = {gruvbox_hidfgcolor, gruvbox_hidbgcolor,
                   gruvbox_hidbordercolor}, /* hidden */
};

typedef struct {
  const char *name;
  const void *cmd;
} Sp;
const char *spcmd1[] = {"alacritty", "--class", "termsc,Termsc", NULL};
const char *spcmd2[] = {"alacritty", "--class", "lfsc,Lfsc", "-e", "lf", NULL};
const char *spcmd3[] = {"alacritty", "--class", "qalsc,Qalsc",
                        "-e",        "qalc",    NULL};
const char *spcmd4[] = {"alacritty", "--class",    "pulsesc,Pulsesc",
                        "-e",        "pulsemixer", NULL};
const char *spcmd5[] = {"alacritty", "--class", "gurks,Gurks",
                        "-e",        "gurks",   NULL};
const char *spcmd6[] = {"alacritty", "--class",  "discordo,Discordo",
                        "-e",        "discodio", NULL};
const char *spcmd7[] = {"alacritty", "--class", "twitch-tui,Twitch-tui",
                        "-e",        "twt",     NULL};
const char *spcmd8[] = {
    "alacritty", "--class",      "subsonic-tui,Subsonic-TUI",
    "-e",        "subsonic-tui", NULL};
const char *spcmd9[] = {"alacritty", "--class",         "cheatsheet,CheatSheet",
                        "-e",        "dwm_keybinds.py", NULL};
static Sp scratchpads[] = {
    /* name          cmd  */
    {"termsc", spcmd1},     {"lfsc", spcmd2},         {"qalsc", spcmd3},
    {"pulsesc", spcmd4},    {"gurks", spcmd5},        {"discordo", spcmd6},
    {"twitch-tui", spcmd7}, {"subsonic-tui", spcmd8}, {"cheatsheet", spcmd9},
};

/* tagging */
static const char *tags[] = {"󰖟", "󰙯", "", "", "󰨇"};

static const Rule rules[] = {
    /* xprop(1):
     *	WM_CLASS(STRING) = instance, class
     *	WM_NAME(STRING) = title
     */
    /* class     instance      title           tags mask  isfloating  isterminal
                     noswallow  monitor */
    {"Gimp", NULL, NULL, 0, 1, 0, 0, -1},
    {"Firefox", NULL, NULL, 1 << 8, 0, 0, -1, -1},
    {"St", NULL, NULL, 0, 0, 1, 0, -1},
    {"Alacritty", NULL, NULL, 0, 0, 1, 0, -1},
    {"org.wezfurlong.wezterm", NULL, NULL, 0, 0, 1, 0, -1},
    {NULL, NULL, "Event Tester", 0, 0, 0, 1, -1}, /* xev */
    {"termsc", NULL, NULL, SPTAG(0), 1, -1},
    {"lfsc", NULL, NULL, SPTAG(1), 1, -1},
    {"qalsc", NULL, NULL, SPTAG(2), 1, -1},
    {"pulsesc", NULL, NULL, SPTAG(3), 1, -1},
    {"gurks", NULL, NULL, SPTAG(4), 1, -1},
    {"discord", NULL, NULL, SPTAG(5), 1, -1},
    {"twitch-tui", NULL, NULL, SPTAG(6), 1, -1},
    {"subsonic-tui", NULL, NULL, SPTAG(7), 1, -1},
    {"cheatsheet", NULL, NULL, SPTAG(8), 1, -1},
    {"Dragon", NULL, NULL, 0, 1, -1},
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
    /* symbol     arrange function */
    {"", tile}, /* first entry is default */
    {"", NULL}, /* no layout function means floating behavior */
    {"󰊓", monocle},
};

/* key definitions */
#include "movestack.c"
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
    {MODKEY, XK_h, setmfact, {.f = -0.05}},
    {MODKEY, XK_l, setmfact, {.f = +0.05}},
    {MODKEY | ALTKEY, XK_j, movestack, {.i = +1}},
    {MODKEY | ALTKEY, XK_k, movestack, {.i = -1}},
    {MODKEY | SHIFTKEY, XK_a, incnmaster, {.i = +1}},
    {MODKEY | SHIFTKEY, XK_m, incnmaster, {.i = -1}},
    {MODKEY | SHIFTKEY, XK_z, zoom, {0}},
    {MODKEY, XK_Tab, view, {0}},
    {MODKEY | ALTKEY | SHIFTKEY, XK_t, setlayout, {.v = &layouts[0]}},
    {MODKEY | ALTKEY | SHIFTKEY, XK_f, setlayout, {.v = &layouts[1]}},
    {MODKEY | ALTKEY | SHIFTKEY, XK_m, setlayout, {.v = &layouts[2]}},
    {MODKEY | SHIFTKEY, XK_f, fullscreen, {0}},
    {MODKEY | SHIFTKEY, XK_space, togglefloating, {0}},
    {MODKEY, XK_0, view, {.ui = ~0}},
    {MODKEY | SHIFTKEY, XK_0, tag, {.ui = ~0}},
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
    {MODKEY | SHIFTKEY, XK_l, spawn, SHCMD("slock")},
    {MODKEY, XK_q, killclient, {0}},

    /* Dmenu */
    {MODKEY, XK_r, spawn, SHCMD("dmenu_run")},
    {ALTKEY, XK_Tab, spawn, SHCMD("dmenu_alttab")},
    {MODKEY, XK_p, spawn, SHCMD("dmenu_flatpak")},
    {MODKEY | SHIFTKEY, XK_c, spawn, SHCMD("clipmgr.py select")},
    {MODKEY | SHIFTKEY, XK_v, spawn, SHCMD("clipmgr.py pin")},
    {MODKEY | SHIFTKEY, XK_e, spawn, SHCMD("dmenu_emoji.py")},
    {MODKEY | ALTKEY, XK_e, spawn, SHCMD("dmenu_nerdfont.py")},
    {MODKEY | SHIFTKEY, XK_p, spawn, SHCMD("dmenu_power")},
    {MODKEY, XK_n, spawn, SHCMD("dmenu_notebook")},
    {MODKEY | ALTKEY, XK_r, spawn, SHCMD("dmenu_screen")},

    /* System */
    {0, XF86XK_MonBrightnessUp, spawn, SHCMD("sysctl bri -i 5")},
    {0, XF86XK_MonBrightnessDown, spawn, SHCMD("sysctl bri -d 5")},
    {0, XF86XK_WLAN, spawn, SHCMD("sysctl wifi --toggle")},
    {0, XF86XK_Bluetooth, spawn, SHCMD("sysctl bt --toggle")},

    /*  Media  */
    {MODKEY | ALTKEY, XK_Up, spawn, SHCMD("sysctl vol -i 5")},
    {MODKEY | ALTKEY, XK_Down, spawn, SHCMD("sysctl vol -d 5")},
    {MODKEY | ALTKEY, XK_m, spawn, SHCMD("sysctl vol --toggle")},

    {ALTKEY, XF86XK_AudioRaiseVolume, spawn, SHCMD("sysctl vol -i 5")},
    {ALTKEY, XF86XK_AudioLowerVolume, spawn, SHCMD("sysctl vol -d 5")},
    {ALTKEY, XF86XK_AudioMute, spawn, SHCMD("sysctl vol --toggle")},

    {SHIFTKEY, XF86XK_AudioRaiseVolume, spawn, SHCMD("sysctl mic -i 5")},
    {SHIFTKEY, XF86XK_AudioLowerVolume, spawn, SHCMD("sysctl mic -d 5")},
    {SHIFTKEY, XF86XK_AudioMicMute, spawn, SHCMD("sysctl mic --toggle")},

    {MODKEY | SHIFTKEY, XK_Up, spawn, SHCMD("sysctl mic -i 5")},
    {MODKEY | SHIFTKEY, XK_Down, spawn, SHCMD("sysctl mic -d 5")},
    {MODKEY | SHIFTKEY, XK_m, spawn, SHCMD("sysctl mic --toggle")},

    /* Music player controls */
    {MODKEY, XK_Right, spawn, SHCMD("mediactl --source song next")},
    {MODKEY, XK_Left, spawn, SHCMD("mediactl --source song previous")},
    {MODKEY, XK_s, spawn, SHCMD("mediactl --source song play-pause")},
    {0, XF86XK_AudioNext, spawn, SHCMD("mediactl --source song next")},
    {0, XF86XK_AudioPrev, spawn, SHCMD("mediactl --source song previous")},
    {0, XF86XK_AudioPlay, spawn, SHCMD("mediactl --source song play-pause")},

    /* Browser media controls */
    {MODKEY | ALTKEY, XK_Right, spawn, SHCMD("mediactl --source browser next")},
    {MODKEY | ALTKEY, XK_Left, spawn,
     SHCMD("mediactl --source browser previous")},
    {MODKEY | ALTKEY, XK_s, spawn,
     SHCMD("mediactl --source browser play-pause")},
    {ALTKEY, XF86XK_AudioNext, spawn, SHCMD("mediactl --source browser next")},
    {ALTKEY, XF86XK_AudioPrev, spawn,
     SHCMD("mediactl --source browser previous")},
    {ALTKEY, XF86XK_AudioPlay, spawn,
     SHCMD("mediactl --source browser play-pause")},

    /*  Applications  */
    {MODKEY, XK_Return, spawn, SHCMD("alacritty")},
    {MODKEY, XK_f, spawn, SHCMD("thunar")},
    {MODKEY, XK_b, spawn, SHCMD("qutebrowser")},
    {MODKEY, XK_e, spawn, SHCMD("neovide")},
    {MODKEY | SHIFTKEY, XK_d, spawn, SHCMD("flatpak run dev.vencord.Vesktop")},
    {MODKEY | SHIFTKEY, XK_g, spawn, SHCMD("signal-desktop")},
    {MODKEY | SHIFTKEY, XK_a, spawn, SHCMD("pavucontrol")},

    /*  Screenshot  */
    {0, XK_Print, spawn, SHCMD("sstool --select")},
    {MODKEY, XK_Print, spawn, SHCMD("sstool --screen")},
    {MODKEY | SHIFTKEY, XK_Print, spawn, SHCMD("sstool --fullscreen")},
    {MODKEY | CTRLKEY, XK_Print, spawn, SHCMD("sstool --window")},
    {MODKEY | ALTKEY, XK_Print, spawn, SHCMD("sstool --colorpicker")},

    /*  Wallpaper  */
    {MODKEY | SHIFTKEY, XK_w, spawn, SHCMD("desktopctl wallpaper theme")},

    /* Tags */
    TAGKEYS(XK_1, 0) TAGKEYS(XK_2, 1) TAGKEYS(XK_3, 2) TAGKEYS(XK_4, 3)
        TAGKEYS(XK_5, 4)

    /* Scratch Pads */
    {MODKEY, XK_t, togglescratch, {.ui = 0}},
    {MODKEY, XK_y, togglescratch, {.ui = 1}},
    {MODKEY, XK_z, togglescratch, {.ui = 2}},
    {MODKEY, XK_a, togglescratch, {.ui = 3}},
    {MODKEY, XK_g, togglescratch, {.ui = 4}},
    {MODKEY, XK_d, togglescratch, {.ui = 5}},
    {MODKEY, XK_c, togglescratch, {.ui = 6}},
    {MODKEY, XK_m, togglescratch, {.ui = 7}},
    {MODKEY, XK_i, togglescratch, {.ui = 8}},
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
    {ClkStatusText, 0, Button1, spawn, SHCMD("dmenu_run")},
    {ClkStatusText, 0, Button2, spawn, SHCMD("alacritty")},
    {ClkStatusText, 0, Button3, spawn, SHCMD("dmenu_power")},
    {ClkClientWin, MODKEY, Button1, movemouse, {0}},
    {ClkClientWin, MODKEY, Button2, togglefloating, {0}},
    {ClkClientWin, MODKEY, Button3, resizemouse, {0}},
    {ClkTagBar, 0, Button1, view, {0}},
    {ClkTagBar, 0, Button3, toggleview, {0}},
    {ClkTagBar, MODKEY, Button1, tag, {0}},
    {ClkTagBar, MODKEY, Button2, toggletag, {0}},
};
