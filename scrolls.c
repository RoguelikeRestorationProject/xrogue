/*
    scrolls.c - Functions for dealing with scrolls

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
#include "rogue.h"

/*
 * let the hero get rid of some type of monster 
 */

genocide()
{
    register struct linked_list *ip;
    register struct thing *mp;
    register struct linked_list *nip;
                             /* cannot genocide any uniques */
    register int num_monst = NUMMONST-NUMUNIQUE-NUMDINOS;
    register int which_monst;

    which_monst = makemonster(FALSE, "wipe out");
    if (which_monst <= 0 || which_monst >= num_monst) {
        msg("");
        return;
    }

    /* Remove this monster from the present level */
    for (ip = mlist; ip; ip = nip) {
        mp = THINGPTR(ip);
        nip = next(ip);
        if (mp->t_index == which_monst) {
            killed(ip, FALSE, FALSE, TRUE);
        }
    }

    /* Remove from available monsters */
    monsters[which_monst].m_normal = FALSE;
    monsters[which_monst].m_wander = FALSE;
    mpos = 0;
    msg("You have wiped out the %s.", monsters[which_monst].m_name);
}

read_scroll(which, flag, is_scroll)
register int which;
int flag;
bool is_scroll;
{
    register struct object *obj = NULL, *nobj;
    register struct linked_list *item, *nitem;
    register int i,j;
    register unsigned char ch, nch;
    bool cursed, blessed;

    blessed = FALSE;
    cursed = FALSE;
    item = NULL;

    if (which < 0) {
        if (on(player, ISBLIND)) {
            msg("You can't see to read anything!");
            return;
        }
        if (on(player, ISINWALL)) {
            msg("You can't see the scroll while inside rock!");
            return;
        }

        /* This is a scroll or book. */
        if (player.t_action != C_READ) {
            int units;

            item = get_item(pack, "read", READABLE, FALSE, FALSE);

            /*
             * Make certain that it is somethings that we want to read
             */
            if (item == NULL)
                return;

            /* How long does it take to read? */
            units = usage_time(item);
            if (units < 0) return;

            player.t_using = item;      /* Remember what it is */
            player.t_no_move = units * movement(&player);
            if ((OBJPTR(item))->o_type == SCROLL) player.t_action = C_READ;
            else player.t_action = C_USE;
            return;
        }

        /* We have waited our time, let's quaff the potion */
        item = player.t_using;
        player.t_using = NULL;
        player.t_action = A_NIL;

        obj = OBJPTR(item);
        /* remove it from the pack */
        inpack--;
        detach(pack, item);

        msg("As you read the scroll, it vanishes.");
        cursed = obj->o_flags & ISCURSED;
        blessed = obj->o_flags & ISBLESSED;

        which = obj->o_which;
    }
    else {
        cursed = flag & ISCURSED;
        blessed = flag & ISBLESSED;
    }

    switch (which) {
        case S_CONFUSE: /* Scroll of monster confusion. Give him that power. */
    {
        register char *str;

        switch (rnd(5)) {
        case 0:
            str = "glow red";
        when 1:
            str = "vibrate";
        when 2:
            str = "glow blue";
        when 3:
            str = "radiate green";
        otherwise:
            str = "itch with a strange desire";
        }
            msg("Your hands begin to %s. ", str);
            turn_on(player, CANHUH);
    }
        when S_CURING:
            /*
             * A cure disease spell
             */
            if (on(player, HASINFEST) || 
                on(player, HASDISEASE)|| 
                on(player, DOROT)) {
                if (on(player, HASDISEASE)) {
                    extinguish(cure_disease);
                    cure_disease();
                }
                if (on(player, HASINFEST)) {
                    msg(terse ? "You feel yourself improving."
                              : "You begin to feel yourself improving.");
                    turn_off(player, HASINFEST);
                    infest_dam = 0;
                }
                if (on(player, DOROT)) {
                    msg("You feel your skin returning to normal.");
                    turn_off(player, DOROT);
                }
            }
            else {
                /* msg(nothing); */
                break;
            }
            if (is_scroll) s_know[S_CURING] = TRUE;
        when S_LIGHT:
            if (blue_light(blessed, cursed) && is_scroll)
                s_know[S_LIGHT] = TRUE;
        when S_HOLD:
            if (cursed) {
                /*
                 * This scroll aggravates all the monsters on the current
                 * level and sets them running towards the hero
                 */
                msg("You hear a high-pitched humming noise.");
                /* protect good charactors */
                if (player.t_ctype == C_PALADIN ||
                    player.t_ctype == C_RANGER  || player.t_ctype == C_MONK) {
                        msg("A chill runs up your spine! ");
                        aggravate(TRUE, FALSE);
                }
                else {
                    aggravate(TRUE, TRUE);
                }
            }
            else if (blessed) { /* Hold all monsters on level */
                if (mlist == NULL) msg(nothing);
                else {
                    register struct linked_list *mon;
                    register struct thing *th;

                    for (mon = mlist; mon != NULL; mon = next(mon)) {
                        th = THINGPTR(mon);
                        turn_off(*th, ISRUN);
                        turn_on(*th, ISHELD);
                        turn_off(*th, ISCHARMED);
                    }
                    if (levtype == OUTSIDE)
                        msg("A sudden peace comes over the land.. ");
                    else
                        msg("A sudden peace comes over the dungeon.. ");
                }
            }
            else {
                /*
                 * Hold monster scroll.  Stop all monsters within two spaces
                 * from chasing after the hero.
                 */
                    register int x,y;
                    register struct linked_list *mon;
                    bool gotone=FALSE;

                    for (x = hero.x-2; x <= hero.x+2; x++) {
                        for (y = hero.y-2; y <= hero.y+2; y++) {
                            if (y < 1 || x < 0 || y > lines - 3 || x > cols - 1)
                                continue;
                            if (isalpha(mvwinch(mw, y, x))) {
                                if ((mon = find_mons(y, x)) != NULL) {
                                    register struct thing *th;

                                    gotone = TRUE;
                                    th = THINGPTR(mon);
                                    turn_off(*th, ISRUN);
                                    turn_on(*th, ISHELD);
                                    turn_off(*th, ISCHARMED);
                                }
                            }
                        }
                    }
                    if (gotone) msg("A sudden peace surrounds you.");
                    else msg(nothing);
            }
        when S_SLEEP:
            /*
             * if cursed, you fall asleep
             */
            if (is_scroll) s_know[S_SLEEP] = TRUE;
            if (cursed) {
                if (ISWEARING(R_ALERT))
                    msg("You feel drowsy for a moment.");
                else {
                    msg("You fall asleep.");
                    player.t_no_move += movement(&player)*(4 + rnd(SLEEPTIME));
                    player.t_action = A_FREEZE;
                }
            }
            else {
                /*
                 * sleep monster scroll.  
                 * puts all monsters within 2 spaces asleep
                 */
                    register int x,y;
                    register struct linked_list *mon;
                    bool gotone=FALSE;

                    for (x = hero.x-2; x <= hero.x+2; x++) {
                        for (y = hero.y-2; y <= hero.y+2; y++) {
                            if (y < 1 || x < 0 || y > lines - 3 || x > cols - 1)
                                continue;
                            if (isalpha(mvwinch(mw, y, x))) {
                                if ((mon = find_mons(y, x)) != NULL) {
                                    register struct thing *th;

                                    th = THINGPTR(mon);
                                    if (on(*th, ISUNDEAD))
                                        continue;
                                    th->t_no_move += movement(th)*(SLEEPTIME+4);
                                    th->t_action = A_FREEZE;
                                    gotone = TRUE;
                                }
                            }
                        }
                    }
                    if (gotone) 
                        msg("The monster(s) around you seem to have fallen asleep!");
                    else 
                        msg(nothing);
            }
        when S_CREATE:
            /*
             * Create a monster
             * First look in a circle around him, next try his room
             * otherwise give up
             */
            creat_mons(&player, (short) 0, TRUE);
            light(&hero);
        when S_IDENT:
            /* 
             * if its blessed then identify everything in the pack
             */
            if (blessed) {
                msg("You feel more Knowledgeable!");
                idenpack();
            }
            else {
                /*
                 * Identify, let the rogue figure something out
                 */
                if (is_scroll && s_know[S_IDENT] != TRUE) {
                    msg("This scroll is an identify scroll");
                }
                whatis((struct linked_list *)NULL);
            }
            if (is_scroll) s_know[S_IDENT] = TRUE;
        when S_MAP:
            /*
             * Scroll of magic mapping.
             */
            if (blessed) {
                register int i;

                if (is_scroll && s_know[S_MAP] != TRUE)
                    s_know[S_MAP] = TRUE;
                  /* light rooms */
                for (i=0; i<MAXROOMS; i++){
                    rooms[i].r_flags &= ~ISDARK;
                }

                msg("This scroll has a very detailed map on it!  --More--");
                wait_for(' ');
                overwrite(stdscr, hw);
                overlay(stdscr, cw);    /* wizard CTRL(F) */
                overlay(mw, cw);        /* wizard CTRL(X) */
                draw(cw);
                goto map_jump;          /* skip over regular mapping routine */
            }
            if (is_scroll && s_know[S_MAP] != TRUE) {
                msg("Oh, now this scroll has a map on it.");
                s_know[S_MAP] = TRUE;
            }
            overwrite(stdscr, hw);
            /*
             * Take all the things we want to keep hidden out of the window
             */
            for (i = 1; i < lines-2; i++)
                for (j = 0; j < cols; j++)
                {
                    switch (nch = ch = mvwinch(hw, i, j))
                    {
                        case SECRETDOOR:
                            nch = secretdoor (i, j);
                            break;
                        case HORZWALL:
                        case VERTWALL:
                        case DOOR:
                        case PASSAGE:
                        case ' ':
                        case STAIRS:
                            if (mvwinch(mw, i, j) != ' ')
                            {
                                register struct thing *it;

                                it = THINGPTR(find_mons(i, j));
                                if (it && it->t_oldch == ' ')
                                    it->t_oldch = nch;
                            }
                            break;
                        default:
                            nch = ' ';
                    }
                    if (nch != ch)
                        waddch(hw, nch);
                }
            /*
             * Copy in what he has discovered
             */
            overlay(cw, hw);
            /*
             * And set up for display
             */
            overwrite(hw, cw);
            map_jump:           /* blessed map jump from above */
        when S_GFIND:
            /*
             * Scroll of gold detection
             */
            {
                int gtotal = 0;

                if (is_scroll) s_know[S_GFIND] = TRUE;
                wclear(hw);
                for (nitem = lvl_obj; nitem != NULL; nitem = next(nitem)) {
                    nobj = OBJPTR(nitem);
                    if (nobj->o_type == GOLD) {
                        gtotal += nobj->o_count;
                        mvwaddch(hw, nobj->o_pos.y, nobj->o_pos.x, GOLD);
                    }
                }
                for (nitem = mlist; nitem != NULL; nitem = next(nitem)) {
                    register struct linked_list *gitem;
                    register struct thing *th;

                    th = THINGPTR(nitem);
                    if (on(*th, NODETECT)) continue;
                    for(gitem = th->t_pack; gitem != NULL; gitem = next(gitem)){
                        nobj = OBJPTR(gitem);
                        if (nobj->o_type == GOLD) {
                            gtotal += nobj->o_count;
                            mvwaddch(hw, th->t_pos.y, th->t_pos.x, GOLD);
                        }
                    }
                }
                if (gtotal) {
                    rmmsg();
                    overlay(hw,cw);
                    draw(cw);
                    msg("You begin to feel greedy.  You sense gold!");
                    break;
                }
            }
            msg("You begin to feel a pull downward..");
        when S_TELEP:
            /*
             * Scroll of teleportation:
             * Make him disappear and reappear
             */
            if (cursed) {
                int old_max = cur_max;

                turns = (vlevel * NLEVMONS) * LEVEL;
                /* if (turns > 42000) turns = 42000; limit turns */
                debug ("vlevel = %d  turns = %d", vlevel, turns);

                level = rnd(201)+80;   /* cursed teleport range */

                msg("You are banished to the lower regions! ");
                new_level(NORMLEV);

                status(TRUE);
                mpos = 0;
                if (old_max == cur_max) { /* if he's been here make it harder */
                /* protect good charactors */
                if (player.t_ctype == C_PALADIN ||
                    player.t_ctype == C_RANGER  || player.t_ctype == C_MONK) {
                        aggravate(TRUE, FALSE);
                }
                else {
                    aggravate(TRUE, TRUE);
                }
                }
            }
            else if (blessed) {
                int     old_level, 
                        much = rnd(6) - 7;

                old_level = level;
                if (much != 0) {
                    level += much;
                    if (level < 1)
                        level = 1;
                    mpos = 0;
                    cur_max = level;
                    turns += much*LEVEL;
                    if (turns < 0)
                        turns = 0;
                    new_level(NORMLEV);         /* change levels */
                    if (level == old_level)
                        status(TRUE);
                    msg("You are whisked away to another region!");
                }
            }
            else {
                teleport();
            }
            if (is_scroll) s_know[S_TELEP] = TRUE;
        when S_SCARE:
            /*
             * A monster will refuse to step on a scare monster scroll
             * if it is dropped.  Thus reading it is a mistake and produces
             * laughter at the poor rogue's boo boo.
             */
            msg("You hear maniacal laughter in the distance.");
        when S_REMOVE:
            if (cursed) { /* curse all player's possessions */
                for (nitem = pack; nitem != NULL; nitem = next(nitem)) {
                    nobj = OBJPTR(nitem);
                    if (nobj->o_flags & ISBLESSED) 
                        nobj->o_flags &= ~ISBLESSED;
                    else 
                        nobj->o_flags |= ISCURSED;
                }
                msg("The smell of fire and brimstone fills the air!");
        /* return; leaks item, go through end of function */
            }
            else if (blessed) {
                for (nitem = pack; nitem != NULL; nitem = next(nitem)) {
                    nobj = OBJPTR(nitem);
                    nobj->o_flags &= ~ISCURSED;
                }
                msg("Your pack glistens brightly!");
                do_panic(NULL);         /* this startles them */
        /* return; leaks item, go through end of function */
            }
            else {
                nitem = get_item(pack, "remove the curse on",ALL,FALSE,FALSE);
                if (nitem != NULL) {
                    nobj = OBJPTR(nitem);
                    nobj->o_flags &= ~ISCURSED;
                    msg("Removed the curse from %s",inv_name(nobj,TRUE));
                }
            }
            if (is_scroll) s_know[S_REMOVE] = TRUE;
        when S_PETRIFY:
            switch (mvinch(hero.y, hero.x)) {
                case WORMHOLE:
                case TRAPDOOR:
                case DARTTRAP:
                case TELTRAP:
                case ARROWTRAP:
                case SLEEPTRAP:
                case BEARTRAP:
                    {
                        register int i;

                        /* Find the right trap */
                        for (i=0; i<ntraps && !ce(traps[i].tr_pos, hero); i++);
                        ntraps--;

                        if (!ce(traps[i].tr_pos, hero))
                            msg("What a strange trap!");
                        else {
                            while (i < ntraps) {
                                traps[i] = traps[i + 1];
                                i++;
                            }
                        }
                    }
                    goto pet_message;
                case DOOR:
                case SECRETDOOR:
                case FLOOR:
                case PASSAGE:
pet_message:        msg("The dungeon begins to rumble and shake!");
                    addch(WALL);

                    /* If the player is phased, unphase him */
                    if (on(player, CANINWALL)) {
                        extinguish(unphase);
                        turn_off(player, CANINWALL);
                        msg("The dizzy feeling leaves you.");
                    }

                    /* Mark the player as in a wall */
                    turn_on(player, ISINWALL);
                    break;
                default:
                    msg(nothing);
            }
        when S_GENOCIDE:
            msg("You have been granted the boon of genocide!  --More--");
            wait_for(' ');
            msg("");
            genocide();
            if (is_scroll) s_know[S_GENOCIDE] = TRUE;
        when S_PROTECT: {
            struct linked_list *ll;
            struct object *lb;
            bool did_it = FALSE;
            msg("You are granted the power of protection.");
            if ((ll=get_item(pack,"protect",PROTECTABLE,FALSE,FALSE)) != NULL) {
                lb = OBJPTR(ll);
                mpos = 0;
                if (cursed) {
                    switch(lb->o_type) {        /* ruin it completely */
                        case RING: if (lb->o_ac > 0) {
                                    if (is_current(lb)) {
                                        switch (lb->o_which) {
                                            case R_ADDWISDOM:
                                                pstats.s_wisdom -= lb->o_ac;
                                            when R_ADDINTEL:  
                                                pstats.s_intel -= lb->o_ac;
                                            when R_ADDSTR:
                                                pstats.s_str -= lb->o_ac;
                                            when R_ADDHIT:
                                                pstats.s_dext -= lb->o_ac;
                                        }
                                    }
                                    did_it = TRUE;
                                        lb->o_ac = 0;
                                }
                        when ARMOR: if (lb->o_ac > 10) {
                                        did_it = TRUE;
                                        lb->o_ac = 10;
                                    }
                        when STICK: if (lb->o_charges > 0) {
                                        did_it = TRUE;
                                        lb->o_charges = 0;
                                    }
                        when WEAPON:if (lb->o_hplus > 0) {
                                        did_it = TRUE;
                                        lb->o_hplus = 0;
                                    }
                                    if (lb->o_dplus > 0) {
                                        did_it = TRUE;
                                        lb->o_dplus = 0;
                                    }
                    }
                    if (lb->o_flags & ISPROT) {
                        did_it = TRUE;
                        lb->o_flags &= ~ISPROT;
                    }
                    if (lb->o_flags & ISBLESSED) {
                        did_it = TRUE;
                        lb->o_flags &= ~ISBLESSED;
                    }
                    if (did_it)
                        msg("Your %s glows red for a moment",inv_name(lb,TRUE));
                    else {
                        msg(nothing);
                        break;
                    }
                }
                else  {
                    lb->o_flags |= ISPROT;
                    msg("Protected %s.",inv_name(lb,TRUE));
                }
            }
            if (is_scroll) s_know[S_PROTECT] = TRUE;
        }
        when S_MAKEIT:
            msg("You have been endowed with the power of creation!");
            if (is_scroll) s_know[S_MAKEIT] = TRUE;
            create_obj(TRUE, 0, 0);
        when S_ALLENCH: {
            struct linked_list *ll;
            struct object *lb;
            int howmuch, flags;
            if (is_scroll && s_know[S_ALLENCH] == FALSE) {
                msg("You are granted the power of enchantment.");
                msg("You may enchant anything (weapon, ring, armor, scroll, potion)");
            }
            if ((ll = get_item(pack, "enchant", ALL, FALSE, FALSE)) != NULL) {
                lb = OBJPTR(ll);
                lb->o_flags &= ~ISCURSED;
                if (blessed) {
                    howmuch = 2;
                    flags = ISBLESSED;
                }
                else if (cursed) {
                    howmuch = -1;
                    flags = ISCURSED;
                }
                else {
                    howmuch = 1;
                    flags = ISBLESSED;
                }
                switch(lb->o_type) {
                    case RING:
                        if (lb->o_ac + howmuch > MAXENCHANT) {
                            msg("The enchantment doesn't seem to work!");
                            break;
                        }
                        lb->o_ac += howmuch;
                        if (lb==cur_ring[LEFT_1]  || lb==cur_ring[LEFT_2]  ||
                            lb==cur_ring[LEFT_3]  || lb==cur_ring[LEFT_4]  ||
                            lb==cur_ring[RIGHT_1] || lb==cur_ring[RIGHT_2] ||
                lb==cur_ring[RIGHT_3] || lb==cur_ring[RIGHT_4]) {
                            switch (lb->o_which) {
                                case R_ADDWISDOM: pstats.s_wisdom += howmuch;
                                when R_ADDINTEL:  pstats.s_intel  += howmuch;
                                when R_ADDSTR:    pstats.s_str    += howmuch;
                                when R_ADDHIT:    pstats.s_dext   += howmuch;
                            }
                        }
                        msg("Enchanted %s.",inv_name(lb,TRUE));
                    when ARMOR:
                        if ((armors[lb->o_which].a_class - lb->o_ac) +
                            howmuch > MAXENCHANT) {
                            msg("The enchantment doesn't seem to work!");
                            break;
                        }
                        else
                            lb->o_ac -= howmuch;
                        msg("Enchanted %s.",inv_name(lb,TRUE));
                    when STICK:
                        lb->o_charges += rnd(16)+10;
                        if (lb->o_charges < 0)
                            lb->o_charges = 0;
                        if (EQUAL(ws_type[lb->o_which], "staff")) {
                            if (lb->o_charges > 200) 
                                lb->o_charges = 200;
                        }
                        else {
                            if (lb->o_charges > 200)  /* make em the same */
                                lb->o_charges = 200;
                        }
                        msg("Enchanted %s.",inv_name(lb,TRUE));
                    when WEAPON:
                        if(lb->o_hplus+lb->o_dplus+howmuch > MAXENCHANT * 2){
                            msg("The enchantment doesn't seem to work!");
                            break;
                        }
                        if (rnd(100) < 50)
                            lb->o_hplus += howmuch;
                        else
                            lb->o_dplus += howmuch;
                        msg("Enchanted %s.",inv_name(lb,TRUE));
                    when MM:
                        switch (lb->o_which) {
                            case MM_BRACERS:
                                if (lb->o_ac + howmuch > MAXENCHANT) {
                                   msg("The enchantment doesn't seem to work!");
                                   break;
                                }
                                else lb->o_ac += howmuch;
                                msg("Enchanted %s.",inv_name(lb,TRUE));
                            when MM_PROTECT:
                                if (lb->o_ac + howmuch > MAXENCHANT) {
                                   msg("The enchantment doesn't seem to work!");
                                   break;
                                }
                                else lb->o_ac += howmuch;
                                msg("Enchanted %s.",inv_name(lb,TRUE));
                        }
                        lb->o_flags |= flags;
                    when POTION:
                    case SCROLL:
                    default:
                        lb->o_flags |= flags;
                    msg("Enchanted %s.",inv_name(lb,TRUE));
                }
            }
            if (is_scroll) s_know[S_ALLENCH] = TRUE;

            /* If gotten here via prayer or Ankh, dock his wisdom. */
            if (!is_scroll) {
                pstats.s_wisdom--;
        if (pstats.s_wisdom < 3) pstats.s_wisdom = 3;
                msg("You feel a drain on your system. ");
            }
        }
        when S_FINDTRAPS:
            for (i=0; i<ntraps; i++) {
                if (!(traps[i].tr_flags & ISFOUND)) {
                    traps[i].tr_flags |= ISFOUND;
                    if (cansee(traps[i].tr_pos.y, traps[i].tr_pos.x))
                        mvwaddch(cw,traps[i].tr_pos.y,traps[i].tr_pos.x,
                                 traps[i].tr_type);
                }
            }
            if (ntraps > 0) {
                msg("You sense the presence of traps.");
                if (is_scroll) s_know[S_FINDTRAPS] = TRUE;
            }
            else
                msg(nothing);

        when S_RUNES:
        {
                register struct linked_list *sitem;

                msg("The scroll explodes in a ball of fire!");
                if (on(player, NOFIRE)) {
                        msg("The fire does not seem to affect you.");
                        break;
                }
                explode(&player);
                if (pstats.s_hpt <= 0) {
            pstats.s_hpt = -1;
                    death(D_SCROLL);
        }
                for (sitem = pack; sitem != NULL; sitem = nitem) {
                    nitem = next(sitem); /* in case we delete it */
                    nobj = OBJPTR(sitem);
                    /*
                     * check for loss of all scrolls and give them
                     * a save versus fire
                     */
                    if (nobj->o_type == SCROLL && roll(1,20) < 17) {
                        msg("%s burns up!", inv_name(nobj, TRUE));
                        inpack--;
                        detach(pack, sitem);
                        o_discard(sitem);
                    }
                }
        }

        when S_CHARM:
        {
            bool spots[9];
            int x, y, spot, count, numcharmed, something, bonus;
            struct linked_list *item;
            register struct thing *tp;

            /* Initialize the places where we look around us */
            for (i=0; i<9; i++) spots[i] = FALSE;
            count = 0;  /* No spots tried yet */
            numcharmed = 0;     /* Nobody charmed yet */
            something = 0;      /* Nothing has been seen yet */
            bonus = 0;          /* no bonus yet */

            /* Now look around us randomly for a charmee */
            while (count < 9) {
                do {
                    spot = rnd(9);
                } while (spots[spot] == TRUE);

                /* We found a place */
                count++;
                spots[spot] = TRUE;
                y = hero.y - 1 + (spot / 3);
                x = hero.x - 1 + (spot % 3);

                /* Be sure to stay on the board! */
                if (x < 0 || x >= cols || (y < 1) || (y >= lines - 2))
                        continue;

                /* Is there a monster here? */
                if (!isalpha(mvwinch(mw, y, x))) continue;

                /* What kind is it? */
                item = find_mons(y, x);
                if (item == NULL) continue;

                tp = THINGPTR(item);
                if (on(*tp,ISCHARMED) || on(*tp,ISUNIQUE) || on(*tp,ISUNDEAD)) 
                    continue;

                /* Will the monster be charmed? */
                if (blessed) bonus -= 3;
                bonus -= (pstats.s_charisma - 12) / 3;
                if ((player.t_ctype==C_PALADIN || player.t_ctype==C_RANGER) &&
                    off(*tp, ISMEAN))
                        bonus -= 3;
                if (save(VS_MAGIC, tp, bonus)) continue;

                /* We got him! */
                numcharmed++;

                /* Let the player know (maybe) */
                if ((off(*tp, ISINVIS)     || on(player, CANSEE)) &&
                    (off(*tp, ISSHADOW)    || on(player, CANSEE)) &&
                    cansee(y, x)) {
                        if (on(*tp, CANSURPRISE)) {
                            turn_off(*tp, CANSURPRISE);
                            msg("What the !? ");
                        }
                        msg("The eyes of %s glaze over!",
                            prname(monster_name(tp), FALSE));
                        something++;
                }

                /* Charm him and turn off any side effects */
                turn_on(*tp, ISCHARMED);
                runto(tp, &hero);
                tp->t_action = A_NIL;

                /* If monster was suffocating us, stop it */
                if (on(*tp, DIDSUFFOCATE)) {
                    turn_off(*tp, DIDSUFFOCATE);
                    extinguish(suffocate);
                }

                /* If monster held us, stop it */
                if (on(*tp, DIDHOLD) && (--hold_count == 0))
                        turn_off(player, ISHELD);
                turn_off(*tp, DIDHOLD);

                /* If frightened of this monster, stop */
                if (on(player, ISFLEE) &&
                    player.t_dest == &tp->t_pos) turn_off(player, ISFLEE);

                if ((blessed && numcharmed >= 5) || numcharmed > 0) break;
            }

            if (something == 0) msg(nothing);
        }

        otherwise:
            msg("What a puzzling scroll!");
            if (item != NULL) o_discard(item);
            return;
    }
    look(TRUE, FALSE);  /* put the result of the scroll on the screen */
    status(FALSE);
    if (is_scroll && item && s_know[which] && s_guess[which])
    {
        free(s_guess[which]);
        s_guess[which] = NULL;
    }
    else if (is_scroll                          && 
             !s_know[which]                     && 
             item                               &&
             askme                              &&
             (obj->o_flags & ISKNOW) == 0       &&
             (obj->o_flags & ISPOST) == 0       &&
             s_guess[which] == NULL) {
        nameitem(item, FALSE);
    }
    if (item != NULL) o_discard(item);
    updpack(TRUE, &player);
}

