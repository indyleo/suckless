
/* See LICENSE file for copyright and license details. */

/* Appearance */
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
/* Gruvbox color variables -- these are now just aliases into theme.h,
 * so this block never has to be hand-edited again: edit theme.h
 * instead (same "one file to re-theme everything" idea as qs's
 * Theme.qml). Values below are byte-identical to before the refactor. */
#include "theme.h"
static const char gruvbox_normfgcolor[] = THEME_TEXT;
static const char gruvbox_normbgcolor[] = THEME_BACKGROUND;
static const char gruvbox_normbordercolor[] = THEME_SURFACE;

static const char gruvbox_selfgcolor[] = THEME_BACKGROUND;
static const char gruvbox_selbgcolor[] = THEME_WARNING;
static const char gruvbox_selbordercolor[] = THEME_SEL_BORDER;

static const char gruvbox_hidfgcolor[] = THEME_HID_FG;
static const char gruvbox_hidbgcolor[] = THEME_HID_BG;
static const char gruvbox_hidbordercolor[] = THEME_SURFACE;

static const char gruvbox_urgfgcolor[] = THEME_URG_FG;
static const char gruvbox_urgbgcolor[] = THEME_DANGER_ALT;
static const char gruvbox_urgbordercolor[] = THEME_DANGER;

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

const char *spcmd1[] = {"st", "-c", "termsc", "-n", "Termsc", NULL};
const char *spcmd2[] = {"st", "-c",  "lfsc", "-n", "Lfsc",
                        "-e", "zsh", "-c",   "lf", NULL};
const char *spcmd3[] = {"st", "-c", "qalsc", "-n", "Qalsc", "-e", "qalc", NULL};
const char *spcmd4[] = {"st",        "-c", "wiremixsc", "-n",
                        "Wiremixsc", "-e", "wiremix",   NULL};
const char *spcmd5[] = {"st", "-c",  "musicsc", "-n",           "Musicsc",
                        "-e", "zsh", "-c",      "subsonic-tui", NULL};
static Sp scratchpads[] = {
    {"termsc", spcmd1},    {"lfsc", spcmd2},    {"qalsc", spcmd3},
    {"wiremixsc", spcmd4}, {"musicsc", spcmd5},
};

/* Autostart */
static const char *const autostart[] = {"/usr/local/bin/autostart.sh", NULL,
                                        NULL};

/* Tagging */
const Tag tags[] = {
    {"󰖟", "web"}, {"󰙯", "chat"}, {"", "dev"},
    {"", "game"}, {"󰨇", "vm"},
};
const int tagslen = LENGTH(tags);

static const Rule rules[] = {
    /*  class                            instance  title tags mask
isfloating  isterminal  noswallow  monitor    w    h    x    y
setpos  center  forcefullscreen */
    {"^Gimp$", NULL, NULL, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0},
    {"^Firefox$", NULL, NULL, 1 << 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0},
    {"^St$", NULL, NULL, 0, 0, 1, 0, -1, 0, 0, 0, 0, 0, 0, 0},
    {"^Alacritty$", NULL, NULL, 0, 0, 1, 0, -1, 0, 0, 0, 0, 0, 0, 0},
    {"^org\\.wezfurlong\\.wezterm$", NULL, NULL, 0, 0, 1, 0, -1, 0, 0, 0, 0, 0,
     0, 0},
    {NULL, NULL, "^Event Tester$", 0, 0, 0, 1, -1, 0, 0, 0, 0, 0, 0,
     0}, /* xev */
    {"^feishin$", NULL, NULL, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
    {"^Dragon$", NULL, NULL, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0},
    {NULL, NULL, "^Picture-in-Picture$", 0, 1, 0, 0, -1, 480, 270, 14, 12, 1, 0,
     0},
    {"^steam_app_(?!0$)[0-9]+$", NULL, NULL, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0,
     1},
    {NULL, NULL, "^StarCraft II$", 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 1},
    {NULL, NULL, "^Brood War$", 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 1},

    /* Scratchpads */
    {"^termsc$", NULL, NULL, SPTAG(0), 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0},
    {"^lfsc$", NULL, NULL, SPTAG(1), 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0},
    {"^qalsc$", NULL, NULL, SPTAG(2), 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0},
    {"^wiremixsc$", NULL, NULL, SPTAG(3), 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0},
    {"^musicsc", NULL, NULL, SPTAG(5), 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0},
};

/* Layout(s) */
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

/* Key Definitions */
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

/* Helper for spawning shell commands */
#define SHCMD(cmd)                                                             \
  {                                                                            \
    .v = (const char *[]) { "/bin/sh", "-c", cmd, NULL }                       \
  }

/* Statusbar Config */
const char *statusdelim = " || ";
const int statusmaxlen = 45;
const int statusclickable = 1;
const int statusleaddelim = 0;
const int statustraildelim = 0;

/* Statusbar Blocks */
const StatusBlock statusblocks[] = {
    /* icon   cmd   interval(s) */
    {"", "mediactl state-title", 0},
    {"",
     "export BLOCK_BUTTON; o=$(sysstats battery); case \"$o\" in "
     "\"\"|*N/A*|*No*|*Not*|*\" 0%\"|\"0%\") ;; "
     "*) printf '%s' \"$o\" ;; "
     "esac",
     15},
    {"", "sysstats brightness", 0}, // refreshed by the OSD, see below
    {"",
     "export BLOCK_BUTTON; o=$(sysstats ethernet); case \"$o\" in "
     "*Connected*) printf '%s' \"$o\" ;; "
     "esac",
     15},
    {"",
     "export BLOCK_BUTTON; w=$(sysstats wifi); case \"$w\" in "
     "*Offline*|*\"No tool\"*|*Disconnected*) "
     "e=$(BLOCK_BUTTON= sysstats ethernet); case \"$e\" in "
     "*Disconnected*) printf '%s' \"$w\" ;; "
     "esac ;; "
     "*) printf '%s' \"$w\" ;; "
     "esac",
     15},
    {"",
     "export BLOCK_BUTTON; t=$(sysstats tail); case \"$t\" in "
     "*\"Not connected\"*) ;; "
     "*Connected*) printf '%s' \"$t\" ;; "
     "esac",
     30},

    {"", "sysstats microphone", 0}, // refreshed by the OSD, see below
    {"", "sysstats volume", 0},     // refreshed by the OSD, see below

    {"", "sysstats date_time", 30},

    /* Notifications -- text is never produced by this empty command; it's
     * pushed directly by notifications.c via statusbar_setblock()
     * whenever the unread count or DND state changes. See notifblockidx
     * below and statusbar.c's statusbar_handleclick(), which routes
     * clicks on this block to notif_blockclick() instead of rerunning
     * this (empty) command. */
    {"", "", 0},
};
const int statusblockslen = LENGTH(statusblocks);

/* index into statusblocks[] (above) of the notification bell/count --
 * must stay in sync if you reorder statusblocks[]. -1 would disable the
 * bar indicator (popups/history/DND still work either way). */
const int notifblockidx = 9;

/* Volume/Microphone Commands For OSD */
static const char *volupcmd[] = {"sysctl", "vol", "-i", "5", NULL};
static const char *voldowncmd[] = {"sysctl", "vol", "-d", "5", NULL};
static const char *voltogglecmd[] = {"sysctl", "vol", "--toggle", NULL};
static const char *briupcmd[] = {"sysctl", "bri", "-i", "5", NULL};
static const char *bridowncmd[] = {"sysctl", "bri", "-d", "5", NULL};
static const char *micupcmd[] = {"sysctl", "mic", "-i", "5", NULL};
static const char *micdowncmd[] = {"sysctl", "mic", "-d", "5", NULL};
static const char *mictogglecmd[] = {"sysctl", "mic", "--toggle", NULL};
static const char *volgetcmd[] = {"sysstats", "vol_raw", NULL};
static const char *micgetcmd[] = {"sysstats", "mic_raw", NULL};
static const char *brigetcmd[] = {"sysstats", "bri_raw", NULL};
enum {
  OsdVolUp,
  OsdVolDown,
  OsdVolToggle,
  OsdBriUp,
  OsdBriDown,
  OsdMicUp,
  OsdMicDown,
  OsdMicToggle
}; // indices into osds[], referenced from keys[] as {.i = OsdVolUp} etc.

const OsdItem osds[] = {
    // label  changecmd       getcmd      blockidx  fastget
    // blockidx indices below must match statusblocks[] above: brightness=2,
    // microphone=6, volume=7. These were previously 12 (out of range --
    // silently
    // ignored by statusbar_setblock()/statusbar_refresh()'s bounds checks) for
    // volume, and -1 (explicitly disabled) for brightness/mic, even though all
    // three of those statusblocks entries are marked "refreshed by the OSD" and
    // have interval=0 so nothing else ever refreshes them. Net effect: the
    // volume/brightness/mic numbers shown in the bar only ever reflected their
    // startup values and silently went stale the moment you pressed a volume,
    // brightness, or mic key, even though the OSD popup itself always showed
    // the correct live value (it reads it independently via getcmd/fastget).
    {"VOL", volupcmd, volgetcmd, 7, osd_vol_fastget},
    {"VOL", voldowncmd, volgetcmd, 7, osd_vol_fastget},
    {"VOL", voltogglecmd, volgetcmd, 7, osd_vol_fastget},
    {"BRI", briupcmd, brigetcmd, 2, osd_bri_fastget},
    {"BRI", bridowncmd, brigetcmd, 2, osd_bri_fastget},
    {"MIC", micupcmd, micgetcmd, 6, osd_mic_fastget},
    {"MIC", micdowncmd, micgetcmd, 6, osd_mic_fastget},
    {"MIC", mictogglecmd, micgetcmd, 6, osd_mic_fastget},
};
const int osdslen = LENGTH(osds);

/* Commands */
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
    {MODKEY | SHIFTKEY, XK_f, togglefullscreen, {0}},
    {MODKEY | SHIFTKEY, XK_space, togglefloating, {0}},
    {MODKEY, XK_comma, focusmon, {.i = -1}},
    {MODKEY, XK_period, focusmon, {.i = +1}},
    {MODKEY | SHIFTKEY, XK_comma, tagmon, {.i = -1}},
    {MODKEY | SHIFTKEY, XK_period, tagmon, {.i = +1}},
    {MODKEY, XK_equal, show, {0}},
    {MODKEY | CTRLKEY | SHIFTKEY, XK_equal, showall, {0}},
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
    {MODKEY, XK_n, spawn, SHCMD("notebook")},
    {MODKEY | SHIFTKEY, XK_c, clippick, {0}},
    {MODKEY | CTRLKEY, XK_c, clippin, {0}},
    {MODKEY | SHIFTKEY | CTRLKEY, XK_c, clipclear, {0}},
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
    {MODKEY, XK_m, togglescratch, {.ui = 4}}, /* musicsc */
    {MODKEY | SHIFTKEY,
     XK_t,
     hideallscratchpads,
     {0}}, /* hide all scratchpads */
};

/* Button Definitions */
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
