/*
    options.c - This file has all the code for the option command
    
    XRogue: Expeditions into the Dungeons of Doom
    Copyright (C) 1991 Robert Pietkivitch
    All rights reserved.
    
    Based on "Advanced Rogue"
    Copyright (C) 1984, 1985 Michael Morgan, Ken Dalka and AT&T
    All rights reserved.

    Based on "Rogue: Exploring the Dungeons of Doom"
    Copyright (C) 1980, 1981 Michael Toy, Ken Arnold and Glenn Wichman
    All rights reserved.
    
    See the file LICENSE.TXT for full copyright and licensing information.
*/  
    
 /*
 * I would rather this command were not necessary, but
 * it is the only way to keep the wolves off of my back.
 */

#include <curses.h>
#include <ctype.h>
#include "rogue.h"

#define NUM_OPTS        (sizeof optlist / sizeof (OPTION))

/*
 * description of an option and what to do with it
 */
struct optstruct {
    char        *o_name;        /* option name */
    char        *o_prompt;      /* prompt for interactive entry */
    int         *o_opt;         /* pointer to thing to set */
    int         (*o_putfunc)(); /* function to print value */
    int         (*o_getfunc)(); /* function to get value interactively */
};

typedef struct optstruct        OPTION;

int     put_bool(), 
        get_bool(),
        put_str(),
        get_str(),
        put_abil(),
        get_abil(),
        get_quest(),
        put_quest(),
    get_default();

OPTION  optlist[] = {
    {"terse",   "Terse output: ",
                (int *) &terse,         put_bool,       get_bool        },
    {"flush",   "Flush typeahead during battle: ",
                (int *) &fight_flush,   put_bool,       get_bool        },
    {"jump",    "Show position only at end of run: ",
                (int *) &jump,          put_bool,       get_bool        },
    {"step",    "Do inventories one line at a time: ",
                (int *) &slow_invent,   put_bool,       get_bool        },
    {"askme",   "Ask me about unidentified things: ",
                (int *) &askme,         put_bool,       get_bool        },
    {"pickup",  "Pick things up automatically: ",
                (int *) &auto_pickup,   put_bool,       get_bool        },
    {"overlay", "Overlay menu: ",
                (int *) &menu_overlay,  put_bool,       get_bool        },
    {"name",    "Name: ",
                (int *) whoami,         put_str,        get_str         },
    {"file",    "Save file: ",
                (int *) file_name,      put_str,        get_str         },
    {"score",   "Score file: ",
                (int *) score_file,     put_str,        get_str         },
    {"class",   "Character type: ",
                (int *) &char_type,     put_abil,       get_abil        },
    {"quest",   "Quest item: ",
                (int *) &quest_item,    put_quest,      get_quest       },
    {"default", "Default Attributes: ",
                (int *) &def_attr,      put_bool,    get_default     }
};

/*
 * The default attribute field is read-only
 */

get_default(bp, win)
bool *bp;
WINDOW *win;
{
    register int oy, ox;

    getyx(win, oy, ox);
    put_bool(bp, win);
    get_ro(win, oy, ox);
}

/*
 * The ability (class) field is read-only
 */

get_abil(abil, win)
int *abil;
WINDOW *win;
{
    register int oy, ox;

    getyx(win, oy, ox);
    put_abil(abil, win);
    get_ro(win, oy, ox);
}

/*
 * The quest field is read-only
 */

get_quest(quest, win)
int *quest;
WINDOW *win;
{
    register int oy, ox;

    getyx(win, oy, ox);
    waddstr(win, rel_magic[*quest].mi_name);
    get_ro(win, oy, ox);
}

/*
 * get_ro:
 *      "Get" a read-only value.
 */

get_ro(win, oy, ox)
WINDOW *win;
register int oy, ox;
{
    register int ny, nx;
    register bool op_bad;
    
    op_bad = TRUE;
    getyx(win, ny, nx);
    while(op_bad)       
    {
        wmove(win, oy, ox);
        draw(win);
        switch (wgetch(win))
        {
            case '\n':
            case '\r':
                op_bad = FALSE;
                break;
            case '\033':
            case '\007':
                return QUIT;
            case '-':
                return MINUS;
            default:
                mvwaddstr(win, ny, nx + 5, "(no change allowed)");
        }
    }
    wmove(win, ny, nx + 5);
    wclrtoeol(win);
    wmove(win, ny, nx);
    waddch(win, '\n');
    return NORM;
}

/*
 * allow changing a boolean option and print it out
 */

get_bool(bp, win)
bool *bp;
WINDOW *win;
{
    register int oy, ox;
    register bool op_bad;

    op_bad = TRUE;
    getyx(win, oy, ox);
    waddstr(win, *bp ? "True" : "False");
    while(op_bad)       
    {
        wmove(win, oy, ox);
        draw(win);
        switch (wgetch(win))
        {
            case 't':
            case 'T':
                *bp = TRUE;
                op_bad = FALSE;
                break;
            case 'f':
            case 'F':
                *bp = FALSE;
                op_bad = FALSE;
                break;
            case '\n':
            case '\r':
                op_bad = FALSE;
                break;
            case '\033':
            case '\007':
                return QUIT;
            case '-':
                return MINUS;
            default:
                mvwaddstr(win, oy, ox + 10, "(T or F)");
        }
    }
    wmove(win, oy, ox);
    wclrtoeol(win);
    waddstr(win, *bp ? "True" : "False");
    waddch(win, '\n');
    return NORM;
}

/*
 * set a string option
 */

get_str(opt, win)
register char *opt;
WINDOW *win;
{
    register char *sp;
    register int c, oy, ox;
    char buf[LINELEN];

    draw(win);
    getyx(win, oy, ox);
    /*
     * loop reading in the string, and put it in a temporary buffer
     */
    for (sp = buf;
        (c = wgetch(win)) != '\n'       && 
        c != '\r'                       && 
        c != '\033'                     && 
        c != '\007'                     &&
        sp < &buf[LINELEN-1];
        wclrtoeol(win), draw(win))
    {
        if (c == -1)
            continue;
        else if (c == erasechar()) /* process erase character */
        {
            if (sp > buf)
            {
                register int i;

                sp--;
                for (i = strlen(unctrl(*sp)); i; i--)
                    waddch(win, '\b');
            }
            continue;
        }
        else if (c == killchar()) /* process kill character */
        {
            sp = buf;
            wmove(win, oy, ox);
            continue;
        }
        else if (sp == buf)
            if (c == '-' && win == hw)  /* To move back a line in hw */
                break;
            else if (c == '~')
            {
                strcpy(buf, home);
                waddstr(win, home);
                sp += strlen(home);
                continue;
            }
        *sp++ = c;
        waddstr(win, unctrl(c));
    }
    *sp = '\0';
    if (sp > buf)       /* only change option if something has been typed */
        strucpy(opt, buf, strlen(buf));
    wmove(win, oy, ox);
    waddstr(win, opt);
    waddch(win, '\n');
    draw(win);
    if (win == msgw)
        mpos += sp - buf;
    if (c == '-')
        return MINUS;
    else if (c == '\033' || c == '\007')
        return QUIT;
    else
        return NORM;
}

/*
 * print and then set options from the terminal
 */

option()
{
    register OPTION     *op;
    register int        retval;

    wclear(hw);
    touchwin(hw);
    /*
     * Display current values of options
     */
    for (op = optlist; op < &optlist[NUM_OPTS]; op++)
    {
        waddstr(hw, op->o_prompt);
        (*op->o_putfunc)(op->o_opt, hw);
        waddch(hw, '\n');
    }
    /*
     * Set values
     */
    wmove(hw, 0, 0);
    for (op = optlist; op < &optlist[NUM_OPTS]; op++)
    {
        waddstr(hw, op->o_prompt);

		retval = (*op->o_getfunc)(op->o_opt, hw);

        if (retval)
            if (retval == QUIT)
                break;
            else if (op > optlist) {    /* MINUS */
                wmove(hw, (op - optlist) - 1, 0);
                op -= 2;
            }
            else        /* trying to back up beyond the top */
            {
                putchar('\007');
                wmove(hw, 0, 0);
                op--;
            }
    }
    /*
     * Switch back to original screen
     */
    mvwaddstr(hw, lines-1, 0, spacemsg);
    draw(hw);
    wait_for(' ');
    restscr(cw);
    after = FALSE;
}

/*
 * parse options from string, usually taken from the environment.
 * the string is a series of comma seperated values, with booleans
 * being stated as "name" (true) or "noname" (false), and strings
 * being "name=....", with the string being defined up to a comma
 * or the end of the entire option string.
 */

parse_opts(str)
register char *str;
{
    register char *sp;
    register OPTION *op;
    register int len;

    if (*str == '\"')
       str++;

    while (*str)
    {
        /*
         * Get option name
         */

        for (sp = str; isalpha(*sp); sp++)
            continue;
        len = (char *)sp - str;
        /*
         * Look it up and deal with it
         */
        for (op = optlist; op < &optlist[NUM_OPTS]; op++)
            if (EQSTR(str, op->o_name, len))
            {
                if (op->o_putfunc == put_bool)  /* if option is a boolean */
                    *(bool *)op->o_opt = TRUE;
                else                            /* string option */
                {
                    register char *start;
                    char value[LINELEN];

                    /*
                     * Skip to start of string value
                     */
                    for (str = sp + 1; *str == '=' || *str == ':'; str++)
                        continue;

                    if (*str == '~')
                    {
                        strcpy((char *) value, home);
                        start = (char *) value + strlen(home);
                        while (*++str == '/')
                            continue;
                    }
                    else
                        start = (char *) value;
                    /*
                     * Skip to end of string value
                     */
                    for (sp = str + 1; *sp && *sp != ',' && *sp != '\"'; sp++)
                        continue;
                    strucpy(start, str, (char *) sp - str);

                    /* Put the value into the option field */
                    if (op->o_putfunc != put_abil) 
                        strcpy((char *)op->o_opt, value);

                    else if (*op->o_opt == -1) { /* Only init ability once */
                        register int len = strlen(value);
                        register int i;

                        for (i=0; i<NUM_CHARTYPES-1; i++) {
                            if (EQSTR(value, char_class[i].name, len)) {
                                *op->o_opt = i;
                                break;
                            }
                        }
                    }
                }
                break;
            }
            /*
             * check for "noname" for booleans
             */
            else if (op->o_putfunc == put_bool
              && EQSTR(str, "no", 2) && EQSTR(str + 2, op->o_name, len - 2))
            {
                *(bool *)op->o_opt = FALSE;
                break;
            }

        /*
         * skip to start of next option name
         */
        while (*sp && !isalpha(*sp))
            sp++;
        str = sp;
    }
}


/*
 * print the default attributes
 */

/* put_default(b, win)
 * bool *b;
 * WINDOW *win;
 * {
 *     waddstr(win, *b ? "True" : "False");
 * }
 */

/*
 * print the character type
 */

put_abil(ability, win)
int *ability;
WINDOW *win;
{
    waddstr(win, char_class[*ability].name);
}

/*
 * print out the quest
 */

put_quest(quest, win)
int *quest;
WINDOW *win;
{
    waddstr(win, rel_magic[*quest].mi_name);
}

/*
 * put out a boolean
 */

put_bool(b, win)
bool    *b;
WINDOW *win;
{
    waddstr(win, *b ? "True" : "False");
}

/*
 * put out a string
 */

put_str(str, win)
char *str;
WINDOW *win;
{
    waddstr(win, str);
}

