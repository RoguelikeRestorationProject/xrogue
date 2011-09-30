/*
    io.c - Various input/output functions

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

#include <curses.h>
#include <ctype.h>
#include <stdarg.h>
#include "rogue.h"

/*
 * msg:
 *      Display a message at the top of the screen.
 */

static char msgbuf[BUFSIZ];
static int newpos = 0;

/* VARARGS */
void
msg(char *fmt, ...)
{
    va_list ap;
    /*
     * if the string is "", just clear the line
     */
    if (*fmt == '\0')
    {
        wclear(msgw);
        overwrite(cw, msgw);
        wmove(msgw, 0, 0);
        clearok(msgw, FALSE);
        draw(msgw);
        mpos = 0;
        return;
    }
    /*
     * otherwise add to the message and flush it out
     */
    va_start(ap, fmt);
    doadd(fmt, ap);
    va_end(ap);
    endmsg();
}

/*
 * add things to the current message
 */

/* VARARGS */
void
addmsg(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    doadd(fmt, ap);
    va_end(ap);
}

/*
 * If there is no current message, do nothing.  Otherwise, prompt the
 * player with the --More-- string.  Then erase the message.
 */

rmmsg()
{
    if (mpos) {
        wclear(msgw);
        overwrite(cw, msgw);
        mvwaddstr(msgw, 0, 0, huh);
        waddstr(msgw, morestr);
        clearok(msgw, FALSE);
        draw(msgw);
        wait_for(' ');
        msg("");
    }
}

/*
 * Display a new msg (giving him a chance to see the previous one if it
 * is up there with the --More--)
 */

endmsg()
{
    /* Needed to track where we are for 5.0 (PC) curses */
    register int x, y;

    if (mpos) {
        /*
         * If this message will fit on the line (plus space for --More--)
         * then just add it (only during combat).
         */
        if (player.t_quiet < 0 && mpos + newpos + strlen(morestr) + 5 < cols) {
            wmove(msgw, 0, mpos + 5);
            newpos += mpos + 5;
            strcat(huh, "  ");
        }
        else {
            wclear(msgw);
            overwrite(cw, msgw);
            mvwaddstr(msgw, 0, 0, huh);
            waddstr(msgw, morestr);
            clearok(msgw, FALSE);
            draw(msgw);
            wait_for(' ');
            wclear(msgw);
            overwrite(cw, msgw);
            wmove(msgw, 0, 0);
            huh[0] = '\0';
        }
    }
    else {
        wclear(msgw);
        overwrite(cw, msgw);
        wmove(msgw, 0, 0);
        huh[0] = '\0';
    }
    strcat(huh, msgbuf);
    mvwaddstr(msgw, 0, 0, huh);
    getyx(msgw, y, x);
    mpos = newpos;
    newpos = 0;
    wmove(msgw, y, x);
    clearok(msgw, FALSE);
    draw(msgw);
}

doadd(char *fmt, va_list ap)
{
    vsprintf((char *) &msgbuf[newpos], fmt, ap);
    newpos = strlen(msgbuf);
}

/*
 * step_ok:
 *      returns true if it is ok for type to step on ch
 *      flgptr will be NULL if we don't know what the monster is yet!
 */

step_ok(y, x, can_on_monst, flgptr)
register int y, x, can_on_monst;
register struct thing *flgptr;
{
    /* can_on_monst = MONSTOK if all we care about are physical obstacles */
    register struct linked_list *item;
    register struct thing *tp;
    unsigned char ch;

    /* What is here?  Don't check monster window if MONSTOK is set */
    if (can_on_monst == MONSTOK) ch = mvinch(y, x);
    else ch = winat(y, x);

    if (can_on_monst == FIGHTOK && isalpha(ch) &&
        (item = find_mons(y, x)) != NULL) {
        tp = THINGPTR(item);    /* What monster is here? */

        /* We can hit it if we're after it */
        if (flgptr->t_dest == &tp->t_pos) return TRUE;

        /*
         * Otherwise, if we're friendly we'll hit it unless it is also
         * friendly or is our race.
         */
        if (off(*flgptr, ISFRIENDLY)    ||
            on(*tp, ISFRIENDLY)         ||
            flgptr->t_index == tp->t_index) return FALSE;
        else return TRUE;
    }
    else switch (ch)
    {
        case ' ':
        case VERTWALL:
        case HORZWALL:
        case SECRETDOOR:
            if (flgptr && on(*flgptr, CANINWALL)) return(TRUE);
            return FALSE;
        when SCROLL:
            if (can_on_monst == MONSTOK) {      /* Not a real obstacle */
                move_free = 0;                  /* check free movement */
                return(TRUE);
            }
            /*
             * If it is a scroll, it might be a scare monster scroll
             * so we need to look it up to see what type it is.
             */
            if (flgptr && flgptr->t_ctype == C_MONSTER) {
                move_free = 1;
                item = find_obj(y, x);
                if (item != NULL &&
                    (OBJPTR(item))->o_which==S_SCARE &&
                    (flgptr == NULL || flgptr->t_stats.s_intel < 17)) {
                        move_free = 2;
                        return(FALSE); /* All but smart ones are scared */
                }
            }
            return(TRUE);
        otherwise:
            return (!isalpha(ch));
    }
    /* return(FALSE); */
    /*NOTREACHED*/
}

/*
 * shoot_ok:
 *      returns true if it is ok for type to shoot over ch
 */

shoot_ok(int ch)
{
    switch (ch)
    {
        case ' ':
        case VERTWALL:
        case HORZWALL:
        case SECRETDOOR:
        case FOREST:
            return FALSE;
        default:
            return (!isalpha(ch));
    }
}

/*
 * status:
 *      Display the important stats line.  Keep the cursor where it was.
 */

status(display)
bool display;   /* is TRUE, display unconditionally */
{
    register struct stats *stat_ptr, *max_ptr;
    register int oy = 0, ox = 0, temp;
    register char *pb;
    char buf[LINELEN];
    static int hpwidth = 0, s_hungry = -1;
    static int s_lvl = -1, s_hp = -1, s_str, maxs_str, 
                s_ac = 0;
    static short s_intel, s_dext, s_wisdom, s_const, s_charisma;
    static short maxs_intel, maxs_dext, maxs_wisdom, maxs_const, maxs_charisma;
    static unsigned long s_exp = 0;
    static int s_carry, s_pack;
    bool first_line=FALSE;

    /* Go to English mode */
    nofont(cw);

    stat_ptr = &pstats;
    max_ptr  = &max_stats;

    /*
     * If nothing has changed in the first line, then skip it
     */
    if (!display                                &&
        s_lvl == level                          && 
        s_intel == stat_ptr->s_intel            &&
        s_wisdom == stat_ptr->s_wisdom          &&
        s_dext == dex_compute()                 && 
        s_const == stat_ptr->s_const            &&
        s_charisma == stat_ptr->s_charisma      &&
        s_str == str_compute()                  && 
        s_hungry == hungry_state                &&
        maxs_intel == max_ptr->s_intel          && 
        maxs_wisdom == max_ptr->s_wisdom        &&
        maxs_dext == max_ptr->s_dext            && 
        maxs_const == max_ptr->s_const          &&
        maxs_charisma == max_ptr->s_charisma    &&
        maxs_str == max_ptr->s_str              ) goto line_two;

    /* Display the first line */
    first_line = TRUE;
    getyx(cw, oy, ox);
    sprintf(buf, "Int:%d(%d)  Str:%d", stat_ptr->s_intel,
        max_ptr->s_intel, str_compute());

    /* Maximum strength */
    pb = &buf[strlen(buf)];
    sprintf(pb, "(%d)", max_ptr->s_str);

    pb = &buf[strlen(buf)];
    sprintf(pb, "  Wis:%d(%d)  Dxt:%d(%d)  Con:%d(%d)  Cha:%d(%d)",
        stat_ptr->s_wisdom,max_ptr->s_wisdom,dex_compute(),max_ptr->s_dext,
        stat_ptr->s_const,max_ptr->s_const,stat_ptr->s_charisma,
        max_ptr->s_charisma);

    /* Update first line status */
    s_intel = stat_ptr->s_intel;
    s_wisdom = stat_ptr->s_wisdom;
    s_dext = dex_compute();
    s_const = stat_ptr->s_const;
    s_charisma = stat_ptr->s_charisma;
    s_str = str_compute();
    maxs_intel = max_ptr->s_intel;
    maxs_wisdom = max_ptr->s_wisdom;
    maxs_dext = max_ptr->s_dext;
    maxs_const = max_ptr->s_const;
    maxs_charisma = max_ptr->s_charisma;
    maxs_str = max_ptr->s_str;

    /* Print the line */
    mvwaddstr(cw, lines-2, 0, buf);
    switch (hungry_state) {
        case F_SATIATED:
            waddstr(cw, "  Satiated");
        when F_OKAY: ;
        when F_HUNGRY:
            waddstr(cw, "  Hungry");
        when F_WEAK:
            waddstr(cw, "  Weak");
        when F_FAINT:
            waddstr(cw, "  Fainting");
    }
    wclrtoeol(cw);
    s_hungry = hungry_state;

    /*
     * If nothing has changed since the last status, don't
     * bother.
     */
line_two: 
    if (!display                                        &&
        s_lvl == level                                  && 
        s_hp == stat_ptr->s_hpt                         && 
        s_ac == ac_compute(FALSE) - dext_prot(s_dext)   &&
        s_pack == stat_ptr->s_pack                      &&
        s_carry == stat_ptr->s_carry                    &&
        s_exp == stat_ptr->s_exp                        ) {
        newfont(cw);
        return;
    }
        
    if (!first_line) getyx(cw, oy, ox);
    if (s_hp != max_ptr->s_hpt) {
        temp = s_hp = max_ptr->s_hpt;
        for (hpwidth = 0; temp; hpwidth++)
            temp /= 10;
    }
    sprintf(buf, "Lvl:%d  Hp:%*d(%*d)  Ac:%d  Carry:%d(%d)  Exp:%d/%lu  %s",
        level, hpwidth, stat_ptr->s_hpt, hpwidth, max_ptr->s_hpt,
        ac_compute(FALSE) - dext_prot(s_dext),stat_ptr->s_pack/10,
        stat_ptr->s_carry/10, stat_ptr->s_lvl, stat_ptr->s_exp, 
        cnames[player.t_ctype][min(stat_ptr->s_lvl-1, NUM_CNAMES-1)]);

    /*
     * Save old status
     */
    s_lvl = level;
    s_hp = stat_ptr->s_hpt;
    s_ac = ac_compute(FALSE) - dext_prot(s_dext);
    s_pack = stat_ptr->s_pack;
    s_carry = stat_ptr->s_carry;
    s_exp = stat_ptr->s_exp; 
    mvwaddstr(cw, lines-1, 0, buf);
    wclrtoeol(cw);
    newfont(cw);
    wmove(cw, oy, ox);
}

/*
 * wait_for
 *      Sit around until the guy types the right key
 */

wait_for(ch)
register char ch;
{
    register char c;

    clearok(msgw, FALSE);
    if (ch == '\n') {
        while ((c = wgetch(msgw)) != '\n' && c != '\r') {
            continue;
        }
    }
    else {
        while (wgetch(msgw) != ch) {
            continue;
        }
    }
}


/*
 * over_win:
 *      Given a current window, a new window, and the max y and x of the
 *      new window, paint the new window on top of the old window without
 *      destroying any of the old window.  Current window and new window
 *      are assumed to have lines lines and cols columns (max y and max x
 *      pertain only the the useful information to be displayed.
 *      If redraw is non-zero, we wait for the character "redraw" to be
 *      typed and then redraw the starting screen.
 */

over_win(oldwin, newin, maxy, maxx, cursory, cursorx, redraw)
WINDOW *oldwin, *newin;
int maxy, maxx, cursory, cursorx;
char redraw;
{
    char blanks[LINELEN+1];
    register int line, i;
    WINDOW *ow; /* Overlay window */

    /* Create a blanking line */
    for (i=0; i<maxx && i<cols && i<LINELEN; i++) blanks[i] = ' ';
    blanks[i] = '\0';

    /* Create the window we will display */
    ow = newwin(lines, cols, 0, 0);

    /* Blank out the area we want to use */
    if (oldwin == cw) {
        msg("");
        line = 1;
    }
    else line = 0;

    overwrite(oldwin, ow);      /* Get a copy of the old window */

    /* Do the remaining blanking */
    for (; line < maxy; line++) mvwaddstr(ow, line, 0, blanks);

    overlay(newin, ow); /* Overlay our new window */

    /* Move the cursor to the specified location */
    wmove(ow, cursory, cursorx);

    clearok(ow, FALSE);         /* Draw inventory without clearing */
    draw(ow);

    if (redraw) {
        wait_for(redraw);

        clearok(oldwin, FALSE);         /* Setup to redraw current screen */
        touchwin(oldwin);               /* clearing first */
        draw(oldwin);
    }

    delwin(ow);
}


/*
 * show_win:
 *      function used to display a window and wait before returning
 */

show_win(scr, message)
register WINDOW *scr;
char *message;
{
    mvwaddstr(scr, 0, 0, message);
    touchwin(scr);
    wmove(scr, hero.y, hero.x);
    draw(scr);
    wait_for(' ');
    restscr(cw);
}

/*
 * dbotline:
 *      Displays message on bottom line and waits for a space to return
 */

dbotline(scr,message)
WINDOW *scr;
char *message;
{
        mvwaddstr(scr,lines-1,0,message);
        draw(scr);
        wait_for(' ');  
}

/*
 * restscr:
 *      Restores the screen to the terminal
 */

restscr(scr)
WINDOW *scr;
{
        clearok(scr,TRUE);
        touchwin(scr);
        draw(scr);
}

/*
 * netread:
 *      Read a byte, short, or long machine independently
 *      Always returns the value as an unsigned long.
 */

unsigned long
netread(error, size, stream)
int *error;
int size;
FILE *stream;
{
    unsigned long result = 0L,  /* What we read in */
                  partial;      /* Partial value */
    int nextc,  /* The next byte */
        i;      /* To index through the result a byte at a time */

    /* Be sure we have a right sized chunk */
    if (size < 1 || size > 4) {
        *error = 1;
        return(0L);
    }

    for (i=0; i<size; i++) {
        nextc = getc(stream);
        if (nextc == EOF) {
            *error = 1;
            return(0L);
        }
        else {
            partial = (unsigned long) (nextc & 0xff);
            partial <<= 8*i;
            result |= partial;
        }
    }

    *error = 0;
    return(result);
}

/*
 * netwrite:
 *      Write out a byte, short, or long machine independently.
 */

netwrite(value, size, stream)
unsigned long value;    /* What to write */
int size;       /* How much to write out */
FILE *stream;   /* Where to write it */
{
    int i;      /* Goes through value one byte at a time */
    char outc;  /* The next character to be written */

    /* Be sure we have a right sized chunk */
    if (size < 1 || size > 4) return(0);

    for (i=0; i<size; i++) {
        outc = (char) ((value >> (8 * i)) & 0xff);
        putc(outc, stream);
    }
    return(size);
}

