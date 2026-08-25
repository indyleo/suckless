/* See LICENSE file for copyright and license details.
 *
 * Central color palette. Mirrors the Gruvbox Dark cal0..cal15 naming
 * used by the companion Quickshell config (qs/Theme.qml) so both
 * projects can be re-themed from the same mental model.
 *
 * To re-theme dwm, edit the values in this file only. config.h's
 * `colors[][3]` table (drawn from the gruvbox_* variables below it)
 * now just references these names instead of hardcoding hex strings,
 * so there's a single point of edit -- same spirit as qs/Theme.qml's
 * "edit this file only" comment.
 *
 * This header only defines preprocessor string constants (not actual
 * `Clr`/QColor values like qs's version) because dwm's color pipeline
 * already turns hex strings into `Clr`s at runtime via
 * `drw_scm_create()` -- there was no reason to duplicate that here.
 */
#ifndef THEME_H
#define THEME_H

/* ---- Gruvbox Dark palette, identical to qs/Theme.qml's cal0..cal15 */
#define CAL0  "#282828"
#define CAL1  "#3c3836"
#define CAL2  "#504945"
#define CAL3  "#7c6f64"
#define CAL4  "#a89984"
#define CAL5  "#d5c4a1"
#define CAL6  "#ebdbb2"
#define CAL7  "#83a598"
#define CAL8  "#fb4934"
#define CAL9  "#d3869b"
#define CAL10 "#fabd2f"
#define CAL11 "#cc241d"
#define CAL12 "#458588"
#define CAL13 "#b8bb26"
#define CAL14 "#fe8019"
#define CAL15 "#bdae93"

/* ---- dwm-specific accents ------------------------------------------
 * A window manager needs a couple of shades a shell/bar never does
 * (selected-border tint, a harder-contrast bg for hidden tags, etc).
 * These fall outside the shared 16-color palette above, so they're
 * named separately rather than forced into a cal* slot that doesn't
 * really mean that color elsewhere. */
#define THEME_SEL_BORDER "#d79921" /* muted yellow, selected window border */
#define THEME_HID_FG "#928374"     /* faded fg for hidden-tag bar text */
#define THEME_HID_BG "#1d2021"     /* hard-contrast bg for hidden tags */
#define THEME_URG_FG "#fbf1c7"     /* near-white fg for urgent windows */

/* ---- Semantic aliases ----------------------------------------------
 * Same names/roles as qs/Theme.qml's semantic layer. Prefer these in
 * new code -- they describe *role*, not palette index, so swapping
 * the scheme later doesn't require re-reading every call site. */
#define THEME_BACKGROUND CAL0
#define THEME_SURFACE CAL1
#define THEME_SURFACE_ALT CAL2
#define THEME_BORDER CAL3
#define THEME_TEXT_MUTED CAL4
#define THEME_TEXT_SUBTLE CAL5
#define THEME_TEXT CAL6
#define THEME_INFO CAL7
#define THEME_DANGER CAL8
#define THEME_MAGENTA CAL9
#define THEME_WARNING CAL10
#define THEME_DANGER_ALT CAL11
#define THEME_BLUE CAL12
#define THEME_SUCCESS CAL13
#define THEME_ACCENT CAL14
#define THEME_TEXT_ALT CAL15

#endif /* THEME_H */
