/*
    command.c  -  Read and execute the user commands
 
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
#include <signal.h>
#include "mach_dep.h"
#include "rogue.h"

/*
 * command:
 *      Process the user commands
 */

command()
{
    unsigned int ch;
    struct linked_list *item;
    unsigned int countch = 0, direction = 0, newcount = FALSE;
    int segment = 1;
    int monst_limit, monst_current;

    monst_limit = monst_current = 1;
    while (playing) {
        /*
         * Let the daemons start up, but only do them once a round
         * (round = 10 segments).
         */
        if (segment >= 10) {
            do_daemons(BEFORE);
            do_fuses(BEFORE);
        }

        after = TRUE;
        do {
            /* One more tick of the clock. */
            if (segment >= 10 && after && (++turns % DAYLENGTH) == 0) {
                daytime ^= TRUE;
                if (levtype == OUTSIDE) {
                    if (daytime) msg("A bright star flares above the horizon.");
                    else msg("The bright star travels beyond the horizon.");
                }
                light(&hero);
            }

            /*
             * Don't bother with these updates unless the player's going
             * to do something.
             */
            if (player.t_action == A_NIL && player.t_no_move <= 1) {
                look(after, FALSE);
                lastscore = purse;
                wmove(cw, hero.y, hero.x);
                if (!((running || count) && jump)) {
                    status(FALSE);
                }
            }

            /* Draw the screen */
            if (!((running || count) && jump)) {
                wmove(cw, hero.y, hero.x);
                draw(cw);
            }

            after = TRUE;

            /*
             * Read command or continue run
             */
            if (--player.t_no_move <= 0) {
                take = 0;               /* Nothing here to start with */
                player.t_no_move = 0;   /* Be sure we don't go too negative */
                if (!running) door_stop = FALSE;

                /* Was the player being held? */
                if (player.t_action == A_FREEZE) {
                    player.t_action = A_NIL;
                    msg("You can move again.");
                }

                if (player.t_action != A_NIL) ch = player.t_action;
                else if (running) {
                    char scratch;

                    /* If in a corridor or maze, if we are at a turn with
                     * only one way to go, turn that way.
                     */
                    scratch = winat(hero.y, hero.x);
                    if ((scratch==PASSAGE||scratch==DOOR||levtype==MAZELEV)  &&
                        off(player, ISHUH)                                   && 
                        off(player, ISBLIND)) {
                        int y, x;
                        if (getdelta(runch, &y, &x) == TRUE) {
                            corr_move(y, x);
                        }
                    }
                    ch = runch;
                }
                else if (count) ch = countch;
                else {
                    ch = wgetch(cw);
                    if (mpos != 0 && !running)  /* Erase message if its there */
                        msg("");
                }

                /*
                 * check for prefixes
                 */
                if (isascii(ch) && isdigit(ch))
                {
                    count = 0;
                    newcount = TRUE;
                    while (isascii(ch) && isdigit(ch))
                    {
                        count = count * 10 + (ch - '0');
                        ch = wgetch(cw);
                    }
                    countch = ch;
                    /*
                     * turn off count for commands which don't make sense
                     * to repeat
                     */
                    switch (ch) {
                        case 'h': case 'j': case 'k': case 'l':
                        case 'y': case 'u': case 'b': case 'n':
                        case 'H': case 'J': case 'K': case 'L':
                        case 'Y': case 'U': case 'B': case 'N':
                        case C_SEARCH: case '.':
                            break;
                        default:
                            count = 0;
                    }
                }

                /* Save current direction */
                if (!running) { /* If running, it is already saved */
                    switch (ch) {
                        case 'h': case 'j': case 'k': case 'l':
                        case 'y': case 'u': case 'b': case 'n':
                        case 'H': case 'J': case 'K': case 'L':
                        case 'Y': case 'U': case 'B': case 'N':
                            runch = tolower(ch);
                    }
                }

                /* Perform the action */
                switch (ch) {
                    case 'f':
                        if (!on(player, ISBLIND))
                        {
                            door_stop = TRUE;
                            firstmove = TRUE;
                        }
                        if (count && !newcount)
                            ch = direction;
                        else
                            ch = wgetch(cw);
                        switch (ch)
                        {
                            case 'h': case 'j': case 'k': case 'l':
                            case 'y': case 'u': case 'b': case 'n':
                                ch = toupper(ch);
                        }
                        direction = ch;
                }
                newcount = FALSE;

                /*
                 * execute a command
                 */
                if (count && !running)
                    count--;

                switch (ch) {
                    case '!' : shell();
                    case KEY_LEFT       : do_move(0, -1);
                    when KEY_DOWN       : do_move(1, 0);
                    when KEY_UP         : do_move(-1, 0);
                    when KEY_RIGHT      : do_move(0, 1);
                    when KEY_HOME       : do_move(-1, -1);
                    when KEY_A1         : do_move(-1, -1);
                    when KEY_PPAGE      : do_move(-1, 1);
                    when KEY_A3         : do_move(-1, 1);
                    when KEY_END         : do_move(1, -1);
                    when KEY_C1         : do_move(1, -1);
                    when KEY_NPAGE      : do_move(1, 1);
                    when KEY_C3         : do_move(1, 1);
#ifdef CTL_RIGHT
                    when CTL_RIGHT      : do_run('l');
                    when CTL_LEFT       : do_run('h');
                    when CTL_UP         : do_run('k');
                    when CTL_DOWN       : do_run('j');
                    when CTL_HOME       : do_run('y');
                    when CTL_PGUP       : do_run('u');
                    when CTL_END        : do_run('b');
                    when CTL_PGDN       : do_run('n');
#endif
                    when 'h' : do_move(0, -1);
                    when 'j' : do_move(1, 0);
                    when 'k' : do_move(-1, 0);
                    when 'l' : do_move(0, 1);
                    when 'y' : do_move(-1, -1);
                    when 'u' : do_move(-1, 1);
                    when 'b' : do_move(1, -1);
                    when 'n' : do_move(1, 1);
                    when 'H' : do_run('h');
                    when 'J' : do_run('j');
                    when 'K' : do_run('k');
                    when 'L' : do_run('l');
                    when 'Y' : do_run('y');
                    when 'U' : do_run('u');
                    when 'B' : do_run('b');
                    when 'N' : do_run('n');
                    when A_ATTACK:
                        /* Is our attackee still there? */
                        if (isalpha(winat(player.t_newpos.y,
                                          player.t_newpos.x))) {
                            /* Our friend is still here */
                            player.t_action = A_NIL;
                            fight(&player.t_newpos, cur_weapon, FALSE);
                        }
                        else {  /* Our monster has moved */
                            player.t_action = A_NIL;
                        }
                    when A_PICKUP:
                        player.t_action = A_NIL;
                        if (add_pack((struct linked_list *)NULL, FALSE)) {
                            char tch;
                            tch = mvwinch(stdscr, hero.y, hero.x);
                            if (tch != FLOOR && tch != PASSAGE) {
                                player.t_action = A_PICKUP; /*get more */
                                player.t_no_move += 2 * movement(&player);
                            }
                        }
                    when A_THROW:
                        if (player.t_action == A_NIL) {
                            item = get_item(pack, "throw", ALL, FALSE, FALSE);
                            if (item != NULL && get_dir(&player.t_newpos)) {
                                player.t_action = A_THROW;
                                player.t_using = item;
                                player.t_no_move = 2 * movement(&player);
                            }
                            else
                                after = FALSE;
                        }
                        else {
                            missile(player.t_newpos.y, player.t_newpos.x, 
                                    player.t_using, &player);
                            player.t_action = A_NIL;
                            player.t_using = 0;
                        }
                    when 'a' :
                        if (player.t_action == A_NIL) {
                            if (get_dir(&player.t_newpos)) {
                                player.t_action = 'a';
                                player.t_no_move = 1 + movement(&player);
                            }
                            else
                                after = FALSE;
                        }
                        else {
                            affect();
                            player.t_action = A_NIL;
                        }
                    when 'A' : choose_qst();  
                    when 'F' : /* frighten a monster */
                        if (player.t_action == A_NIL) {
                            player.t_action = 'F';
                            player.t_no_move = 2*movement(&player);
                        }
                        else {
                after = FALSE;
                            player.t_action = A_NIL;
                            fright();
                        }
                    when 'g' : /* Give command: give slime-molds to monsters */
                        if (player.t_action == A_NIL) {
                            player.t_action = 'g';
                            player.t_no_move = 2*movement(&player);
                        }
                        else {
                after = FALSE;
                            player.t_action = A_NIL;
                            give();
                        }
                    when 'G' :
                        if (player.t_action == A_NIL) {
                            player.t_action = 'G';
                            player.t_no_move = movement(&player);
                        }
                        else {
                            player.t_action = A_NIL;
                            gsense();
                        }
                    when 'i' : after = FALSE; inventory(pack, ALL);
                    when 'I' : after = FALSE; picky_inven();
                    when 'm' : nameitem((struct linked_list *)NULL, TRUE);
                    when 'o' : option();
                    when 'O' : msg("Charactor type: %s    Quest item: %s", char_class[char_type].name, rel_magic[quest_item].mi_name);
                    when ',' :
                    case 'P' :
                        if (levtype != POSTLEV) {
                            /* We charge 2 movement units per item */
                            player.t_no_move =
                                2 * grab(hero.y, hero.x) * movement(&player);
                        }
                        else {
                            /* Let's quote the wise guy a price */
                            buy_it();
                            after = FALSE;
                        }
                    when 'Q' : after = FALSE; quit(0);
                    when 'S' : 
                        after = FALSE;
                        if (save_game())
                            exit_game(EXIT_CLS | EXIT_ENDWIN);
                    when 'v' : after = FALSE;
                               msg("Advanced xrogue, Version %s  ", release);
                    when 'X' :  /* trap sense */
            after = FALSE;
                        if (player.t_action == A_NIL) {
                            player.t_action = 'X';
                            player.t_no_move = movement(&player);
                        }
                        else {
                            xsense();
                            player.t_action = A_NIL;
                        }
                    when '.' :
                        player.t_no_move = movement(&player);  /* Rest */
                        player.t_action = A_NIL;
                    when ' ' : after = FALSE;   /* Do Nothing */
                    when '>' : after = FALSE; d_level();
                    when '<' : after = FALSE; u_level();
                    when '=' : after = FALSE; display();
                    when '?' : after = FALSE; help();

            /* no character descriptions yet until updated (help.c) */
            /* when '\\' : after = FALSE; ident_hero(); */
            when '\\' : msg("Charon (the Boatman) looks at you... ");

                    when '/' : after = FALSE; identify(NULL);
                    when C_COUNT : count_gold();
                    when C_DIP : dip_it();
                    when C_DROP : player.t_action = C_DROP; 
                                  drop((struct linked_list *)NULL);
                    when C_EAT : eat();
                    when C_QUAFF : quaff(-1, NULL, NULL, TRUE);
                    when C_READ : read_scroll(-1, NULL, TRUE);
                    when C_SETTRAP : set_trap(&player, hero.y, hero.x);
                    when C_SEARCH :
                        if (player.t_action == A_NIL) {
                            player.t_action = C_SEARCH;
                            player.t_no_move = 2 + movement(&player);
                        }
                        else {
                            search(FALSE, FALSE);
                            player.t_action = A_NIL;
                        }
                    when C_TAKEOFF : take_off();
                    when C_USE : use_mm(-1);
                    when C_WEAR : wear();
                    when C_WIELD : wield();
                    when C_ZAP : if (!player_zap(NULL, FALSE)) after=FALSE;
                    when C_CAST : cast();
                    when C_CHANT : chant();
                    when C_PRAY : pray();
                    when CTRL('B') : msg("Current score: %d",
                    pstats.s_exp + (long) purse);
                    when CTRL('E') : msg("Current food level: %d(2000)",
                    food_left);
                    when CTRL('L') : after = FALSE; clearok(curscr, TRUE);
                                    touchwin(cw);
                    when CTRL('N') : nameit();
                    when CTRL('O') : after = FALSE; opt_player();
                    when CTRL('R') : after = FALSE; msg(huh);
                    when CTRL('T') :
                        if (player.t_action == A_NIL) {
                            if (get_dir(&player.t_newpos)) {
                                player.t_action = CTRL('T');
                                player.t_no_move = 2 * movement(&player);
                            }
                            else
                                after = FALSE;
                        }
                        else {
                            steal();
                            player.t_action = A_NIL;
                        }
                    when ESC :  /* Escape */
                        door_stop = FALSE;
                        count = 0;
                        after = FALSE;
                    when '#':
                        if (levtype == POSTLEV)         /* buy something */
                            buy_it();
                        after = FALSE;
                    when '$':
                        if (levtype == POSTLEV)         /* price something */
                            price_it();
                        after = FALSE;
                    when '%':
                        if (levtype == POSTLEV)         /* sell something */
                            sell_it();
                        after = FALSE;
            when '+':   /* instant karma! */
            switch (rnd(100)) {
                case 0:  msg("You waste some time. ");
                when 5:  msg("An oak tree in the garden. ");
                when 10: msg("Character is what you become in the dark. ");
                when 15: msg("May you live all the days of your life. ");
                when 20: msg("A hero is no braver than an ordinary man, but he is brave five minutes longer. ");
                when 25: msg("Get down! ");
                when 30: msg("Go back to sleep. ");
                when 35: msg("Be here now. ");
                when 40: msg("Choose the rock that feels right to you. ");
                when 45: msg("Wait... ");
                when 50: msg("You take a break (yawn)... ");
                when 55: msg("Without danger there is no pleasure. ");
                when 60: msg("Define meaningless? ");
                when 65: msg("Don't push your luck! ");
                when 70: msg("Gung ho. ");
                when 75: msg("You are inside a computer. ");
                when 80: msg("Directive is now required... ");
                when 85: msg("Charon (the Boatman) awaits you... ");
                when 95: msg(nothing);
                otherwise: msg("");
            }
            after = FALSE;
                    when CTRL('P') :
                        after = FALSE;
                        if (wizard)
                        {
                            wizard = FALSE;
                            trader = 0;
                            msg("Not wizard any more");
                        }
                        else
                        {
                            if (waswizard || passwd())
                            {
                                msg("Welcome, O Mighty Wizard! ");
                                wizard = waswizard = TRUE;
                            }
                            else
                                msg("Sorry");
                        }

                    otherwise :
                        after = FALSE;
                        if (wizard) switch (ch) {
                            case 'M' : create_obj(TRUE, 0, 0);
                            when 'V' : msg("vlevel = %d  turns = %d",
                                           vlevel, turns);
                            when CTRL('A') : activity();
                            when CTRL('C') : do_teleport();
                            when CTRL('D') : level++;
                                           take_with();
                                           new_level(NORMLEV);
                            when CTRL('F') : overlay(stdscr,cw);
                            when CTRL('G') :
                            {
                                item=get_item(pack,"charge",STICK,FALSE,FALSE);
                                if (item != NULL) {
                                    (OBJPTR(item))->o_charges=10000;
                                }
                            }
                            when CTRL('H') :
                            {
                                register int i, j;
                                register struct object *obj;

                                for (i = 0; i < 9; i++)
                                    raise_level();
                                /*
                                 * Give the rogue a sword 
                                 */
                                if (cur_weapon==NULL || cur_weapon->o_type !=
                                    RELIC) {
                                if (player.t_ctype == C_THIEF   ||
                                    player.t_ctype == C_ASSASSIN ||
                                    player.t_ctype == C_MONK)
                                      item = spec_item(WEAPON, BASWORD, 20, 20);
                                else
                                      item = spec_item(WEAPON,TWOSWORD, 20, 20);
                                    if (add_pack(item, TRUE))
                                    {
                                        cur_weapon = OBJPTR(item);
                                        (OBJPTR(item))->o_flags |= (ISKNOW|ISPROT);
                                    }
                                    else
                                        o_discard(item);
                                /*
                                 * And his suit of armor
                                 */
                                if (player.t_ctype == C_THIEF   ||
                                    player.t_ctype == C_ASSASSIN ||
                                    player.t_ctype == C_MONK)
                                      j = PADDED_ARMOR;
                                else
                                      j = PLATE_ARMOR;
                                    item = spec_item(ARMOR, j, 20, 0);
                                    obj = OBJPTR(item);
                                    obj->o_flags |= (ISKNOW | ISPROT);
                                    obj->o_weight = armors[j].a_wght;
                                    if (add_pack(item, TRUE))
                                        cur_armor = obj;
                                    else
                                        o_discard(item);
                                }
                                purse += 20000;
                            }
                            when CTRL('I') : inventory(lvl_obj, ALL);
                            when CTRL('J') : teleport();
                            when CTRL('K') : whatis((struct linked_list *)NULL);
                            when CTRL('W') : wanderer();
                            when CTRL('X') : overlay(mw,cw);
                            when CTRL('Y') : msg("food left: %d\tfood level: %d", 
                                                    food_left, foodlev);
                            otherwise :
                                msg("Illegal wizard command '%s'.", unctrl(ch));
                                count = 0;
                        }
                        else
                        {
                            msg("Illegal command '%s'.", unctrl(ch));
                            count = 0;
                            after = FALSE;
                        }
                }

                /*
                 * If he ran into something to take, let him pick it up.
                 * unless it's a trading post
                 */
                if (auto_pickup && take != 0 && levtype != POSTLEV) {
                    /* get ready to pick it up */
                    player.t_action = A_PICKUP;
                    player.t_no_move += 2 * movement(&player);
                }
            }

            /* If he was fighting, let's stop (for now) */
            if (player.t_quiet < 0) player.t_quiet = 0;

            if (!running)
                door_stop = FALSE;

            if (after && segment >= 10) {
                /*
                 * Kick off the rest if the daemons and fuses
                 */

                /* 
                 * If player is infested, take off a hit point 
                 */
                if (on(player, HASINFEST)) {
            pstats.s_hpt -= infest_dam;
                    if (pstats.s_hpt == 50 || pstats.s_hpt == 25)
            msg("You feel yourself withering away... ");
                    if (pstats.s_hpt < 1) {
            msg("You die a festering mass.  --More--");
            wait_for(' ');
            pstats.s_hpt = -1;
            death(D_INFESTATION);
            }
                }

                /*
                 * The eye of Vecna is a constant drain on the player
                 */
                if (cur_relic[EYE_VECNA]) {
            pstats.s_hpt -= 1;
                    if (pstats.s_hpt == 50 || pstats.s_hpt == 25)
            msg("You feel Vecna's eye looking about. ");
                    if (pstats.s_hpt <= 10 && pstats.s_hpt >= 3)
            msg("Vecna's eye moves about very quickly. ");
                    if (pstats.s_hpt < 1) {
            msg("Vecna's curse is upon you!  --More--");
            wait_for(' ');
            pstats.s_hpt = -1;
            death(D_RELIC);
            }
                }

                /* 
                 * if player has body rot then take off three hits 
                 */
                if (on(player, DOROT)) {
             pstats.s_hpt -= rnd(3)+1;
                     if (pstats.s_hpt == 50 || pstats.s_hpt == 25) 
             msg("Something really begins to stink and smell! ");
                     if (pstats.s_hpt < 1) {
             msg("You keel over with rot.  --More--");
             wait_for(' ');
             pstats.s_hpt = -1;
             death(D_ROT);
             }
                }
                do_daemons(AFTER);
                do_fuses(AFTER);
            }
        } while (after == FALSE);

        /* Make the monsters go */
        if (--monst_current <= 0)
            monst_current = monst_limit = runners(monst_limit);

        if (++segment > 10) segment = 1;
        reap(); /* bury all the dead monsters */
    }
}

/*
 * display
 *      tell the player what is at a certain coordinates assuming
 *      it can be seen.
 */
display()
{
    coord c;
    struct linked_list *item;
    struct thing *tp;
    int what;

    msg("What do you want to display (* for help)?");
    c = get_coordinates();
    mpos = 0;
    if (!cansee(c.y, c.x)) {
        msg("You can't see what is there.");
        return;
    }
    what = mvwinch(cw, c.y, c.x);
    if (isalpha(what)) {
        item = find_mons(c.y, c.x);
        tp = THINGPTR(item);
        msg("%s", monster_name(tp));
        return;
    }
    if ((item = find_obj(c.y, c.x)) != NULL) {
        msg("%s", inv_name(OBJPTR(item), FALSE));
        return;
    }
    identify(what);
}

/*
 * quit:
 *      Have player make certain, then exit.
 */

/*UNUSED*/
void
quit(sig)
int sig;
{
    register int oy, ox;

	NOOP(sig);

    /*
     * Reset the signal in case we got here via an interrupt
     */

    if ((VOID(*)())signal(SIGINT, quit) != (VOID(*)())quit)
        mpos = 0;

    getyx(cw, oy, ox);
    if (level < 1) {    /* if not down in the dungeon proper; exit the game */
        wclear(hw);
        wmove(hw, lines-1, 0);
        draw(hw);
        wmove(hw, 12, 30);
        wprintw(hw, "Good-bye!");
        draw(hw);
        exit_game(EXIT_ENDWIN);
    }
    msg("Really quit? <yes or no> ");   /* otherwise ask about quitting */
    draw(cw);
    prbuf[0] = '\0';
    if ((get_str(prbuf, msgw) == NORM) && strcmp(prbuf, "yes") == 0) {
        clear();
        move(lines-1, 0);
        draw(stdscr);
        score(pstats.s_exp + (long) purse, CHICKEN, 0);
        exit_game(EXIT_ENDWIN);
    }
    else {
        signal(SIGINT, quit);
        wmove(msgw, 0, 0);
        wclrtoeol(msgw);
        draw(msgw);
        status(FALSE);
        wmove(cw, oy, ox);
        draw(cw);
        mpos = 0;
        count = 0;
        running = FALSE;
    }
}

/*
 * bugkill:
 *      killed by a program bug instead of voluntarily.
 */

bugkill(sig)
int sig;
{
    signal(sig, quit);      /* If we get it again, give up */
    if (levtype == OUTSIDE) {
        msg("Oh no!  You walk right into a flying swarm of nasty little bugs!! ");
        msg("One of them penetrates your brain!!!  --More--");
    }
    else {
        msg("Charon (the Boatman) has finally come for you...  --More--");
    }
    wait_for(' ');
    pstats.s_hpt = -1;
    death(D_SIGNAL);    /* Killed by a bug */
}

/*
 * search:
 *      Player gropes about him to find hidden things.
 */

search(is_thief, door_chime)
register bool is_thief, door_chime;
{
    register int x, y;
    register char ch,   /* The trap or door character */
                 sch;   /* Trap or door character (as seen on screen) */
    register unsigned char mch;   /* Monster, if a monster is on the trap or door */
    register struct linked_list *item;
    register struct thing *mp; /* Status on surrounding monster */

    /*
     * Look all around the hero, if there is something hidden there,
     * give him a chance to find it.  If its found, display it.
     */
    if (on(player, ISBLIND))
        return;
    for (x = hero.x - 1; x <= hero.x + 1; x++)
        for (y = hero.y - 1; y <= hero.y + 1; y++)
        {
            if (y==hero.y && x==hero.x)
                continue;

            if (x < 0 || y < 0 || x >= cols || y >= lines)
                continue;

            /* Mch and ch will be the same unless there is a monster here */
            mch = winat(y, x);
            ch = mvwinch(stdscr, y, x);
            sch = mvwinch(cw, y, x);    /* What's on the screen */

            if (door_chime == FALSE && isatrap(ch)) {
                    register struct trap *tp;

                    /* Is there a monster on the trap? */
                    if (mch != ch && (item = find_mons(y, x)) != NULL) {
                        mp = THINGPTR(item);
                        if (sch == mch) sch = mp->t_oldch;
                    }
                    else mp = NULL;

                    /* 
                     * is this one found already?
                     */
                    if (isatrap(sch)) 
                        continue;       /* give him chance for other traps */
                    tp = trap_at(y, x);
                    /* 
                     * if the thief set it then don't display it.
                     * if its not a thief he has 50/50 shot
                     */
                    if((tp->tr_flags&ISTHIEFSET) || (!is_thief && rnd(100)>50))
                        continue;       /* give him chance for other traps */
                    tp->tr_flags |= ISFOUND;

                    /* Let's update the screen */
                    if (mp != NULL && mvwinch(cw, y, x) == mch)
                        mp->t_oldch = ch; /* Will change when monst moves */
                    else mvwaddch(cw, y, x, ch);

                    count = 0;
                    running = FALSE;

                    /* Stop what we were doing */
                    player.t_no_move = movement(&player);
                    player.t_action = A_NIL;
                    player.t_using = NULL;

                    if (fight_flush) flushinp();
                    msg(tr_name(tp->tr_type));
            }
            else if (ch == SECRETDOOR) {
                if (door_chime == TRUE || (!is_thief && rnd(100) < 20)) {
                    struct room *rp;
                    coord cp;

                    /* Is there a monster on the door? */
                    if (mch != ch && (item = find_mons(y, x)) != NULL) {
                        mp = THINGPTR(item);

                        /* Screen will change when monster moves */
                        if (sch == mch) mp->t_oldch = ch;
                    }
                    mvaddch(y, x, DOOR);
                    count = 0;
                    /*
                     * if its the entrance to a treasure room, wake it up
                     */
                    cp.y = y;
                    cp.x = x;
                    rp = roomin(&cp);
                    if (rp->r_flags & ISTREAS)
                        wake_room(rp);

                    /* Make sure we don't shoot into the room */
                    if (door_chime == FALSE) {
                        count = 0;
                        running = FALSE;

                        /* Stop what we were doing */
                        player.t_no_move = movement(&player);
                        player.t_action = A_NIL;
                        player.t_using = NULL;
                    }
                }
            }
        }
}

/*
 * d_level:
 *      He wants to go down a level
 */

d_level()
{
    bool no_phase=FALSE;
    char position = winat(hero.y, hero.x);
    int au;


    /* If we are on a trading post, go to a trading post level. */
    if (position == POST) {
        take_with();   /* Take charmed monsters with you while shopping */
        new_level(POSTLEV);
        return;
    }

    /* Dive for gold */
    if (position == POOL) {
        if (rnd(300) < 2) {
            msg("Oh no!!!  You drown in the pool!!!  --More--");
            pstats.s_hpt = -1;
            wait_for(' ');
            death(D_DROWN);
        }
        else if (rnd(125) < 25) {
            au = rnd(350) + (level * 10);
            msg("You dive under the water momentarily.. ");
            msg("You found %d gold pieces! ", au);
            purse = purse + au;
            return;
        }
        else return;  /* doesn't happen all of the time */
    }

    /* Going down traps is hazardous */
    switch (position) {
    case WORMHOLE:
    case TRAPDOOR:
    case MAZETRAP:
    case DARTTRAP:
    case SLEEPTRAP:
    case ARROWTRAP:
    case BEARTRAP:
    case TELTRAP:
        msg ("You find yourself in some sort of quicksand!? ");
        msg ("Hey!  There are rock maggots in here!! ");
        player.t_no_move += movement(&player) * FREEZETIME;
        player.t_action = A_FREEZE;
        msg("You can't move. ");                   /* spare monks */
        if (!ISWEARING(R_HEALTH) && player.t_ctype != C_MONK) {
            turn_on(player, DOROT);
            msg("You feel your skin starting to rot and peel away!! ");
            }
        return;
    }

    /* If we are at a top-level trading post, we probably can't go down */
    if (levtype == POSTLEV && level == 0 && position != STAIRS) {
        msg("I see no way down.");
        return;
    }

    if (winat(hero.y, hero.x) != STAIRS) {
        if (off(player, CANINWALL) ||   /* Must use stairs if can't phase */
            (levtype == OUTSIDE && rnd(100) < 90)) {
            msg("I see no way down.");
            return;
        }
    if (levtype == OUTSIDE) {
            level++;
            take_with();
            new_level(NORMLEV);
            if (no_phase) unphase();
            return;
        }

        /* Is there any dungeon left below? */
        if (level >= nfloors) {
            msg("There is only solid rock below.");
            return;
        }

        extinguish(unphase);    /* Using phase to go down gets rid of it */
        no_phase = TRUE;
    }

    /* Is this the bottom? */
    if (level >= nfloors) {
        msg("The stairway only goes up.");
        return;
    }

    level++;
    take_with();
    new_level(NORMLEV);
    if (no_phase) unphase();
}

/*
 * u_level:
 *      He wants to go up a level
 */

u_level()
{
    bool no_phase = FALSE;
    register struct linked_list *item;
    char position = winat(hero.y, hero.x);
    struct thing *tp;
    struct object *obj;

    /* You can go up into the outside if standing on top of a worm hole */
    if (position == WORMHOLE) {
            prev_max = 1000;
        level--;
        if (level <= 0) level = 1;
            msg("You find yourself in strange surroundings... ");
        if (wizard) addmsg("Going up through a worm hole. ");
            take_with();
            new_level(OUTSIDE);
            return;
   }

    if (winat(hero.y, hero.x) != STAIRS) {
        if (off(player, CANINWALL)) {   /* Must use stairs if can't phase */
            msg("I see no way up.");
            return;
        }

        extinguish(unphase);
        no_phase = TRUE;
    }

    if (position != STAIRS) return;

    if (level == 0 || levtype == OUTSIDE) {
        msg("The stairway only goes down.");
        return;
    }

    /*
     * does he have the item he was quested to get?
     */
    if (level == 1) {
        for (item = pack; item != NULL; item = next(item)) {
            obj = OBJPTR(item);
            if (obj->o_type == RELIC && obj->o_which == quest_item)
                total_winner();
        }
    }
    /*
     * check to see if he trapped a UNIQUE, If he did then put it back
     * in the monster table for next time
     */
    for (item = tlist; item != NULL; item = next(item)) {
        tp = THINGPTR(item);
        if (on(*tp, ISUNIQUE)) 
            monsters[tp->t_index].m_normal = TRUE;
    }
    t_free_list(tlist); /* Monsters that fell below are long gone! */

    if (levtype != POSTLEV)
    level--;
    if (level > 0) {
        take_with();
        new_level(NORMLEV);
    }
    else if (cur_max > level) {
        prev_max = 1000;    /* flag used in n_level.c */
    level--;
        if (level <= 0) level = 1;
        msg("You emerge into the %s. ", daytime ? "eerie light" : "dark night");
    if (wizard) msg("Going up: cur_max=%d level=%d. ", cur_max, level);
        take_with();
        new_level(OUTSIDE);     /* Leaving the dungeon for outside */
    return;
 }
    else {
        prev_max = 1;   /* flag used in n_level.c */
        level = -1;     /* Indicate that we are new to the outside */
        msg("You emerge into the %s. ", daytime ? "eerie light" : "dark night");
    if (wizard) msg("Going up: cur_max=%d level=%d. ", cur_max, level);
        take_with();
        new_level(OUTSIDE); 
    return;
    }

    if (no_phase) unphase();
}

/*
 * Let him escape for a while
 */

shell()
{
    /*
     * Set the terminal back to original mode
     */
    wclear(hw);
    wmove(hw, lines-1, 0);
    draw(hw);
    endwin();
    in_shell = TRUE;
    fflush(stdout);

	md_shellescape();
	
    printf(retstr);
    fflush(stdout);
    noecho();
    crmode();
    in_shell = FALSE;
    wait_for('\n');
	restscr(cw);
}

/*
 * see what we want to name -- an item or a monster.
 */
nameit()
{
    char answer;

    msg("Name monster or item (m or i)? ");
    answer = wgetch(cw);
    mpos = 0;

    while (answer != 'm' && answer != 'i' && answer != ESC) {
        mpos = 0;
        msg("Please specify m or i, for monster or item - ");
        answer = wgetch(cw);
    }

    switch (answer) {
        case 'm': namemonst();
        when 'i': nameitem((struct linked_list *)NULL, FALSE);
    }
}

/*
 * allow a user to call a potion, scroll, or ring something
 */

nameitem(item, mark)
struct linked_list *item;
bool mark;
{
    register struct object *obj;
    register char **guess = NULL, *elsewise = NULL;
    register bool *know;

    if (item == NULL) {
        if (mark) item = get_item(pack, "mark", ALL, FALSE, FALSE);
        else      item = get_item(pack, "name", CALLABLE, FALSE, FALSE);
        if (item == NULL) return;
    }
    /*
     * Make certain that it is somethings that we want to wear
     */
    obj = OBJPTR(item);
    switch (obj->o_type)
    {
        case RING:
            guess = r_guess;
            know = r_know;
            elsewise = (r_guess[obj->o_which] != NULL ?
                        r_guess[obj->o_which] : r_stones[obj->o_which]);
        when POTION:
            guess = p_guess;
            know = p_know;
            elsewise = (p_guess[obj->o_which] != NULL ?
                        p_guess[obj->o_which] : p_colors[obj->o_which]);
        when SCROLL:
            guess = s_guess;
            know = s_know;
            elsewise = (s_guess[obj->o_which] != NULL ?
                        s_guess[obj->o_which] : s_names[obj->o_which]);
        when STICK:
            guess = ws_guess;
            know = ws_know;
            elsewise = (ws_guess[obj->o_which] != NULL ?
                        ws_guess[obj->o_which] : ws_made[obj->o_which]);
        when MM:
            guess = m_guess;
            know = m_know;
            elsewise = (m_guess[obj->o_which] != NULL ?
                        m_guess[obj->o_which] : "nothing");
        otherwise:
            if (!mark) {
                msg("You can't call that anything.");
                return;
            }
            else know = (bool *) 0;
    }
    if ((obj->o_flags & ISPOST) || (know && know[obj->o_which]) && !mark) {
        msg("That has already been identified.");
        return;
    }
    if (mark) {
        if (obj->o_mark[0]) {
            addmsg(terse ? "M" : "Was m");
            msg("arked \"%s\"", obj->o_mark);
        }
        msg(terse ? "Mark it: " : "What do you want to mark it? ");
        prbuf[0] = '\0';
    }
    else {
        if (elsewise) {
            addmsg(terse ? "N" : "Was n");
            msg("amed \"%s\"", elsewise);
            strcpy(prbuf, elsewise);
        }
        else prbuf[0] = '\0';
        msg(terse ? "Name it: " : "What do you want to name it? ");
    }
    if (get_str(prbuf, msgw) == NORM) {
        if (mark) {
            strncpy((char *)obj->o_mark, prbuf, MARKLEN-1);
            obj->o_mark[MARKLEN-1] = '\0';
        }
        else if (prbuf[0] != '\0') {
            if (guess[obj->o_which] != NULL)
                free(guess[obj->o_which]);
            guess[obj->o_which] = new((unsigned int) strlen(prbuf) + 1);
            strcpy(guess[obj->o_which], prbuf);
        }
    }
}

/* Name a monster */

namemonst()
{
    register struct thing *tp;
    struct linked_list *item;
    coord c;

    /* Find the monster */
    msg("Choose the monster (* for help)");
    c = get_coordinates();

    /* Make sure we can see it and that it is a monster. */
    mpos = 0;
    if (!cansee(c.y, c.x)) {
        msg("You can't see what is there.");
        return;
    }
    
    if (isalpha(mvwinch(cw, c.y, c.x))) {
        item = find_mons(c.y, c.x);
        if (item != NULL) {
            tp = THINGPTR(item);
            if (tp->t_name == NULL)
                strcpy(prbuf, monsters[tp->t_index].m_name);
            else
                strcpy(prbuf, tp->t_name);

            addmsg(terse ? "N" : "Was n");
            msg("amed \"%s\"", prbuf);
            msg(terse ? "Name it: " : "What do you want to name it? ");

            if (get_str(prbuf, msgw) == NORM) {
                if (prbuf[0] != '\0') {
                    if (tp->t_name != NULL)
                        free(tp->t_name);
                    tp->t_name = new((unsigned int) strlen(prbuf) + 1);
                    strcpy(tp->t_name, prbuf);
                }
            }
            return;
        }
    }

    msg("There is no monster there to name.");
}

count_gold()
{
        if (player.t_action != C_COUNT) {
            msg("You take a break to count your money.. ");
            player.t_using = NULL;
            player.t_action = C_COUNT;  /* We are counting */
            if (purse > 500000) msg("This may take some time... 10, 20, 30...");
            player.t_no_move = (purse/75000 + 1) * movement(&player);
            return;
        }
        if (purse > 10000)
                msg("You have %ld pieces of gold. ", purse);
        else if (purse == 1)
                msg("You have 1 piece of gold. ");
        else
                msg("You have %ld gold pieces. ", purse);
        player.t_action = A_NIL;
}

/* 
 * Teleport somewhere, anywhere...
 */

do_teleport()
{
    int tlev;
    prbuf[0] = '\0';
    msg("To which level do you wish to teleport? ");
    if(get_str(prbuf,msgw) == NORM) {
        tlev = atoi(prbuf);
    if (quest_item == ALTERAN_CARD || wizard) {
            if (wizard && (tlev < 1 || tlev > LEVEL)) {  /* wizard */
                mpos = 0;
                msg("The power of teleportation does have its limitations. ");
            return;
            }
        else if (!wizard && (tlev < 10 || tlev > LEVEL)) {
                mpos = 0;
                msg("The power of teleportation does have its limitations. ");
            return;
            }
            else if (tlev >= LEVEL-100 && tlev < LEVEL-50) {
                levtype = OUTSIDE;
                level = LEVEL-100;
            prev_max = 1000; /* a flag for going outside */
            } 
            else if (tlev >= LEVEL-150 && tlev < LEVEL-100) {
                levtype = MAZELEV;
                level = LEVEL-150;
            } 
            else if (tlev >= LEVEL-200 && tlev < LEVEL-150) {
                levtype = POSTLEV;
                level = LEVEL-200;
            }
            else {
                levtype = NORMLEV;
                level = tlev;
            }
    }
    else if (tlev < 40 || tlev > 399) {  /* not quest item or wizard */
            mpos = 0;
            msg("The power of teleportation does have its limitations. ");
        return;
        }
    else {
        levtype = NORMLEV;
        level = tlev;
    }

    /* okay, now send him off */
    cur_max = level;  /* deepest he's been */
    new_level(levtype);
    }
}

