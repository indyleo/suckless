/* See LICENSE file for copyright and license details. */

/* appearance */
static const unsigned int borderpx  = 1;        /* border pixel of windows */
static const unsigned int gappx     = 4;        /* gaps between windows */
static const unsigned int snap      = 16;       /* snap pixel */
static const unsigned int systraypinning = 0;   /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
static const unsigned int systrayonleft = 0;    /* 0: systray in the right corner, >0: systray on left of status text */
static const unsigned int systrayspacing = 2;   /* systray spacing */
static const int systraypinningfailfirst = 1;   /* 1: if pinning fails, display systray on the first monitor, False: display systray on the last monitor*/
static const int showsystray        = 1;        /* 0 means no systray */
static const int swallowfloating    = 0;        /* 1 means swallow floating windows by default */
static const int showbar            = 1;        /* 0 means no bar */
static const int topbar             = 1;        /* 0 means bottom bar */
static const char *fonts[]                    = { 
  "MesloLGS Nerd Font Mono:pixelsize=12", 
  "NotoColorEmoji:pixelsize=12:antialias=true:autohint=true" 
};
static const char normbordercolor[]           = "#3B4252";
static const char normbgcolor[]               = "#2E3440";
static const char normfgcolor[]               = "#D8DEE9";
static const char selbordercolor[]            = "#434C5E";
static const char selbgcolor[]                = "#434C5E";
static const char selfgcolor[]                = "#ECEFF4";
/* Colors based on Nord theme */
static const char *colors[][3]      = {
	/*               fg         bg         border   */
	[SchemeNorm] = { "#D8DEE9", "#2E3440", "#3B4252" },  /* normal */
	[SchemeSel]  = { "#ECEFF4", "#81A1C1", "#5E81AC" },  /* selected */
};

typedef struct {
	const char *name;
	const void *cmd;
} Sp;
const char *spcmd1[] = {"alacritty", "--class", "termsc,Termsc", NULL };
const char *spcmd2[] = {"alacritty", "--class", "yazisc,Yazisc",  "-e", "yazi",       NULL };
const char *spcmd3[] = {"alacritty", "--class", "qalsc,Qalsc",   "-e", "qalc",       NULL };
const char *spcmd4[] = {"alacritty", "--class", "pulsesc,Pulsesc", "-e", "pulsemixer", NULL };
const char *spcmd5[] = {"alacritty", "--class", "notesc,Notesc",  "-e", "nvim", "~/Documents/Markdown/Notes.md",  NULL };
const char *spcmd6[] = {"alacritty", "--class", "keymaps,Keymaps",  "-e", "zsh", "-c", "keymaps",  NULL };
static Sp scratchpads[] = {
	/* name          cmd  */
	{"termsc",      spcmd1},
	{"yazisc",      spcmd2},
	{"qalsc",       spcmd3},
	{"pulsesc",     spcmd4},
	{"notesc",      spcmd5},
	{"keymaps",     spcmd6},
};

/* tagging */
static const char *tags[] = { "󰖟", "", "󰙯", "", "", "", "󰨇" };

static const Rule rules[] = {
	/* xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 */
	/* class     instance      title           tags mask  isfloating  isterminal  noswallow  monitor */
	{ "Gimp",      NULL,        NULL,           0,         1,          0,           0,        -1 },
	{ "Firefox",   NULL,        NULL,           1 << 8,    0,          0,          -1,        -1 },
	{ "St",        NULL,        NULL,           0,         0,          1,           0,        -1 },
  { "Alacritty", NULL,        NULL,           0,         0,          1,           0,        -1 },
	{ NULL,        NULL,       "Event Tester",  0,         0,          0,           1,        -1 }, /* xev */
  { "termsc",    NULL,			  NULL,		        SPTAG(0),	 1,                                 -1 },
  { "yazisc",		 NULL,        NULL,		        SPTAG(1),	 1,			                            -1 },
  { "qalsc",		 NULL,        NULL,		        SPTAG(2),	 1,			                            -1 },
  { "pulsesc",	 NULL,        NULL,		        SPTAG(3),	 1,			                            -1 },
  { "notesc",		 NULL,        NULL,		        SPTAG(4),	 1,			                            -1 },
  { "keymaps",	 NULL,        NULL,   		    SPTAG(5),	 1,			                            -1 },
};

/* layout(s) */
static const float mfact     = 0.55; /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;    /* number of clients in master area */
static const int resizehints = 0;    /* 1 means respect size hints in tiled resizals */
static const int attachbelow = 1;    /* 1 means attach after the currently active window */
static const int lockfullscreen = 1; /* 1 will force focus on the fullscreen window */

static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "[]=",      tile },    /* first entry is default */
	{ "><>",      NULL },    /* no layout function means floating behavior */
	{ "[M]",      monocle },
};

/* key definitions */
#include <X11/XF86keysym.h>
#define MODKEY Mod4Mask
#define ALTKEY Mod1Mask
#define CTRLKEY ControlMask
#define SHIFTKEY ShiftMask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                  KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|CTRLKEY,          KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|SHIFTKEY,         KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|CTRLKEY|SHIFTKEY, KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *dmenucmd[]        = { "dmenu_run", NULL };
static const char *termcmd[]         = { "alacritty", NULL };
static const char *filemanager[]     = { "thunar", NULL };
static const char *browser[]         = { "brave-browser", NULL };
static const char *ssgui[]           = { "sstool", "--select", NULL };
static const char *ssscreen[]        = { "sstool", "--screen", NULL };
static const char *ssfull[]          = { "sstool", "--full", NULL };
static const char *guieditor[]       = { "neovide", NULL };

static const Key keys[] = {
	/* modifier                     key        function        argument */

  /* DWM Controls */
	{ MODKEY|SHIFTKEY,              XK_b,      togglebar,      {0} },
	{ MODKEY,                       XK_j,      focusstack,     {.i = +1 } },
	{ MODKEY,                       XK_k,      focusstack,     {.i = -1 } },
	{ MODKEY,                       XK_h,      setmfact,       {.f = -0.05} },
	{ MODKEY,                       XK_l,      setmfact,       {.f = +0.05} },
	{ MODKEY|SHIFTKEY,              XK_i,      incnmaster,     {.i = +1 } },
	{ MODKEY|SHIFTKEY,              XK_d,      incnmaster,     {.i = -1 } },
	{ MODKEY|SHIFTKEY,              XK_z, zoom,           {0} },
	{ MODKEY,                       XK_Tab,    view,           {0} },
	{ MODKEY,                       XK_q,      killclient,     {0} },
	{ MODKEY|ALTKEY|SHIFTKEY,       XK_t,      setlayout,      {.v = &layouts[0]} },
	{ MODKEY|ALTKEY|SHIFTKEY,       XK_f,      setlayout,      {.v = &layouts[1]} },
	{ MODKEY|ALTKEY|SHIFTKEY,       XK_m,      setlayout,      {.v = &layouts[2]} },
  { MODKEY|SHIFTKEY,              XK_f,      fullscreen,     {0} },
	{ MODKEY,                       XK_space,  setlayout,      {0} },
	{ MODKEY|SHIFTKEY,              XK_space,  togglefloating, {0} },
	{ MODKEY,                       XK_0,      view,           {.ui = ~0 } },
	{ MODKEY|SHIFTKEY,              XK_0,      tag,            {.ui = ~0 } },
	{ MODKEY,                       XK_comma,  focusmon,       {.i = -1 } },
	{ MODKEY,                       XK_period, focusmon,       {.i = +1 } },
	{ MODKEY|SHIFTKEY,              XK_comma,  tagmon,         {.i = -1 } },
	{ MODKEY|SHIFTKEY,              XK_period, tagmon,         {.i = +1 } },
	{ MODKEY|SHIFTKEY,              XK_q,      quit,           {0} },

	TAGKEYS(                        XK_1,                      0)
	TAGKEYS(                        XK_2,                      1)
	TAGKEYS(                        XK_3,                      2)
	TAGKEYS(                        XK_4,                      3)
	TAGKEYS(                        XK_5,                      4)
	TAGKEYS(                        XK_6,                      5)
	TAGKEYS(                        XK_7,                      6)

  /* Scratch Pads */
  { MODKEY,            		      	XK_t,  	   togglescratch,  {.ui = 0 } },
  { MODKEY,                		  	XK_y,	     togglescratch,  {.ui = 1 } },
  { MODKEY,                 			XK_z,	     togglescratch,  {.ui = 2 } },
  { MODKEY,                 			XK_a,	     togglescratch,  {.ui = 3 } },
  { MODKEY,                 			XK_n,	     togglescratch,  {.ui = 4 } },
  { MODKEY|SHIFTKEY,         			XK_k,	     togglescratch,  {.ui = 5 } },

  /* Media Controls  */
	{ 0,                            XF86XK_AudioMute,           spawn,          SHCMD("volumectrl --togglemute") },
	{ 0,                            XF86XK_AudioLowerVolume,    spawn,          SHCMD("volumectrl --dec") },
  { 0,                            XF86XK_AudioRaiseVolume,    spawn,          SHCMD("volumectrl --inc") },
  { MODKEY,                       XK_s,                       spawn,          SHCMD("songctrl --togglepause Supersonic") },
  { MODKEY|SHIFTKEY,              XK_s,                       spawn,          SHCMD("songctrl --skip Supersonic") },
  { MODKEY|CTRLKEY,               XK_s,                       spawn,          SHCMD("songctrl --previous Supersonic") },

  /* Brightness Controls */
  { 0,                            XF86XK_MonBrightnessUp,     spawn,          SHCMD("brightnessctrl --inc 5") },
  { 0,                            XF86XK_MonBrightnessDown,   spawn,          SHCMD("brightnessctrl --dec 5") },
  
  /* Launchers */
  { MODKEY|SHIFTKEY,              XK_p,      spawn,          SHCMD("dmenu_power") },
	{ MODKEY|SHIFTKEY,              XK_c,      spawn,          SHCMD("dmenu_clip") },
	{ MODKEY|SHIFTKEY,              XK_e,      spawn,          SHCMD("dmenu_emoji") },
	{ MODKEY|SHIFTKEY,              XK_l,      spawn,          SHCMD("slock") },
	{ MODKEY,                       XK_d,      spawn,          SHCMD("flatpak run dev.vencord.Vesktop") },
	{ MODKEY,                       XK_m,      spawn,          SHCMD("flatpak run io.github.dweymouth.supersonic") },
	{ MODKEY,                       XK_f,      spawn,          {.v = filemanager } },
	{ MODKEY,                       XK_b,      spawn,          {.v = browser } },
  { 0,                            XK_Print,  spawn,          {.v = ssgui} },
  { MODKEY,                       XK_Print,  spawn,          {.v = ssscreen} },
  { MODKEY|SHIFTKEY,              XK_Print,  spawn,          {.v = ssfull} },
	{ MODKEY,                       XK_e,      spawn,          {.v = guieditor } },
	{ MODKEY,                       XK_r,      spawn,          {.v = dmenucmd } },
	{ MODKEY,                       XK_p,      spawn,          SHCMD("dmenu_flatpak") },
	{ MODKEY,                       XK_Return, spawn,          {.v = termcmd } },
	{ MODKEY|SHIFTKEY,              XK_w,      spawn,          SHCMD("xwall xwalr ~/Pictures/Wallpapers") },

  /* Alt Tab */
  { ALTKEY,                       XK_Tab,    spawn,          SHCMD("dmenu_alttab") },
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static const Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
	{ ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[2]} },
	{ ClkWinTitle,          0,              Button2,        zoom,           {0} },
	{ ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button1,        resizemouse,    {0} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};

