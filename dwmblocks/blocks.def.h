// Modify this file to change what commands output to your statusbar, and
// recompile using the make command.
static const Block blocks[] = {
    /*Icon*/ /*Command*/ /*Update Interval*/ /*Update Signal*/

    {"", "isrec", 0, 12},

    // {"", "browserctrl -bst 25", 1, 13},

    {"", "songctrl -sst Supersonic 20", 1, 14},

    {"", "brightnessctrl --get", 0, 15},

    {"", "volumectrl --printvol", 0, 16},

    {"", "battery", 60, 0},

    {"", "sysstats date_time", 5, 0},
};

// sets delimiter between status commands. NULL character ('\0') means no
// delimiter.
static char delim[] = " || ";
static unsigned int delimLen = 5;
