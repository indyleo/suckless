/* See LICENSE file for copyright and license details. */
/* Default settings; can be overriden by command line. */

static int topbar = 1;                      /* -b  option; if 0, dmenu appears at bottom     */
static int fuzzy  = 1;                      /* -F  option; if 0, dmenu doesn't use fuzzy matching */
static const unsigned int alpha = 0xff;     /* Amount of opacity. 0xff is opaque             */
/* -fn option overrides fonts[0]; default X11 font or font set */
static const int user_bh = 4;               /* add an defined amount of pixels to the bar height */

static const char *fonts[] = {
	"FiraCode Nerd Font:pixelsize=12"
};
static const char *prompt      = "What to Run > ";      /* -p  option; prompt to the left of input field */
static const char *colors[SchemeLast][2] = {
    /*     fg         bg       */
    [SchemeNorm] = { "#D8DEE9", "#2E3440" },  // Light gray foreground, dark blue background
    [SchemeSel]  = { "#ECEFF4", "#5E81AC" },  // Lighter gray foreground, medium blue background for selected item
    [SchemeSelHighlight] = { "#ebcb8b", "#5E81AC" },
    [SchemeNormHighlight] = { "#ebcb8b", "#2E3440" },
    [SchemeOut]  = { "#2E3440", "#88C0D0" },  // Dark blue foreground, light blue background
};


static const unsigned int alphas[SchemeLast][2] = {
	[SchemeNorm] = { OPAQUE, 0xD9 },
	[SchemeSel] = { OPAQUE, 0xD9 },
  [SchemeSelHighlight] = { OPAQUE, 0xD9 },
  [SchemeNormHighlight] = { OPAQUE, 0xD9 },
	[SchemeOut] = { OPAQUE, 0xD9 },
};
/* -l option; if nonzero, dmenu uses vertical list with given number of lines */
static unsigned int lines      = 0;

/*
 * Characters not considered part of a word while deleting words
 * for example: " /?\"&[]"
 */
static const char worddelimiters[] = " ";
