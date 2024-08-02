#ifndef CONFIG_H
#define CONFIG_H

#define ASCII_LOGO_LINES 7
const char *ascii_logo[ASCII_LOGO_LINES] = {
    "    .--.",
    "   |o_o |",
    "   |:_/ |",
    "  //   \\ \\",
    " (|     | )",
    "/'\\_   _/`\\",
    "\\___)=(___/"
};

#define INFO_WIDTH 50
#define TOTAL_HEIGHT 25

#define RESET_COLOR "\033[0m"
#define GREEN_COLOR "\033[0;32m"
#define RED_COLOR "\033[0;31m"
#define YELLOW_COLOR "\033[0;33m"
#define BLUE_COLOR "\033[0;34m"
#define MAGENTA_COLOR "\033[0;35m"
#define CYAN_COLOR "\033[0;36m"
#define WHITE_COLOR "\033[0;37m"

#define ASCII_LOGO_COLOR "\033[0;36m"

#endif // CONFIG.H