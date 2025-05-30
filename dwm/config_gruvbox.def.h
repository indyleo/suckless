/* See LICENSE file for copyright and license details. */

/* appearance */
static const unsigned int borderpx = 1; /* border pixel of windows */
static const unsigned int gappx = 8;    /* gaps between windows */
static const unsigned int snap = 16;    /* snap pixel */
static const unsigned int systraypinning =
    0; /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor
          X */
static const unsigned int systrayonleft =
    0; /* 0: systray in the right corner, >0: systray on left of status text */
static const unsigned int systrayspacing = 2; /* systray spacing */
static const int systraypinningfailfirst =
    1; /* 1: if pinning fails, display systray on the first monitor, False:
          display systray on the last monitor*/
static const int showsystray = 1; /* 0 means no systray */
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
const char *spcmd1[] = {"st", "-c", "termsc", "-n", "Termsc", NULL};
const char *spcmd2[] = {"st", "-c",  "yazisc", "-n",   "Yazisc",
                        "-e", "zsh", "-c",     "yazi", NULL};
const char *spcmd3[] = {"st", "-c",  "qalsc", "-n",   "Qalsc",
                        "-e", "zsh", "-c",    "qalc", NULL};
const char *spcmd4[] = {"st", "-c",  "pulsesc", "-n",         "Pulsesc",
                        "-e", "zsh", "-c",      "pulsemixer", NULL};
const char *spcmd5[] = {"st", "-c",  "notesc", "-n",    "Notesc",
                        "-e", "zsh", "-c",     "notes", NULL};
const char *spcmd6[] = {"st", "-c",  "keymaps", "-n",      "Keymaps",
                        "-e", "zsh", "-c",      "keymaps", NULL};
const char *spcmd7[] = {"st", "-c",  "discordo", "-n",       "Discordo",
                        "-e", "zsh", "-c",       "discodio", NULL};
static Sp scratchpads[] = {
    /* name          cmd  */
    {"termsc", spcmd1},   {"yazisc", spcmd2}, {"qalsc", spcmd3},
    {"pulsesc", spcmd4},  {"notesc", spcmd5}, {"keymaps", spcmd6},
    {"discordo", spcmd7},
};

/* tagging */
static const char *tags[] = {"󰖟", "", "󰙯", "",
                             "",  "", "󰨇"};

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
    {NULL, NULL, "Event Tester", 0, 0, 0, 1, -1}, /* xev */
    {"termsc", NULL, NULL, SPTAG(0), 1, -1},
    {"yazisc", NULL, NULL, SPTAG(1), 1, -1},
    {"qalsc", NULL, NULL, SPTAG(2), 1, -1},
    {"pulsesc", NULL, NULL, SPTAG(3), 1, -1},
    {"notesc", NULL, NULL, SPTAG(4), 1, -1},
    {"keymaps", NULL, NULL, SPTAG(5), 1, -1},
    {"discord", NULL, NULL, SPTAG(6), 1, -1},
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
static const char *termcmd[] = {"st", NULL};
static const char *filemanager[] = {"thunar", NULL};
static const char *browser[] = {"qutebrowser", NULL};
static const char *ssel[] = {"sstool", "--select", NULL};
static const char *sscreen[] = {"sstool", "--screen", NULL};
static const char *ssfull[] = {"sstool", "--full", NULL};
static const char *sswin[] = {"sstool", "--window", NULL};
static const char *guieditor[] = {"neovide", NULL};
static const char *Gquit[] = {"qquit", NULL};
static const char *Grestart[] = {"qrestart", NULL};
static const char *TRecord[] = {"record-toggle", NULL};

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
    {MODKEY, XK_q, killclient, {0}},
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
    {MODKEY | SHIFTKEY | CTRLKEY | ALTKEY, XK_p, quit, {0}},
    {MODKEY | SHIFTKEY, XK_r, spawn, {.v = Grestart}},
    {MODKEY | SHIFTKEY, XK_q, spawn, {.v = Gquit}},

    TAGKEYS(XK_1, 0) TAGKEYS(XK_2, 1) TAGKEYS(XK_3, 2) TAGKEYS(XK_4, 3)
        TAGKEYS(XK_5, 4) TAGKEYS(XK_6, 5) TAGKEYS(XK_7, 6)

    /* Scratch Pads */
    {MODKEY, XK_t, togglescratch, {.ui = 0}},
    {MODKEY, XK_y, togglescratch, {.ui = 1}},
    {MODKEY, XK_z, togglescratch, {.ui = 2}},
    {MODKEY, XK_a, togglescratch, {.ui = 3}},
    {MODKEY, XK_n, togglescratch, {.ui = 4}},
    {MODKEY | SHIFTKEY, XK_k, togglescratch, {.ui = 5}},
    {MODKEY, XK_d, togglescratch, {.ui = 6}},

    /* Media Controls  */
    {0, XF86XK_AudioMute, spawn, SHCMD("volumectrl --togglemute")},
    {0, XF86XK_AudioLowerVolume, spawn, SHCMD("volumectrl --dec")},
    {0, XF86XK_AudioRaiseVolume, spawn, SHCMD("volumectrl --inc")},
    {MODKEY, XK_s, spawn, SHCMD("songctrl --togglepause Supersonic")},
    {MODKEY | SHIFTKEY, XK_s, spawn, SHCMD("songctrl --skip Supersonic")},
    {MODKEY | CTRLKEY, XK_s, spawn, SHCMD("songctrl --previous Supersonic")},

    /* Brightness Controls */
    {0, XF86XK_MonBrightnessUp, spawn, SHCMD("brightnessctrl --inc 5")},
    {0, XF86XK_MonBrightnessDown, spawn, SHCMD("brightnessctrl --dec 5")},

    /* Launchers */
    {MODKEY | SHIFTKEY, XK_p, spawn, SHCMD("dmenu_power")},
    {MODKEY | SHIFTKEY, XK_c, spawn, SHCMD("dmenu_clip")},
    {MODKEY | SHIFTKEY, XK_e, spawn, SHCMD("dmenu_emoji")},
    {MODKEY | SHIFTKEY, XK_l, spawn, SHCMD("slock")},
    {MODKEY, XK_g, spawn, SHCMD("signal-desktop")},
    {MODKEY, XK_m, spawn, SHCMD("flatpak run io.github.dweymouth.supersonic")},
    {MODKEY | SHIFTKEY, XK_d, spawn, SHCMD("flatpak run dev.vencord.Vesktop")},
    {MODKEY, XK_f, spawn, {.v = filemanager}},
    {MODKEY, XK_b, spawn, {.v = browser}},
    {0, XK_Print, spawn, {.v = ssel}},
    {MODKEY, XK_Print, spawn, {.v = sscreen}},
    {MODKEY | SHIFTKEY, XK_Print, spawn, {.v = ssfull}},
    {MODKEY | CTRLKEY, XK_Print, spawn, {.v = sswin}},
    {MODKEY, XK_e, spawn, {.v = guieditor}},
    {MODKEY, XK_r, spawn, {.v = dmenucmd}},
    {MODKEY, XK_p, spawn, SHCMD("dmenu_flatpak")},
    {MODKEY, XK_Return, spawn, {.v = termcmd}},
    {MODKEY | SHIFTKEY, XK_w, spawn,
     SHCMD("xwall xwalr ~/Pictures/Wallpapers/gruvbox")},
    {MODKEY | ALTKEY, XK_r, spawn, {.v = TRecord}},

    /* Alt Tab */
    {ALTKEY, XK_Tab, spawn, SHCMD("dmenu_alttab")},
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
    {ClkStatusText, 0, Button2, spawn, {.v = termcmd}},
    {ClkClientWin, MODKEY, Button1, movemouse, {0}},
    {ClkClientWin, MODKEY, Button2, togglefloating, {0}},
    {ClkClientWin, MODKEY, Button3, resizemouse, {0}},
    {ClkTagBar, 0, Button1, view, {0}},
    {ClkTagBar, 0, Button3, toggleview, {0}},
    {ClkTagBar, MODKEY, Button1, tag, {0}},
    {ClkTagBar, MODKEY, Button2, toggletag, {0}},
};
