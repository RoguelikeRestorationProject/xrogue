/*
    wizard.c - Special wizard commands
    
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
 * Special wizard commands (some of which are also non-wizard commands
 * under strange circumstances)
 */

#include <curses.h>
#include <ctype.h>
#include <string.h>
#include "rogue.h"
#include "mach_dep.h"

/*
 * create_obj:
 *      Create any object for wizard, scroll, magician, or cleric
 */

create_obj(prompt, which_item, which_type)
bool prompt;
int which_item, which_type;
{
    reg struct linked_list *item;
    reg struct object *obj;
    reg int wh;
    char *pt;
    reg int ch, whc, newtype = 0, msz, newitem;
    WINDOW *thiswin;

    thiswin = cw;
    if (prompt) {
        bool nogood = TRUE;

        thiswin = hw;
        wclear(hw);
        wprintw(hw,"Item\t\t\tKey\n\n");
        wprintw(hw,"%s\t\t\t%c\n%s\t\t\t%c\n",things[TYP_RING].mi_name,RING,
                things[TYP_STICK].mi_name,STICK);
        wprintw(hw,"%s\t\t\t%c\n%s\t\t\t%c\n",things[TYP_POTION].mi_name,POTION,
                things[TYP_SCROLL].mi_name,SCROLL);
        wprintw(hw,"%s\t\t\t%c\n%s\t\t\t%c\n",things[TYP_ARMOR].mi_name,ARMOR,
                things[TYP_WEAPON].mi_name,WEAPON);
        wprintw(hw,"%s\t%c\n",things[TYP_MM].mi_name,MM);
        wprintw(hw,"%s\t\t\t%c\n",things[TYP_FOOD].mi_name,FOOD);
        if (wizard) {
            wprintw(hw,"%s\t\t%c\n",things[TYP_RELIC].mi_name,RELIC);
            wprintw(hw,"monster\t\t\tm");
        }
        wprintw(hw,"\n\nWhat do you want to create? ");
        draw(hw);
        do {
            ch = wgetch(hw);
            if (ch == ESC) {
                restscr(cw);
                return;
            }
            switch (ch) {
                case RING:
                case STICK:     
                case POTION:
                case SCROLL:    
                case ARMOR:     
                case WEAPON:
                case FOOD:
                case MM:
                    nogood = FALSE;
                    break;
                case RELIC:
                case 'm':
                    if (wizard) 
                        nogood = FALSE;
                    break;
                default:
                    nogood = TRUE;
            }
        } while (nogood);
        newitem = ch;
    }
    else
        newitem = which_item;

    pt = "those";
    msz = 0;
    if(newitem == 'm') {
        /* make monster and be done with it */
        wh = makemonster(TRUE, "create");
        if (wh > 0) {
            creat_mons (&player, wh, TRUE);
            light(&hero);
        }
        return;
    }
    if(newitem == GOLD) pt = "gold";
    else if(isatrap(newitem)) pt = "traps";

    switch(newitem) {
        case POTION:    whc = TYP_POTION;       msz = MAXPOTIONS;
        when SCROLL:    whc = TYP_SCROLL;       msz = MAXSCROLLS;
        when WEAPON:    whc = TYP_WEAPON;       msz = MAXWEAPONS;
        when ARMOR:     whc = TYP_ARMOR;        msz = MAXARMORS;
        when RING:      whc = TYP_RING;         msz = MAXRINGS;
        when STICK:     whc = TYP_STICK;        msz = MAXSTICKS;
        when MM:        whc = TYP_MM;           msz = MAXMM;
        when RELIC:     whc = TYP_RELIC;        msz = MAXRELIC;
        when FOOD:      whc = TYP_FOOD;         msz = MAXFOODS;
        otherwise:
            if (thiswin == hw)
                restscr(cw);
            mpos = 0;
            msg("Even wizards can't create %s !!",pt);
            return;
    }
    if(msz == 1) {              /* if only one type of item */
        ch = 'a';
    }
    else if (prompt) {
        register struct magic_item *wmi;
        char wmn;
        register int ii;
        int old_prob;

        mpos = 0;
        wmi = NULL;
        wmn = 0;
        switch(newitem) {
                case POTION:    wmi = &p_magic[0];
                when SCROLL:    wmi = &s_magic[0];
                when RING:      wmi = &r_magic[0];
                when STICK:     wmi = &ws_magic[0];
                when MM:        wmi = &m_magic[0];
                when RELIC:     wmi = &rel_magic[0];
                when FOOD:      wmi = &foods[0];
                when WEAPON:    wmn = 1;
                when ARMOR:     wmn = 2;
        }
        wclear(hw);
        thiswin = hw;
        if (wmi != NULL) {
            ii = old_prob = 0;
            while (ii < msz) {
                if(wmi->mi_prob == old_prob && wizard == FALSE) { 
                    msz--; /* can't make a unique item */
                }
                else {
                    mvwaddch(hw,ii % 13,ii > 12 ? cols/2 : 0, ii + 'a');
                    waddstr(hw,") ");
                    waddstr(hw,wmi->mi_name);
                    ii++;
                }
                old_prob = wmi->mi_prob;
                wmi++;
            }
        }
        else if (wmn != 0) {
            for(ii = 0 ; ii < msz ; ii++) {
                mvwaddch(hw,ii % 13,ii > 12 ? cols/2 : 0, ii + 'a');
                waddstr(hw,") ");
                if(wmn == 1)
                    waddstr(hw,weaps[ii].w_name);
                else
                    waddstr(hw,armors[ii].a_name);
            }
        }
        sprintf(prbuf,"Which %s? ",things[whc].mi_name);
        mvwaddstr(hw,lines - 1, 0, prbuf);
        draw(hw);
        do {
            ch = wgetch(hw);
            if (ch == ESC) {
                restscr(cw);
                msg("");
                return;
            }
        } until (isalpha(ch));
        if (thiswin == hw)                      /* restore screen if need be */
            restscr(cw);
        newtype = ch - 'a';
        if(newtype < 0 || newtype >= msz) {     /* if an illegal value */
            mpos = 0;
            msg("There is no such %s",things[whc].mi_name);
            return;
        }
    }
    else 
        newtype = which_type;
    item = new_item(sizeof *obj);       /* get some memory */
    obj = OBJPTR(item);
    obj->o_type = newitem;              /* store the new items */
    obj->o_mark[0] = '\0';
    obj->o_which = newtype;
    obj->o_group = 0;
    obj->contents = NULL;
    obj->o_count = 1;
    obj->o_flags = 0;
    obj->o_dplus = obj->o_hplus = 0;
    obj->o_weight = 0;
    wh = obj->o_which;
    mpos = 0;
    if (!wizard)                /* users get 0 to +5 */
        whc = rnd(6);
    else                        /* wizard gets to choose */
        whc = getbless();
    if (whc < 0)
        obj->o_flags |= ISCURSED;
    switch (obj->o_type) {
        case WEAPON:
        case ARMOR:
            if (obj->o_type == WEAPON) {
                init_weapon(obj, wh);
                obj->o_hplus += whc;
                if (!wizard) whc = rnd(6);
                obj->o_dplus += whc;
            }
            else {                              /* armor here */
                obj->o_weight = armors[wh].a_wght;
                obj->o_ac = armors[wh].a_class - whc;
            }
        when RING:
            r_know[wh] = TRUE;
            switch(wh) {
                case R_ADDSTR:
                case R_ADDWISDOM:
                case R_ADDINTEL:
                case R_PROTECT:
                case R_ADDHIT:
                case R_ADDDAM:
                case R_DIGEST:
                    obj->o_ac = whc + 2;
                    break;
                default: 
                    obj->o_ac = 0;
            }
            obj->o_weight = things[TYP_RING].mi_wght;
        when MM:
            if (whc > 1 && m_magic[wh].mi_bless != 0)
                obj->o_flags |= ISBLESSED;
            m_know[wh] = TRUE;
            switch(wh) {
                case MM_JUG:
                    switch(rnd(11)) {
                        case 0: obj->o_ac = P_PHASE;
                        when 1: obj->o_ac = P_CLEAR;
                        when 2: obj->o_ac = P_SEEINVIS;
                        when 3: obj->o_ac = P_HEALING;
                        when 4: obj->o_ac = P_MFIND;
                        when 5: obj->o_ac = P_TFIND;
                        when 6: obj->o_ac = P_HASTE;
                        when 7: obj->o_ac = P_RESTORE;
                        when 8: obj->o_ac = P_FLY;
                        when 9: obj->o_ac = P_SKILL;
                        when 10:obj->o_ac = P_FFIND;
                    }
                when MM_HUNGER:
                case MM_CHOKE:
            if (whc < 0 )
            whc = -whc;     /* cannot be negative */
            obj->o_ac = (whc + 1) * 2;
            break;
                when MM_OPEN:
                case MM_DRUMS:
                case MM_DISAPPEAR:
                case MM_KEOGHTOM:
                    if (whc < 0)
                        whc = -whc;     /* these cannot be negative */
                    obj->o_ac = (whc + 3) * 5;
                    break;
                when MM_BRACERS:
                    obj->o_ac = whc + 4;
                when MM_DISP:
                    obj->o_ac = 3;
                when MM_PROTECT:
                    obj->o_ac = whc + 4;
                when MM_SKILLS:
                    if (whc < 2)
                        obj->o_ac = rnd(NUM_CHARTYPES-1);
                    else
                        obj->o_ac = player.t_ctype;
        when MM_CRYSTAL:
            obj->o_ac = 1;
                otherwise: 
                    obj->o_ac = 0;
            }
            obj->o_weight = things[TYP_MM].mi_wght;
        when STICK:
            if (whc > 1 && ws_magic[wh].mi_bless != 0)
                obj->o_flags |= ISBLESSED;
            ws_know[wh] = TRUE;
            fix_stick(obj);
        when SCROLL:
            if (whc > 3 && s_magic[wh].mi_bless != 0)
                obj->o_flags |= ISBLESSED;
            obj->o_weight = things[TYP_SCROLL].mi_wght;
            s_know[wh] = TRUE;
        when POTION:
            if (whc > 3 && p_magic[wh].mi_bless != 0)
                obj->o_flags |= ISBLESSED;
            obj->o_weight = things[TYP_POTION].mi_wght;
            if (wh == P_ABIL) obj->o_kind = rnd(NUMABILITIES);
            p_know[wh] = TRUE;
        when RELIC:
            obj->o_weight = things[TYP_RELIC].mi_wght;
            switch (obj->o_which) {
                case QUILL_NAGROM: obj->o_charges = QUILLCHARGES;
                when EMORI_CLOAK:  obj->o_charges = 1;
                otherwise: break;
            }
        when FOOD:
            obj->o_weight = things[TYP_FOOD].mi_wght;
    }
    mpos = 0;
    obj->o_flags |= ISKNOW;
    if (add_pack(item, FALSE) == FALSE) {
        obj->o_pos = hero;
        fall(item, TRUE);
    }
}

/*
 * getbless:
 *      Get a blessing for a wizards object
 */

int
getbless()
{
        reg char bless;

        msg("Blessing? (+,-,n)");
        bless = wgetch(msgw);
        if (bless == '+')
                return (15);
        else if (bless == '-')
                return (-1);
        else
                return (0);
}

/*
 * get a non-monster death type
 */

getdeath()
{
    register int i;
    int which_death;
    char label[80];

    clear();
    for (i=0; i<DEATHNUM; i++) {
        sprintf(label, "[%d] %s", i+1, deaths[i].name);
        mvaddstr(i+2, 0, label);
    }
    mvaddstr(0, 0, "Which death? ");
    refresh();

    /* Get the death */
    for (;;) {
        get_str(label, stdscr);
        which_death = atoi(label);
        if ((which_death < 1 || which_death > DEATHNUM)) {
            mvaddstr(0, 0, "Please enter a number in the displayed range -- ");
            refresh();
        }
        else break;
    }
    return(deaths[which_death-1].reason);
}

/*
 * make a monster for the wizard
 */

makemonster(showall, action) 
bool showall;   /* showall -> show uniques and genocided creatures */
char *action;
{
    register int i;
    register short which_monst;
    register int num_monst = NUMMONST, pres_monst=1, num_lines=2*(lines-3);
    int max_monster;
    char monst_name[40];

    /* If we're not showing all, subtract UNIQUES, DINOS, and quartermaster */
    if (!showall) num_monst -= NUMUNIQUE + NUMDINOS + 1;
    max_monster = num_monst;

    /* Print out the monsters */

    if (levtype == OUTSIDE) {
        num_monst = NUMDINOS;
        max_monster = NUMMONST - 1;
        pres_monst = (pres_monst + NUMMONST - NUMDINOS - 1);
    }

    while (num_monst > 0) {
        register int left_limit;

        if (num_monst < num_lines) left_limit = (num_monst+1)/2;
        else left_limit = num_lines/2;

        wclear(hw);
        touchwin(hw);

        /* Print left column */
        wmove(hw, 2, 0);
        for (i=0; i<left_limit; i++) {
            sprintf(monst_name, "[%d] %c%s\n",
                                pres_monst,
                                (showall || monsters[pres_monst].m_normal)
                                    ? ' '
                                    : '*',
                                monsters[pres_monst].m_name);
            waddstr(hw, monst_name);
            pres_monst++;
        }

        /* Print right column */
        for (i=0; i<left_limit && pres_monst<=max_monster; i++) {
            sprintf(monst_name, "[%d] %c%s",
                                pres_monst,
                                (showall || monsters[pres_monst].m_normal)
                                    ? ' '
                                    : '*',
                                monsters[pres_monst].m_name);
            wmove(hw, i+2, cols/2);
            waddstr(hw, monst_name);
            pres_monst++;
        }

        if ((num_monst -= num_lines) > 0) {
            mvwaddstr(hw, lines-1, 0, morestr);
            draw(hw);
            wait_for(' ');
        }

        else {
            mvwaddstr(hw, 0, 0, "Which monster");
            if (!terse) {
                waddstr(hw, " do you wish to ");
                waddstr(hw, action);
            }
            waddstr(hw, "? ");
            draw(hw);
        }
    }

get_monst:
    get_str(monst_name, hw);
    which_monst = atoi(monst_name);
    if (levtype == OUTSIDE)
    if ((which_monst < NUMMONST-NUMDINOS || which_monst > max_monster)) {
        mvwaddstr(hw, 0, 0, "Please enter a number in the displayed range -- ");
        draw(hw);
        goto get_monst;
    }
    if ((which_monst < 1 || which_monst > max_monster)) {
        mvwaddstr(hw, 0, 0, "Please enter a number in the displayed range -- ");
        draw(hw);
        goto get_monst;
    }
    restscr(cw);
    return(which_monst);
}

/*
 * passwd:
 *      see if user knows password
 */

bool
passwd()
{
    register char *sp, c;
    char buf[LINELEN];

    msg("Wizard's Password:");
    mpos = 0;
    sp = buf;
    while ((c = wgetch(cw)) != '\n' && c != '\r' && c != '\033') {
        if (c == killchar())
            sp = buf;
        else if (c == erasechar() && sp > buf)
            sp--;
        else
            *sp++ = c;
    }
    if (sp == buf)
        return FALSE;
    *sp = '\0';
    return (strcmp(PASSWD, xcrypt(buf, "mT")) == 0);

    /* don't mess with the password here or elsewhere.
     *
     * If anyone goes wizard they forfeit being placed in the scorefile.
     * So, no need to be secretive about it.  Let them have it!
     *
     * Additionally, you can begin the game as wizard by starting it
     * with a null argument, as in: xrogue ""
     */
}

/*
 * teleport:
 *      Bamf the hero someplace else
 */

void
teleport()
{
    register struct room *new_rp = NULL, *old_rp = roomin(&hero);
    register int rm, which;
    coord old;
    bool got_position = FALSE;

    /* Disrupt whatever the hero was doing */
    dsrpt_player();

    /*
     * If the hero wasn't doing something disruptable, NULL out his
     * action anyway and let him know about it.  We don't want him
     * swinging or moving into his old place.
     */
    if (player.t_action != A_NIL) {
        player.t_action = A_NIL;
        msg("You feel momentarily disoriented.");
    }

    old = hero;
    mvwaddch(cw, hero.y, hero.x, mvwinch(stdscr, hero.y, hero.x));
    if (ISWEARING(R_TELCONTROL) || wizard) {
        got_position = move_hero(H_TELEPORT);
        if (!got_position)
            msg("Your attempt fails.");
        else {
            new_rp = roomin(&hero);
            msg("You teleport successfully.");
        }
    }
    if (!got_position) {
        do {
            rm = rnd_room();
            rnd_pos(&rooms[rm], &hero);
        } until(winat(hero.y, hero.x) == FLOOR);
        new_rp = &rooms[rm];
    }
    player.t_oldpos = old;      /* Save last position */

    /* If hero gets moved, darken old room */
    if (old_rp && old_rp != new_rp) {
        old_rp->r_flags |= FORCEDARK;   /* Fake darkness */
        light(&old);
        old_rp->r_flags &= ~FORCEDARK; /* Restore light state */
    }

    /* Darken where we just came from */
    else if (levtype == MAZELEV) light(&old);

    light(&hero);
    mvwaddch(cw, hero.y, hero.x, PLAYER);
    /* if entering a treasure room, wake everyone up......Surprise! */
    if (new_rp->r_flags & ISTREAS)
        wake_room(new_rp);

    /* Reset current room and position */
    oldrp = new_rp;     /* Used in look() */
    player.t_oldpos = hero;
    /*
     * make sure we set/unset the ISINWALL on a teleport
     */
    which = winat(hero.y, hero.x);
    if (isrock(which)) turn_on(player, ISINWALL);
    else turn_off(player, ISINWALL);

    /*
     * turn off ISHELD in case teleportation was done while fighting
     * something that holds you
     */
    if (on(player, ISHELD)) {
        register struct linked_list *ip, *nip;
        register struct thing *mp;

        turn_off(player, ISHELD);
        hold_count = 0;
        for (ip = mlist; ip; ip = nip) {
            mp = THINGPTR(ip);
            nip = next(ip);
            if (on(*mp, DIDHOLD)) {
                turn_off(*mp, DIDHOLD);
                turn_on(*mp, CANHOLD);
            }
            turn_off(*mp, DIDSUFFOCATE); /* Suffocation -- see below */
        }
    }

    /* Make sure player does not suffocate */
    extinguish(suffocate);

    count = 0;
    running = FALSE;
    flushinp();
}

/*
 * whatis:
 *      What a certin object is
 */

whatis(what)
struct linked_list *what;
{
    register struct object *obj;
    register struct linked_list *item;

    if (what == NULL) {         /* do we need to ask which one? */
        if ((item = get_item(pack, "identify", IDENTABLE, FALSE, FALSE))==NULL)
            return;
    }
    else
        item = what;
    obj = OBJPTR(item);
    switch (obj->o_type) {
        case SCROLL:
            s_know[obj->o_which] = TRUE;
            if (s_guess[obj->o_which]) {
                free(s_guess[obj->o_which]);
                s_guess[obj->o_which] = NULL;
            }
        when POTION:
            p_know[obj->o_which] = TRUE;
            if (p_guess[obj->o_which]) {
                free(p_guess[obj->o_which]);
                p_guess[obj->o_which] = NULL;
            }
        when STICK:
            ws_know[obj->o_which] = TRUE;
            if (ws_guess[obj->o_which]) {
                free(ws_guess[obj->o_which]);
                ws_guess[obj->o_which] = NULL;
            }
        when RING:
            r_know[obj->o_which] = TRUE;
            if (r_guess[obj->o_which]) {
                free(r_guess[obj->o_which]);
                r_guess[obj->o_which] = NULL;
            }
        when MM:
            /* If it's an identified jug, identify its potion */
            if (obj->o_which == MM_JUG && (obj->o_flags & ISKNOW)) {
                if (obj->o_ac != JUG_EMPTY)
                    p_know[obj->o_ac] = TRUE;
                break;
            }

            m_know[obj->o_which] = TRUE;
            if (m_guess[obj->o_which]) {
                free(m_guess[obj->o_which]);
                m_guess[obj->o_which] = NULL;
            }
        otherwise:
            break;
    }
    obj->o_flags |= ISKNOW;
    if (what == NULL)
        msg(inv_name(obj, FALSE));
}

/* 
 *  Choose a quest item
 *      (if on STARTLEV equipage level = 0)
 */

choose_qst()
{
    bool doit = TRUE;
    bool escp = TRUE;

    /* let wizard in on this too */
    if (waswizard == TRUE || (levtype == POSTLEV && level == 0)) {
        wclear(hw);
    touchwin(hw);
        wmove(hw, 2, 0);
        wprintw(hw, "a) Cloak of Emori\n");
        wprintw(hw, "b) Ankh of Heil\n");
        wprintw(hw, "c) Quill of Nagrom\n");
        wprintw(hw, "d) Eye of Vecna\n");
        wprintw(hw, "e) Ring of Surtur\n");
        wprintw(hw, "f) Staff of Ming\n");
        wprintw(hw, "g) Wand of Orcus\n");
        wprintw(hw, "h) Rod of Asmodeus\n");
        wprintw(hw, "i) Amulet of Yendor\n");
        wprintw(hw, "j) Amulet of Stonebones\n");
        wprintw(hw, "k) Mandolin of Brian\n"); 
        wprintw(hw, "l) Horn of Geryon\n");
        wprintw(hw, "m) Daggers of Musty Doit\n");
        wprintw(hw, "n) Axe of Aklad\n");
        wprintw(hw, "o) Morning Star of Hruggek\n");
        wprintw(hw, "p) Flail of Yeenoghu\n");
        wprintw(hw, "q) Card of Alteran\n");
        mvwaddstr(hw, 0, 0, "Select a quest item: "); /* prompt */

        if (menu_overlay)  /* Print the selections.  The longest line is
                * Hruggek (26 characters).  The prompt is 21.
                */
            over_win(cw, hw, 20, 29, 0, 21, NULL);
        else
            draw(hw);

        while (doit) {
        switch (wgetch(cw)) {
            case EOF:
            case ESC:
                escp = FALSE;   /* used below */
            doit = FALSE;
            when 'a':
                quest_item = EMORI_CLOAK;
            doit = FALSE;
            when 'b':
                quest_item = HEIL_ANKH;
            doit = FALSE;
            when 'c':
                quest_item = QUILL_NAGROM;
            doit = FALSE;
            when 'd':
                quest_item = EYE_VECNA;
            doit = FALSE;
            when 'e':
                quest_item = SURTUR_RING;
            doit = FALSE;
            when 'f':
                quest_item = MING_STAFF;
            doit = FALSE;
            when 'g':
                quest_item = ORCUS_WAND;
            doit = FALSE;
            when 'h':
                quest_item = ASMO_ROD;
            doit = FALSE;
            when 'i':
                quest_item = YENDOR_AMULET;
            doit = FALSE;
            when 'j':
                quest_item = STONEBONES_AMULET;
            doit = FALSE;
            when 'k':
                quest_item = BRIAN_MANDOLIN;
            doit = FALSE;
            when 'l':
                quest_item = GERYON_HORN;
            doit = FALSE;
            when 'm':
                quest_item = MUSTY_DAGGER;
            doit = FALSE;
            when 'n':
                quest_item = AXE_AKLAD;
            doit = FALSE;
            when 'o':
                quest_item = HRUGGEK_MSTAR;
            doit = FALSE;
            when 'p':
                quest_item = YEENOGHU_FLAIL;
            doit = FALSE;
            when 'q':
                quest_item = ALTERAN_CARD;
            doit = FALSE;
            otherwise:
            doit = TRUE;
        }
    }
    if (menu_overlay) {
            status(FALSE);
            touchwin(cw);
        if (escp == TRUE) {
                msg("Your quest item is the %s.  --More--",
             rel_magic[quest_item].mi_name);
            wait_for(' ');
        }
        return;
    }
    else {
        if (escp == TRUE) {
                wmove(hw, lines-4, 0);
                wprintw(hw, "Your quest item is the %s.",
                    rel_magic[quest_item].mi_name);
        }
            wmove(hw, lines-1, 0);
            wprintw(hw, spacemsg);
            draw(hw);
            wait_for(' ');
            wclear(hw);
            draw(hw);
            wmove(cw, 0, 0);
            wclrtoeol(cw);
            status(FALSE);
            touchwin(cw);
        return;
    }
    }
    else {
        msg("You can no longer select a quest item. ");
        return;
    }
}

