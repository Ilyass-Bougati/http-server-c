#pragma once

/*
 * ANSI escape sequences for colouring terminal output, as string literals meant
 * to be concatenated into a format string:
 *
 *     printf(ANSI_BOLD ANSI_FG_RED "failed" ANSI_RESET "\n");
 *     printf(A_RED("failed") "\n");          // same thing, self-closing
 *
 * Defining ANSI_DISABLE, or the conventional NO_COLOR, makes every sequence
 * expand to an empty string, so the same format strings produce plain output
 * with no other change.
 */

#if defined(ANSI_DISABLE) || defined(NO_COLOR)
#define ANSI_(seq) ""
#else
#define ANSI_(seq) "\x1b[" seq "m"
#endif

#define ANSI_RESET ANSI_("0")

#define ANSI_BOLD ANSI_("1")
#define ANSI_DIM ANSI_("2")
#define ANSI_ITALIC ANSI_("3")
#define ANSI_UNDERLINE ANSI_("4")
#define ANSI_BLINK ANSI_("5")
#define ANSI_REVERSE ANSI_("7")
#define ANSI_HIDDEN ANSI_("8")
#define ANSI_STRIKE ANSI_("9")

#define ANSI_NO_BOLD ANSI_("22")
#define ANSI_NO_ITALIC ANSI_("23")
#define ANSI_NO_UNDERLINE ANSI_("24")
#define ANSI_NO_BLINK ANSI_("25")
#define ANSI_NO_REVERSE ANSI_("27")
#define ANSI_NO_STRIKE ANSI_("29")

#define ANSI_FG_BLACK ANSI_("30")
#define ANSI_FG_RED ANSI_("31")
#define ANSI_FG_GREEN ANSI_("32")
#define ANSI_FG_YELLOW ANSI_("33")
#define ANSI_FG_BLUE ANSI_("34")
#define ANSI_FG_MAGENTA ANSI_("35")
#define ANSI_FG_CYAN ANSI_("36")
#define ANSI_FG_WHITE ANSI_("37")
#define ANSI_FG_DEFAULT ANSI_("39")

#define ANSI_FG_BBLACK ANSI_("90")
#define ANSI_FG_BRED ANSI_("91")
#define ANSI_FG_BGREEN ANSI_("92")
#define ANSI_FG_BYELLOW ANSI_("93")
#define ANSI_FG_BBLUE ANSI_("94")
#define ANSI_FG_BMAGENTA ANSI_("95")
#define ANSI_FG_BCYAN ANSI_("96")
#define ANSI_FG_BWHITE ANSI_("97")

#define ANSI_BG_BLACK ANSI_("40")
#define ANSI_BG_RED ANSI_("41")
#define ANSI_BG_GREEN ANSI_("42")
#define ANSI_BG_YELLOW ANSI_("43")
#define ANSI_BG_BLUE ANSI_("44")
#define ANSI_BG_MAGENTA ANSI_("45")
#define ANSI_BG_CYAN ANSI_("46")
#define ANSI_BG_WHITE ANSI_("47")
#define ANSI_BG_DEFAULT ANSI_("49")

#define ANSI_BG_BBLACK ANSI_("100")
#define ANSI_BG_BRED ANSI_("101")
#define ANSI_BG_BGREEN ANSI_("102")
#define ANSI_BG_BYELLOW ANSI_("103")
#define ANSI_BG_BBLUE ANSI_("104")
#define ANSI_BG_BMAGENTA ANSI_("105")
#define ANSI_BG_BCYAN ANSI_("106")
#define ANSI_BG_BWHITE ANSI_("107")

#define ANSI_STR_(x) #x
#define ANSI_STR(x) ANSI_STR_(x)

#if defined(ANSI_DISABLE) || defined(NO_COLOR)
#define ANSI_FG_256(n) ""
#define ANSI_BG_256(n) ""
#define ANSI_FG_RGB(r, g, b) ""
#define ANSI_BG_RGB(r, g, b) ""
#else
#define ANSI_FG_256(n) "\x1b[38;5;" ANSI_STR(n) "m"
#define ANSI_BG_256(n) "\x1b[48;5;" ANSI_STR(n) "m"
#define ANSI_FG_RGB(r, g, b) "\x1b[38;2;" ANSI_STR(r) ";" ANSI_STR(g) ";" ANSI_STR(b) "m"
#define ANSI_BG_RGB(r, g, b) "\x1b[48;2;" ANSI_STR(r) ";" ANSI_STR(g) ";" ANSI_STR(b) "m"
#endif

#if defined(ANSI_DISABLE) || defined(NO_COLOR)
#define ANSI_CLEAR_SCREEN ""
#define ANSI_CLEAR_LINE ""
#define ANSI_CURSOR_HOME ""
#define ANSI_CURSOR_HIDE ""
#define ANSI_CURSOR_SHOW ""
#else
#define ANSI_CLEAR_SCREEN "\x1b[2J"
#define ANSI_CLEAR_LINE "\x1b[2K\r"
#define ANSI_CURSOR_HOME "\x1b[H"
#define ANSI_CURSOR_HIDE "\x1b[?25l"
#define ANSI_CURSOR_SHOW "\x1b[?25h"
#endif

#define ANSI_WRAP(seq, s) seq s ANSI_RESET

#define A_BOLD(s) ANSI_WRAP(ANSI_BOLD, s)
#define A_DIM(s) ANSI_WRAP(ANSI_DIM, s)
#define A_ITALIC(s) ANSI_WRAP(ANSI_ITALIC, s)
#define A_UNDER(s) ANSI_WRAP(ANSI_UNDERLINE, s)
#define A_HILITE(s) ANSI_WRAP(ANSI_REVERSE, s)
#define A_STRIKE(s) ANSI_WRAP(ANSI_STRIKE, s)

#define A_RED(s) ANSI_WRAP(ANSI_FG_RED, s)
#define A_GREEN(s) ANSI_WRAP(ANSI_FG_GREEN, s)
#define A_YELLOW(s) ANSI_WRAP(ANSI_FG_YELLOW, s)
#define A_BLUE(s) ANSI_WRAP(ANSI_FG_BLUE, s)
#define A_MAGENTA(s) ANSI_WRAP(ANSI_FG_MAGENTA, s)
#define A_CYAN(s) ANSI_WRAP(ANSI_FG_CYAN, s)
#define A_GREY(s) ANSI_WRAP(ANSI_FG_BBLACK, s)
