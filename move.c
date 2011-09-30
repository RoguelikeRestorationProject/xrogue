/*
    move.c - Hero movement commands

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
 * Used to hold the new hero position
 */

coord move_nh;

static char Moves[3][3] = {
    { 'y', 'k', 'u' },
    { 'h', '.', 'l' },
    { 'b', 'j', 'n' }
};

/*
 * be_trapped:
 *      The guy stepped on a trap.... Make him pay.
 */

be_trapped(th, tc)
register struct thing *th;
register coord *tc;
{
    register struct trap *tp;
    register char ch, *mname = NULL;
    register bool is_player = (th == &player),
                  can_see;
    register struct linked_list *mitem = NULL;
    register struct thing *mp;


    /* Can the player see the creature? */
    can_see = cansee(tc->y, tc->x);
    can_see &= (is_player || !invisible(th));

    tp = trap_at(tc->y, tc->x);
    /*
     * if he's wearing boots of elvenkind, he won't set off the trap
     * unless its a magic pool (they're not really traps)
     */
    if (is_player                                       &&
        cur_misc[WEAR_BOOTS] != NULL                    &&
        cur_misc[WEAR_BOOTS]->o_which == MM_ELF_BOOTS   &&
        tp->tr_type != POOL)
            return '\0';

    /*
     * if the creature is flying then it won't set off the trap
     */
     if (on(*th, ISFLY))
        return '\0';

    tp->tr_flags |= ISFOUND;

    if (!is_player) {
        mitem = find_mons(th->t_pos.y, th->t_pos.x);
        mname = monster_name(th);
    }
    else {
        count = running = FALSE;
        mvwaddch(cw, tp->tr_pos.y, tp->tr_pos.x, tp->tr_type);
    }
    switch (ch = tp->tr_type) {
        case TRAPDOOR:
            if (is_player) {
                level++;
                pstats.s_hpt -= roll(1, 10);
                msg("You fell through a trap! ");
                if (pstats.s_hpt < 1) {
            pstats.s_hpt = -1;
            death(D_FALL);
        }
        wclear(cw);
        wclear(mw);
                new_level(NORMLEV);
            }
            else {
                if (can_see) msg("%s fell into a trap!", prname(mname, TRUE));

                /* 
                 * See if the fall killed the monster 
                 * don't let a UNIQUE die since it might have an artifact
                 * that we need
                 */
                if (off(*th,ISUNIQUE) && (th->t_stats.s_hpt-=roll(1,10)) <= 0){
                    killed(mitem, FALSE, FALSE, FALSE);
                }
                else {  /* Just move monster to next level */
                    check_residue(th);

                    /* Erase the monster from the old position */
                    if (isalpha(mvwinch(cw, th->t_pos.y, th->t_pos.x)))
                        mvwaddch(cw, th->t_pos.y, th->t_pos.x, th->t_oldch);
                    mvwaddch(mw, th->t_pos.y, th->t_pos.x, ' ');

                    /* let him summon on next lvl */
                    if (on (*th, HASSUMMONED)) {
                            turn_off(*th, HASSUMMONED); 
                            turn_on(*th, CANSUMMON);
                    }
                    turn_on(*th,ISELSEWHERE);
                    detach(mlist, mitem);
                    attach(tlist, mitem);       /* remember him next level */

                    /* Make sure that no one is still chasing us */
                    for (mitem = mlist; mitem != NULL; mitem = next(mitem)) {
                        mp = THINGPTR(mitem);
                        if (mp->t_dest == &th->t_pos) {
                            mp->t_dest = &hero;
                            mp->t_wasshot = FALSE;
                            turn_off(*mp, ISFLEE);      /* Don't run away! */
                        }
                    }

                    /* Make sure we were not chasing a monster here */
                    th->t_dest = &hero;
                    if (on(*th, ISFRIENDLY)) turn_off(*th, ISFLEE);
                }
            }
        /* worm hole trap to OUTSIDE */
        when WORMHOLE:
            if (is_player) {
                prev_max = 1000;    /* flag used in n_level.c */
        level++;
                msg("You suddenly find yourself in strange surroundings! ");
                pstats.s_hpt -= roll(1, 10);
                if (pstats.s_hpt < 1) {
            pstats.s_hpt = -1;
            death(D_FALL);
        }
                new_level(OUTSIDE);
        return(ch);
            }
            else {
                if (can_see) msg("%s fell into the worm hole! ", prname(mname, TRUE));

                /* 
                 * See if the fall killed the monster 
                 * don't let a UNIQUE die since it might have an artifact
                 * that we need
                 */
                if (off(*th,ISUNIQUE) && (th->t_stats.s_hpt-=roll(1,10)) <= 0){
                    killed(mitem, FALSE, FALSE, FALSE);
                }
                else {  /* Just move monster to next level */
                    check_residue(th);

                    /* Erase the monster from the old position */
                    if (isalpha(mvwinch(cw, th->t_pos.y, th->t_pos.x)))
                        mvwaddch(cw, th->t_pos.y, th->t_pos.x, th->t_oldch);
                    mvwaddch(mw, th->t_pos.y, th->t_pos.x, ' ');

                    /* let him summon on next lvl */
                    if (on (*th, HASSUMMONED)) {
                            turn_off(*th, HASSUMMONED); 
                            turn_on(*th, CANSUMMON);
                    }

                    turn_on(*th,ISELSEWHERE);
                    detach(mlist, mitem);
                    attach(tlist, mitem);       /* remember him next level */

                    /* Make sure that no one is still chasing us */
                    for (mitem = mlist; mitem != NULL; mitem = next(mitem)) {
                        mp = THINGPTR(mitem);
                        if (mp->t_dest == &th->t_pos) {
                            mp->t_dest = &hero;
                            mp->t_wasshot = FALSE;
                            turn_off(*mp, ISFLEE);      /* Don't run away! */
                        }
                    }

                    /* Make sure we were not chasing a monster here */
                    th->t_dest = &hero;
                    if (on(*th, ISFRIENDLY)) turn_off(*th, ISFLEE);
                }
            }
        when BEARTRAP:
            if (is_stealth(th)) {
                if (is_player) msg("You pass a bear trap.");
                else if (can_see) msg("%s passes a bear trap.", 
                                      prname(mname, TRUE));
            }
            else {
                th->t_no_move += movement(&player) * BEARTIME;
                th->t_action = A_FREEZE;
                if (is_player) msg("You are caught in a bear trap.");
                else if (can_see) msg("%s is caught in a bear trap.",
                                        prname(mname, TRUE));
            }
        when SLEEPTRAP:
            if (is_player) {
                if (!ISWEARING(R_ALERT)) {
                    msg("A strange white mist envelops you.  You fall asleep. ");
                    player.t_no_move += movement(&player) * SLEEPTIME;
                    player.t_action = A_FREEZE;
                }
        else {
            msg("The white mist invigorates you. ");
        }
            }
            else {
                if (can_see) 
                    msg("A strange white mist envelops %s. ",
                        prname(mname, FALSE));
                if (on(*th, ISUNDEAD)) {
                    if (can_see) 
                        msg("The mist doesn't seem to affect %s.",
                           prname(mname, FALSE));
                }
                else {
                    th->t_no_move += movement(th) * SLEEPTIME;
                    th->t_action = A_FREEZE;
                }
            }
        when ARROWTRAP:
            if (swing(th->t_ctype, th->t_stats.s_lvl-1, th->t_stats.s_arm, 1))
            {
                if (is_player) {
                    msg("Oh no! An arrow shot you.");
                    if ((pstats.s_hpt -= roll(1, 8)) < 1) {
            pstats.s_hpt = -1;
                        msg("The arrow killed you.  --More--");
            wait_for(' ');
                        death(D_ARROW);
                    }
                }
                else {
                    if (can_see) 
                        msg("An arrow shot %s.", prname(mname, FALSE));
                    if ((th->t_stats.s_hpt -= roll(1, 8)) < 1) {
                        if (can_see) 
                            msg("The arrow killed %s.", prname(mname, FALSE));
                        killed(mitem, FALSE, FALSE, TRUE);
                    }
                }
            }
            else
            {
                register struct linked_list *item;
                register struct object *arrow;

                if (is_player) msg("An arrow shoots past you.");
                else if (can_see) 
                      msg("An arrow shoots by %s.", prname(mname, FALSE));
                item = new_item(sizeof *arrow);
                arrow = OBJPTR(item);
                arrow->o_type = WEAPON;
                arrow->contents = NULL;
                arrow->o_which = ARROW;
                arrow->o_hplus = rnd(7) - 1;
                arrow->o_dplus = rnd(7) - 1;
                init_weapon(arrow, ARROW);
                arrow->o_count = 1;
                arrow->o_pos = *tc;
                arrow->o_mark[0] = '\0';
                fall(item, FALSE);
            }
        when TELTRAP:
            if (is_player) teleport();
            else {
                register int rm;
                struct room *old_room;  /* old room of monster */

                /* 
                 * Erase the monster from the old position 
                 */
                if (isalpha(mvwinch(cw, th->t_pos.y, th->t_pos.x)))
                    mvwaddch(cw, th->t_pos.y, th->t_pos.x, th->t_oldch);
                mvwaddch(mw, th->t_pos.y, th->t_pos.x, ' ');
                /*
                 * check to see if room should go dark
                 */
                if (on(*th, HASFIRE)) {
                    old_room=roomin(&th->t_pos);
                    if (old_room != NULL) {
                        register struct linked_list *fire_item;

                        for (fire_item = old_room->r_fires; fire_item != NULL;
                             fire_item = next(fire_item)) {
                            if (THINGPTR(fire_item) == th) {
                                detach(old_room->r_fires, fire_item);
                                destroy_item(fire_item);

                                if (old_room->r_fires == NULL) {
                                    old_room->r_flags &= ~HASFIRE;
                                    if (can_see) light(&hero);
                                }
                            }
                        }
                    }
                }

                /* Get a new position */
                do {
                    rm = rnd_room();
                    rnd_pos(&rooms[rm], &th->t_pos);
                } until(winat(th->t_pos.y, th->t_pos.x) == FLOOR);

                /* Put it there */
                mvwaddch(mw, th->t_pos.y, th->t_pos.x, th->t_type);
                th->t_oldch = mvwinch(cw, th->t_pos.y, th->t_pos.x);
                /*
                 * check to see if room that creature appears in should
                 * light up
                 */
                if (on(*th, HASFIRE)) {
                    register struct linked_list *fire_item;

                    fire_item = creat_item();
                    ldata(fire_item) = (char *) th;
                    attach(rooms[rm].r_fires, fire_item);

                    rooms[rm].r_flags |= HASFIRE;
                    if(cansee(th->t_pos.y, th->t_pos.x) && 
                       next(rooms[rm].r_fires) == NULL)
                        light(&hero);
                }
                if (can_see) 
                    msg("%s seems to have disappeared!", prname(mname, TRUE));
            }
        when DARTTRAP:
            if (swing(th->t_ctype, th->t_stats.s_lvl+1, th->t_stats.s_arm, 1)) {
                if (is_player) {
                    msg("A small dart just hit you. ");
                    if ((pstats.s_hpt -= roll(1, 8)) < 1) {
            pstats.s_hpt = -1;
                        msg("The dart killed you.");
            wait_for(' ');
                        death(D_DART);
                    }

                    /* Now the poison */
                    if (!save(VS_POISON, &player, 0)) {
                        /* 75% chance it will do point damage - else strength */
                        if (rnd(100) < 75) {
                            pstats.s_hpt /= 2;
                            if (pstats.s_hpt < 1) {
                pstats.s_hpt = -1;
                death(D_POISON);
                }
                        }
                        else if (!ISWEARING(R_SUSABILITY))
                                chg_str(-1);
                    }
                }
                else {
                    if (can_see)
                        msg("A small dart stabs the %s. ",
                                prname(mname, FALSE));
                    if ((th->t_stats.s_hpt -= roll(1,8)) < 1) {
                        if (can_see) 
                            msg("The dart killed %s.", prname(mname, FALSE));
                        killed(mitem, FALSE, FALSE, TRUE);
                    }
                    if (!save(VS_POISON, th, 0)) {
                        th->t_stats.s_hpt /= 2 + level;
                        if (th->t_stats.s_hpt < 1) {
                            if (can_see) 
                                msg("The dart killed %s.", prname(mname,FALSE));
                            killed(mitem, FALSE, FALSE, TRUE);
                        }
                    }
                }
            }
            else {
                if (is_player)
                    msg("A small dart whizzes by your ear and vanishes.");
                else if (can_see)
                    msg("A small dart whizzes by %s's ear and vanishes.",
                        prname(mname, FALSE));
            }
        when POOL: {
            register int i;

            i = rnd(100);
            if (is_player) {
                if ((tp->tr_flags & ISGONE)) {
                    if (i < 56) {
                        teleport();        /* teleport away */
                        pool_teleport = TRUE;
                    }
                    else if((i < 72) && level > 4) {
                        level -= rnd(4) + 1;
                        cur_max = level;
                        new_level(NORMLEV);
                        pool_teleport = TRUE;
                        msg("You here a faint groan from below.");
                    }
                    else if(i < 85) {
                        level += rnd(4) + 1;
                        new_level(NORMLEV);
                        pool_teleport = TRUE;
                        msg("You find yourself in strange surroundings.");
                    }
                    else if(i > 96) {
                        msg("Oh no!!! You drown in the pool!!!  --More--");
                        wait_for(' ');
            pstats.s_hpt = -1;
                        death(D_DROWN);
                    }
            else {
            new_level(NORMLEV);
            pool_teleport = TRUE;
            msg("You are whisked away to another region.");
            }
                }
            }
            else {
                if (i < 60) {
                    if (can_see) {
                        /* Drowns */
                        if (i < 50) 
                            msg("%s drowned in the pool!", prname(mname, TRUE));

                        /* Teleported to another level */
                        else msg("%s disappeared!", prname(mname, TRUE));
                    }
                    killed(mitem, FALSE, FALSE, TRUE);
                }
            }
        }
    when MAZETRAP:
        if (is_player) {
            pstats.s_hpt -= roll(1, 10);
            level++;
            if (pstats.s_hpt < 1) {
        pstats.s_hpt = -1;
        death(D_FALL);
        }
        wclear(cw);
        wclear(mw);
            new_level(MAZELEV);
            msg("You are surrounded by twisty passages! ");
        }
        else {
            if (can_see) msg("%s fell into a maze trap!", prname(mname, TRUE));
            if (on(*th, ISUNIQUE)) {
                    check_residue(th);

                    /* Erase the monster from the old position */
                    if (isalpha(mvwinch(cw, th->t_pos.y, th->t_pos.x)))
                        mvwaddch(cw, th->t_pos.y, th->t_pos.x, th->t_oldch);
                    mvwaddch(mw, th->t_pos.y, th->t_pos.x, ' ');

                    /* let him summon on next lvl */
                    if (on (*th, HASSUMMONED)) {
                            turn_off(*th, HASSUMMONED); 
                            turn_on(*th, CANSUMMON);
                    }
                    turn_on(*th,ISELSEWHERE);
                    detach(mlist, mitem);
                    attach(tlist, mitem);       /* remember him next level */

                    /* Make sure that no one is still chasing us */
                    for (mitem = mlist; mitem != NULL; mitem = next(mitem)) {
                        mp = THINGPTR(mitem);
                        if (mp->t_dest == &th->t_pos) {
                            mp->t_dest = &hero;
                            mp->t_wasshot = FALSE;
                            turn_off(*mp, ISFLEE);      /* Don't run away! */
                        }
                    }

                    /* Make sure we were not chasing a monster here */
                    th->t_dest = &hero;
                    if (on(*th, ISFRIENDLY)) turn_off(*th, ISFLEE);
            }
            else
                    killed(mitem, FALSE, FALSE, FALSE);
        }
    }

    /* Move the cursor back onto the hero */
    wmove(cw, hero.y, hero.x);

    flushinp();
    return(ch);
}

/*
 * blue_light:
 *      magically light up a room (or level or make it dark)
 */

bool
blue_light(blessed, cursed)
bool blessed, cursed;
{
    register struct room *rp;
    bool ret_val=FALSE; /* Whether or not affect is known */

    rp = roomin(&hero); /* What room is hero in? */

    /* Darken the room if the magic is cursed */
    if (cursed) {
        if ((rp == NULL) || !lit_room(rp)) msg(nothing);
        else {
            rp->r_flags |= ISDARK;
            if (!lit_room(rp) && (levtype != OUTSIDE || !daytime) &&
        !ISWEARING(R_LIGHT)) 
                msg("The %s suddenly goes dark.",
                    levtype == OUTSIDE ? "area" : "room");
            else msg(nothing);
            ret_val = TRUE;
        }
    }
    else {
        ret_val = TRUE;
        if (rp && !lit_room(rp) &&
            (levtype != OUTSIDE || !daytime)) {
            addmsg("The %s is lit", levtype == OUTSIDE ? "area" : "room");
            addmsg(" by a %s blue light.", blessed ? "bright" : "shimmering");
            endmsg();
        }
        else if (winat(hero.y, hero.x) == PASSAGE)
            msg("The corridor glows %sand then fades",
                    blessed ? "brightly " : "");
        else {
            ret_val = FALSE;
            msg(nothing);
        }
        if (blessed) {
            register int i;     /* Index through rooms */

            for (i=0; i<MAXROOMS; i++)
                rooms[i].r_flags &= ~ISDARK;
        }
        else if (rp) rp->r_flags &= ~ISDARK;
    }

    /*
     * Light the room and put the player back up
     */
    light(&hero);
    mvwaddch(cw, hero.y, hero.x, PLAYER);
    return(ret_val);
}

/*
 * corr_move:
 *      Check to see that a move is legal.  If so, return correct character.
 *      If not, if player came from a legal place, then try to turn him.
 */

corr_move(dy, dx)
int dy, dx;
{
    int legal=0;                /* Number of legal alternatives */
    register int y, x,          /* Indexes though possible positions */
                 locy = 0, locx = 0;    /* Hold delta of chosen location */

    /* New position */
    move_nh.y = hero.y + dy;
    move_nh.x = hero.x + dx;

    /* If it is a legal move, just return */
    if (move_nh.x >= 0 && move_nh.x < cols && move_nh.y > 0 && move_nh.y < lines - 2) {
        
        switch (winat(move_nh.y, move_nh.x)) {
            case WALL:
            case VERTWALL:
            case HORZWALL:
                break;
            default:
                if (diag_ok(&hero, &move_nh, &player))
                        return;
        }
    }

    /* Check legal places surrounding the player -- ignore previous position */
    for (y = hero.y - 1; y <= hero.y + 1; y++) {
        if (y < 1 || y > lines - 3)
            continue;
        for (x = hero.x - 1; x <= hero.x + 1; x++) {
            /* Ignore borders of the screen */
            if (x < 0 || x > cols - 1)
                continue;
            
            /* 
             * Ignore where we came from, where we are, and where we couldn't go
             */
            if ((x == hero.x - dx && y == hero.y - dy) ||
                (x == hero.x + dx && y == hero.y + dy) ||
                (x == hero.x && y == hero.y))
                continue;

            switch (winat(y, x)) {
                case WALL:
                case VERTWALL:
                case HORZWALL:
                    break;
                default:
                    move_nh.y = y;
                    move_nh.x = x;
                    if (diag_ok(&hero, &move_nh, &player)) {
                        legal++;
                        locy = y - (hero.y - 1);
                        locx = x - (hero.x - 1);
                    }
            }
        }
    }

    /* If we have 2 or more legal moves, make no change */
    if (legal != 1) {
        return;
    }

    runch = Moves[locy][locx];

    /*
     * For mazes, pretend like it is the beginning of a new run at each turn
     * in order to get the lighting correct.
     */
    if (levtype == MAZELEV) firstmove = TRUE;
    return;
}

/*
 * dip_it:
 *      Dip an object into a magic pool
 */

dip_it()
{
        reg struct linked_list *what;
        reg struct object *ob;
        reg struct trap *tp;
        reg int wh, i;

        tp = trap_at(hero.y,hero.x);
        if (tp == NULL || tp->tr_type != POOL) {
            msg("I see no shimmering pool here");
            return;
        }
        if (tp->tr_flags & ISGONE) {
            msg("This shimmering pool appears to have been used once already.");
            return;
        }

        /* It takes 3 movement periods to dip something */
        if (player.t_action != C_DIP) {
            if ((what = get_item(pack, "dip", ALL, FALSE, FALSE)) == NULL) {
                msg("");
                after = FALSE;
                return;
            }

            ob = OBJPTR(what);
            if (ob == cur_armor               || 
                ob == cur_misc[WEAR_BOOTS]    ||
                ob == cur_misc[WEAR_JEWEL]    ||
                ob == cur_misc[WEAR_GAUNTLET] ||
                ob == cur_misc[WEAR_CLOAK]    ||
                ob == cur_misc[WEAR_BRACERS]  ||
                ob == cur_misc[WEAR_NECKLACE] ||
                ob == cur_ring[LEFT_1]  || ob == cur_ring[LEFT_2]  ||
                ob == cur_ring[LEFT_3]  || ob == cur_ring[LEFT_4]  ||
                ob == cur_ring[RIGHT_1] || ob == cur_ring[RIGHT_2] ||
        ob == cur_ring[RIGHT_3] || ob == cur_ring[RIGHT_4]) {
                mpos = 0;
                msg("You'll have to take it off first.");
                return;
            }

            player.t_using = what;      /* Remember what it is */
            player.t_action = C_DIP;    /* We are dipping */
            player.t_no_move = 3 * movement(&player);
            return;
        }

        /* We have waited our time, let's dip it */
        what = player.t_using;
        player.t_using = NULL;
        player.t_action = A_NIL;

        ob = OBJPTR(what);

        tp->tr_flags |= ISGONE;
        if (ob != NULL) {
            wh = ob->o_which;
            ob->o_flags |= ISKNOW;
            i = rnd(100);
            if (ob->o_group != 0)
                ob->o_group = newgrp(); /* change the group */
            switch(ob->o_type) {
                case WEAPON:
                    if(i < 60) {                /* enchant weapon here */
                        if ((ob->o_flags & ISCURSED) == 0) {
                                ob->o_hplus += 1;
                                ob->o_dplus += 1;
                        }
                        else {          /* weapon was prev cursed here */
                                ob->o_hplus = rnd(2);
                                ob->o_dplus = rnd(2);
                        }
                        ob->o_flags &= ~ISCURSED;
                        msg("The %s glows blue for a moment.",weaps[wh].w_name);
                    }
                    else if(i < 75) {   /* curse weapon here */
                        if ((ob->o_flags & ISCURSED) == 0) {
                                ob->o_hplus = -(rnd(2)+1);
                                ob->o_dplus = -(rnd(2)+1);
                        }
                        else {                  /* if already cursed */
                                ob->o_hplus--;
                                ob->o_dplus--;
                        }
                        ob->o_flags |= ISCURSED;
                        msg("The %s glows red for a moment.",weaps[wh].w_name);
                    }                   
                    else
                        msg(nothing);
                when ARMOR:
                    if (i < 60) {       /* enchant armor */
                        if((ob->o_flags & ISCURSED) == 0)
                            ob->o_ac -= rnd(2) + 1;
                        else
                            ob->o_ac = -rnd(3)+ armors[wh].a_class;
                        ob->o_flags &= ~ISCURSED;
                        msg("The %s glows blue for a moment",armors[wh].a_name);
                    }
                    else if(i < 75){    /* curse armor */
                        if ((ob->o_flags & ISCURSED) == 0)
                            ob->o_ac = rnd(3)+ armors[wh].a_class;
                        else
                            ob->o_ac += rnd(2) + 1;
                        ob->o_flags |= ISCURSED;
                        msg("The %s glows red for a moment.",armors[wh].a_name);
                    }
                    else
                        msg(nothing);
                when STICK: {
                    int j;
                    j = rnd(14) + 1;
                    if(i < 60) {                /* add charges */
                        ob->o_charges += j;
                        ws_know[wh] = TRUE;
                        if (ob->o_flags & ISCURSED)
                            ob->o_flags &= ~ISCURSED;
                        msg("The %s %s glows blue for a moment.",
                            ws_made[wh],ws_type[wh]);
                    }
                    else if(i < 75) {   /* remove charges */
                        if ((ob->o_charges -= i) < 0)
                            ob->o_charges = 0;
                        ws_know[wh] = TRUE;
                        if (ob->o_flags & ISBLESSED)
                            ob->o_flags &= ~ISBLESSED;
                        else
                            ob->o_flags |= ISCURSED;
                        msg("The %s %s glows red for a moment.",
                            ws_made[wh],ws_type[wh]);
                    }
                    else 
                        msg(nothing);
                }
                when SCROLL:
                    s_know[wh] = TRUE;
                    msg("The '%s' scroll unfurls.",s_names[wh]);
                when POTION:
                    p_know[wh] = TRUE;
                    msg("The %s potion bubbles for a moment.. ",p_colors[wh]);
                when RING:
                    if(i < 60) {         /* enchant ring */
                        if ((ob->o_flags & ISCURSED) == 0)
                            ob->o_ac += rnd(2) + 1;
                        else
                            ob->o_ac = rnd(2) + 1;
                        ob->o_flags &= ~ISCURSED;
                    }
                    else if(i < 75) { /* curse ring */
                        if ((ob->o_flags & ISCURSED) == 0)
                            ob->o_ac = -(rnd(2) + 1);
                        else
                            ob->o_ac -= (rnd(2) + 1);
                        ob->o_flags |= ISCURSED;
                    }
                    r_know[wh] = TRUE;
                    msg("The %s ring vibrates for a moment.",r_stones[wh]);
                when MM:
                    m_know[wh] = TRUE;
                    switch (ob->o_which) {
                    case MM_BRACERS:
                    case MM_PROTECT:
                        if(i < 60) {     /* enchant item */
                            if ((ob->o_flags & ISCURSED) == 0)
                                ob->o_ac += rnd(2) + 1;
                            else
                                ob->o_ac = rnd(2) + 1;
                            ob->o_flags &= ~ISCURSED;
                        }
                        else if(i < 75) { /* curse item */
                            if ((ob->o_flags & ISCURSED) == 0)
                                ob->o_ac = -(rnd(2) + 1);
                            else
                                ob->o_ac -= (rnd(2) + 1);
                            ob->o_flags |= ISCURSED;
                        }
                        msg("The item vibrates for a moment.");
                    when MM_CHOKE:
                    case MM_DISAPPEAR:
                        ob->o_ac = 0;
                        msg ("The dust dissolves in the pool!");
                    }
                otherwise:
                msg("The pool bubbles up for a moment.. ");
            }
            updpack(FALSE, &player);
        }
        else
            msg(nothing);
}

/*
 * do_move:
 *      Check to see that a move is legal.  If it is handle the
 * consequences (fighting, picking up, etc.)
 */

do_move(dy, dx)
int dy, dx;
{
    register struct room *rp, *orp;
    register unsigned char ch;
    struct linked_list *item;
    register struct thing *tp = NULL;
    coord old_hero;
    register int wasfirstmove, moved, num_hits;
    bool changed=FALSE;   /* Did we switch places with a friendly monster? */

    wasfirstmove = firstmove;
    firstmove = FALSE;
    curprice = -1;        /* if in trading post, we've moved off obj */

    /*
     * Do a confused move (maybe)
     */
    if (player.t_action == A_NIL &&
        ((on(player, ISHUH) && rnd(100) < 80)   || 
         (on(player, ISDANCE) && rnd(100) < 90) || 
         (ISWEARING(R_DELUSION) && rnd(100) < 70)))
    {
        /* Get a random move */
        move_nh = rndmove(&player);
        dy = move_nh.y - hero.y;
        dx = move_nh.x - hero.x;
    }
    else {
        move_nh.y = hero.y + dy;
        move_nh.x = hero.x + dx;
    }

    /*
     * Check if he tried to move off the screen or make an illegal
     * diagonal move, and stop him if he did.
     */
    if (move_nh.x < 0 || move_nh.x > cols-1 || move_nh.y < 1 || move_nh.y >= lines - 2
        || !diag_ok(&hero, &move_nh, &player))
    {
        after = running = FALSE;
        player.t_action = A_NIL;
        return;
    }
    if (running && ce(hero, move_nh))
        after = running = FALSE;
    ch = winat(move_nh.y, move_nh.x);

    /* Take care of hero trying to move close to something frightening */
    if (on(player, ISFLEE)) {
        if (rnd(100) < 12) {
            turn_off(player, ISFLEE);
            msg("You regain your composure.");
        }
        else if (DISTANCE(move_nh.y, move_nh.x, player.t_dest->y, player.t_dest->x) <
                 DISTANCE(hero.y, hero.x, player.t_dest->y, player.t_dest->x)) {
                        running = FALSE;
                        msg("You are too terrified to move that way");
                        player.t_action = A_NIL;
                        player.t_no_move = movement(&player);
                        return;
        }
    }

    /* If we want to move to a monster, see what it is */
    if (isalpha(ch)) {
        item = find_mons(move_nh.y, move_nh.x);
        if (item == NULL) {
            debug("Cannot find monster in move.");
            player.t_action = A_NIL;
            return;
        }
        tp = THINGPTR(item);
    }

    /*
     * Take care of hero being held.  If the player is being held, he
     * can't move unless he is either attacking a non-friendly monster
     * or attacking a friendly monster that can't move.
     */
    if (on(player, ISHELD) &&
        (!isalpha(ch) || (on(*tp, ISFRIENDLY) && off(*tp, ISHELD)))) {
        msg("You are being held.");
        player.t_action = A_NIL;
        return;
    }

    /* See if we have to wait for our movement rate */
    if (player.t_action == A_NIL) {
        after = FALSE;
        firstmove = wasfirstmove;       /* Remember if this is first move */
        player.t_no_move = movement(&player);
        if (player.t_ctype == C_MONK)
            player.t_no_move -= pstats.s_lvl/6;
        if (on(player, ISFLY)) 
            player.t_no_move /= 2; /* If flying, speed him up */

        if (player.t_no_move < 1) player.t_no_move = 1;

        /* Remember our action */
        player.t_action = Moves[dy+1][dx+1];
        return;
    }

    /* Now let's forget the old move and just do it */
    player.t_action = A_NIL;

    /* If we're moving onto a friendly monster, let's change places. */
    if (isalpha(ch) && on(*tp, ISFRIENDLY) && off(*tp, ISHELD)) {
        coord tpos,     /* Where monster may have been going */
              current;  /* Current hero position */
        int action;     /* The monster's action */

        current = hero;
        tpos = tp->t_newpos;
        action = tp->t_action;

        /* Disrupt whatever our friend was doing */
        tp->t_action = A_NIL;

        /* Tentatively move us to where he is */
        hero = tp->t_pos;

        /* See if we can move him to where we were */
        tp->t_newpos = current;
        do_chase(tp);

        /* Did we succeed? */
        if (ce(tp->t_pos, current)) {
            /* Reset our idea of what ch is */
            ch = winat(move_nh.y, move_nh.x);

            /* Let it be known that we made the switch */
            changed = TRUE;
            old_hero = current;

            /* Make the monster think it didn't move */
            tp->t_oldpos = current;
            tp->t_doorgoal.x = tp->t_doorgoal.y = -1;

            /* Let the player know something funny happened. */
            msg("What a sidestep!");
        }
        else {
            /* Restore things -- we couldn't move */
            hero = current;
            tp->t_newpos = tpos;
            tp->t_action = action;
        }
    }

    /* assume he's not in a wall */
    if (!isalpha(ch)) turn_off(player, ISINWALL);

    switch (ch) {
        case VERTWALL:
        case HORZWALL:
            if (levtype == OUTSIDE) {
                hero = move_nh;
                new_level(OUTSIDE);
                return;
            }
        case WALL:
        case SECRETDOOR:
            if (off(player, CANINWALL) || running) {
                after = running = FALSE;

                /* Light if finishing run */
                if (levtype == MAZELEV && lit_room(&rooms[0]))
                    look(FALSE, TRUE);

                after = running = FALSE;

                return;
            }
            turn_on(player, ISINWALL);
            break;
        case POOL:
            if (levtype == OUTSIDE) {
                /* lake_check(&move_nh); */  /* not implemented yet */
                running = FALSE;
                break;
            }
        case MAZETRAP:
            if (levtype == OUTSIDE) {
            running = FALSE;
            break;
        }
        case TRAPDOOR:
        case TELTRAP:
        case BEARTRAP:
        case SLEEPTRAP:
        case ARROWTRAP:
        case DARTTRAP:
        case WORMHOLE:
            ch = be_trapped(&player, &move_nh);
            if (ch == TRAPDOOR || ch == TELTRAP || 
                pool_teleport  || ch == MAZETRAP) {
                pool_teleport = FALSE;
                return;
            }
            break;
        case GOLD:
        case POTION:
        case SCROLL:
        case FOOD:
        case WEAPON:
        case ARMOR:
        case RING:
        case MM:
        case RELIC:
        case STICK:
            running = FALSE;
            take = ch;
            break;
        case DOOR:
        case STAIRS:
        case POST:
            running = FALSE;
            break;
        default:
            break;
    }

    if (isalpha(ch)) { /* if its a monster then fight it */
        /*
         * If we were running down a corridor and didn't start right
         * next to the critter, don't do anything.
         */
        if (running && wasfirstmove == FALSE && roomin(&hero) == NULL) {
            struct linked_list *item;

            item = find_mons(move_nh.y, move_nh.x);
            if (item != NULL && !invisible(THINGPTR(item))) {
                after = running = FALSE;
                return;
            }
        }

        /* We have to add time because we're attacking */
        player.t_no_move = FIGHTBASE;
        player.t_no_move += weap_move(&player, cur_weapon);
        if (on(player, ISHASTE))
                player.t_no_move /= 2;
        else if (on(player, ISSLOW))
                player.t_no_move *= 2;

        /* We may attack faster if we're high enough level 
         * and the right class
         */
        switch(player.t_ctype) {
            case C_FIGHTER: num_hits = player.t_stats.s_lvl/25 + 1;
            when C_PALADIN: num_hits = player.t_stats.s_lvl/35 + 1;
            when C_RANGER:  num_hits = player.t_stats.s_lvl/35 + 1;
            when C_MONK:  if(cur_weapon) num_hits = player.t_stats.s_lvl/40 + 1;
                          else     num_hits = player.t_stats.s_lvl/30 + 1;
            otherwise:      num_hits = player.t_stats.s_lvl/60 + 1;
        }

        /*
         * The player has already moved the initial movement period.
         * Let's add that in, do our division, and then subtract it
         * out so that the total time is divided, not just the
         * additional attack time.
         */
        moved = movement(&player),
        player.t_no_move += moved;
        player.t_no_move /= num_hits;
        player.t_no_move -= moved;
        running = FALSE;

        /* Mark that we are attacking and save the attack coordinate */
        player.t_action = A_ATTACK;
        player.t_newpos = move_nh;
        runch = Moves[dy+1][dx+1];      /* Remember the direction */

        if (player.t_no_move <= 0) after = FALSE;
        return;
    }

    /*
     * if not fighting then move the hero
     */
    if (changed == FALSE) {
        old_hero = hero;        /* Save hero's old position */
        hero = move_nh;              /* Move the hero */
    }
    rp = roomin(&hero);
    orp = roomin(&old_hero);

    /* Unlight any possible cross-corridor */
    if (levtype == MAZELEV) {
        register bool call_light = FALSE;
        register unsigned char wall_check;

        if (wasfirstmove && lit_room(&rooms[0])) {
            /* Are we moving out of a corridor? */
            switch (runch) {
                case 'h':
                case 'l':
                    if (old_hero.y + 1 < lines - 2) {
                        wall_check = winat(old_hero.y + 1, old_hero.x);
                        if (!isrock(wall_check)) call_light = TRUE;
                    }
                    if (old_hero.y - 1 > 0) {
                        wall_check = winat(old_hero.y - 1, old_hero.x);
                        if (!isrock(wall_check)) call_light = TRUE;
                    }
                    break;
                case 'j':
                case 'k':
                    if (old_hero.x + 1 < cols) {
                        wall_check = winat(old_hero.y, old_hero.x + 1);
                        if (!isrock(wall_check)) call_light = TRUE;
                    }
                    if (old_hero.x - 1 >= 0) {
                        wall_check = winat(old_hero.y, old_hero.x - 1);
                        if (!isrock(wall_check)) call_light = TRUE;
                    }
                    break;
                default:
                    call_light = TRUE;
            }
            player.t_oldpos = old_hero;
            if (call_light) light(&old_hero);
        }
    }

    else if (orp != NULL && rp == NULL) {    /* Leaving a room -- darken it */
        orp->r_flags |= FORCEDARK;      /* Fake darkness */
        light(&old_hero);
        orp->r_flags &= ~FORCEDARK;     /* Restore light state */
    }
    else if (rp != NULL && orp == NULL){/* Entering a room */
        light(&hero);
        if (rp->r_flags & ISTREAS)
            wake_room(rp);
    }
    ch = winat(old_hero.y, old_hero.x);
    wmove(cw, unc(old_hero));
    waddch(cw, ch);
    wmove(cw, unc(hero));
    waddch(cw, PLAYER);
}

/*
 * do_run:
 *      Start the hero running
 */

do_run(ch)
char ch;
{
    firstmove = TRUE;
    running = TRUE;
    after = FALSE;
    runch = ch;
}

/*
 * getdelta:
 *      Takes a movement character (eg. h, j, k, l) and returns the
 *      y and x delta corresponding to it in the remaining arguments.
 *      Returns TRUE if it could find it, FALSE otherwise.
 */

bool
getdelta(match, dy, dx)
char match;
int *dy, *dx;
{
    register int y, x;

    for (y = 0; y < 3; y++)
        for (x = 0; x < 3; x++)
            if (Moves[y][x] == match) {
                *dy = y - 1;
                *dx = x - 1;
                return(TRUE);
            }

    return(FALSE);
}

/*
 * isatrap:
 *      Returns TRUE if this character is some kind of trap
 */

isatrap(ch)
reg char ch;
{
        switch(ch) {
                case WORMHOLE:
                case DARTTRAP:
                case TELTRAP:
                case TRAPDOOR:
                case ARROWTRAP:
                case SLEEPTRAP:
                case BEARTRAP:  return(TRUE);
                case MAZETRAP:
                case POOL:      return(levtype != OUTSIDE);
                default:        return(FALSE);
        }
}

/*
 * Called to illuminate a room.
 * If it is dark, remove anything that might move.
 */

light(cp)
coord *cp;
{
    register struct room *rp;
    register int j, k, x, y;
    register unsigned char ch, rch, sch;
    register struct linked_list *item;
    int jlow, jhigh, klow, khigh;       /* Boundaries of lit area */

    if ((rp = roomin(cp)) != NULL) {
        /*
         * is he wearing ring of illumination? 
         */
        if (&hero == cp && ISWEARING(R_LIGHT)) /* Must be hero's room */
            rp->r_flags &= ~ISDARK;
        
        /* If we are in a maze, don't look at the whole room (level) */
        if (levtype == MAZELEV) {
            int see_radius;

            see_radius = 1;

            /* If we are looking at the hero in a rock, broaden our sights */
            if (&hero == cp || &player.t_oldpos == cp) {
                ch = winat(hero.y, hero.x);
                if (isrock(ch)) see_radius = 2;
                ch = winat(player.t_oldpos.y, player.t_oldpos.x);
                if (isrock(ch)) see_radius = 2;
            }

            jlow = max(0, cp->y - see_radius - rp->r_pos.y);
            jhigh = min(rp->r_max.y, cp->y + see_radius + 1 - rp->r_pos.y);
            klow = max(0, cp->x - see_radius - rp->r_pos.x);
            khigh = min(rp->r_max.x, cp->x + see_radius + 1 - rp->r_pos.x);
        }
        else {
            jlow = klow = 0;
            jhigh = rp->r_max.y;
            khigh = rp->r_max.x;
        }
        for (j = 0; j < rp->r_max.y; j++)
        {
            for (k = 0; k < rp->r_max.x; k++)
            {
                bool see_here = 0, see_before = 0;

                /* Is this in the give area -- needed for maze */
                if ((j < jlow || j >= jhigh) && (k < klow || k >= khigh))
                    continue;

                y = rp->r_pos.y + j;
                x = rp->r_pos.x + k;

                /*
                 * If we are in a maze do not look at this area unless
                 * we can see it from where we are or where we last were
                 * (for erasing purposes).
                 */
                if (levtype == MAZELEV) {
                    /* If we can't see it from here, could we see it before? */
                    if ((see_here = maze_view(y, x)) == FALSE) {
                        coord savhero;

                        /* Could we see it from where we were? */
                        savhero = hero;
                        hero = player.t_oldpos;
                        see_before = maze_view(y, x);
                        hero = savhero;

                        if (!see_before) continue;
                    }
                }

                ch = show(y, x);
                wmove(cw, y, x);
                /*
                 * Figure out how to display a secret door
                 */
                if (ch == SECRETDOOR) {
                    if (j == 0 || j == rp->r_max.y - 1)
                        ch = HORZWALL;
                    else
                        ch = VERTWALL;
                }
                /* For monsters, if they were previously not seen and
                 * now can be seen, or vice-versa, make sure that will
                 * happen.  This is for dark rooms as opposed to invisibility.
                 *
                 * Call winat() in the test because ch will not reveal
                 * invisible monsters.
                 */
                if (isalpha(winat(y, x))) {
                    struct thing *tp;   /* The monster */

                    item = wake_monster(y, x);
                    tp = THINGPTR(item);

                    /* Previously not seen -- now can see it */
                    if (tp->t_oldch == ' ' && cansee(tp->t_pos.y, tp->t_pos.x)) 
                        tp->t_oldch = mvinch(y, x);

                    /* Previously seen -- now can't see it */
                    else if (!cansee(tp->t_pos.y, tp->t_pos.x) &&
                             roomin(&tp->t_pos) != NULL)
                        switch (tp->t_oldch) {
                            /*
                             * Only blank it out if it is in a room and not
                             * the border (or other wall) of the room.
                             */
                             case DOOR:
                             case SECRETDOOR:
                             case HORZWALL:
                             case VERTWALL:
                                break;

                             otherwise:
                                tp->t_oldch = ' ';
                        }
                }

                /*
                 * If the room is a dark room, we might want to remove
                 * monsters and the like from it (since they might
                 * move).
                 * A dark room.
                 */
                if ((!lit_room(rp) && (levtype != OUTSIDE)) ||
                    (levtype == OUTSIDE && !daytime) ||
                    on(player, ISBLIND)         ||
                    (rp->r_flags & FORCEDARK)   ||
                    (levtype == MAZELEV && !see_here && see_before)) {
                    sch = mvwinch(cw, y, x);    /* What's seen */
                    rch = mvinch(y, x); /* What's really there */
                    switch (rch) {
                        case DOOR:
                        case SECRETDOOR:
                        case STAIRS:
                        case TRAPDOOR:
                        case WORMHOLE:
                        case TELTRAP:
                        case BEARTRAP:
                        case SLEEPTRAP:
                        case ARROWTRAP:
                        case DARTTRAP:
                        case MAZETRAP:
                        case POOL:
                        case POST:
                        case VERTWALL:
                        case HORZWALL:
                        case WALL:
                            if (isalpha(sch)) ch = rch;
                            else if (sch != FLOOR) ch = sch;
                            else ch = ' '; /* Hide undiscoverd things */
                        when FLOOR:
                            ch = ' ';
                        otherwise:
                            ch = ' ';
                    }
                    /* Take care of our magic bookkeeping. */
                    switch (sch) {
                        case MAGIC:
                        case BMAGIC:
                        case CMAGIC:
                            ch = sch;
                    }
                }
                mvwaddch(cw, y, x, ch);
            }
        }
    }
}

/*
 * lit_room:
 *      Called to see if the specified room is lit up or not.
 */

bool
lit_room(rp)
register struct room *rp;
{
    register struct linked_list *fire_item;
    register struct thing *fire_creature;

    if (!(rp->r_flags & ISDARK)) return(TRUE);  /* A definitely lit room */

    /* Is it lit by fire light? */
    if (rp->r_flags & HASFIRE) {
        switch ((int)levtype) {
            case MAZELEV:
                /* See if a fire creature is in line of sight */
                for (fire_item = rp->r_fires; fire_item != NULL;
                     fire_item = next(fire_item)) {
                    fire_creature = THINGPTR(fire_item);
                    if (maze_view(fire_creature->t_pos.y,
                                  fire_creature->t_pos.x)) return(TRUE);
                }

                /* Couldn't find any in line-of-sight */
                return(FALSE);

            /* We should probably do something special for the outside */
            otherwise:
                return TRUE;
        }
    }
    return(FALSE);
}

/*
 * movement:
 *      Given a pointer to a player/monster structure, calculate the
 *      movement rate for that character.
 */

short
movement(tp)
register struct thing *tp;
{
    register int result;
    register int carry;         /* Percentage carried */

    result = 0;

    /* Adjust for armor (player only) */
    if (tp == &player && cur_armor) {
        int diff;  /* Now armor class differs from normal one of same type */

        /* Blessed armor adds less */
        diff = cur_armor->o_ac - armors[cur_armor->o_which].a_class;
        switch (cur_armor->o_which) {
            case LEATHER:
            case RING_MAIL:
            case CHAIN_MAIL:
            case SCALE_MAIL:
            case PADDED_ARMOR:
                diff += 1;
            when STUDDED_LEATHER:
            case SPLINT_MAIL:
            case BANDED_MAIL:
            case PLATE_MAIL:
                diff += 2;
            when PLATE_ARMOR:
                diff += 3;
            otherwise:
                debug("forgot an armor in movement()");
        }
        if (diff < 0) diff = 0;
        result += diff;

    }

    /* Adjust for the pack */
    carry = 100 * tp->t_stats.s_pack / tp->t_stats.s_carry;
    if (carry > 75) result++;

    /* Get a bonus for dexterity */
    result -= dext_plus(tp == &player ? dex_compute() : tp->t_stats.s_dext);

    /* only allow adjust for the minus's */
    if (result < 0) result = 0;
    result += tp->t_movement; /* now add in movement rate */

    /* Is the character slowed? */
    if (on(*tp, ISSLOW) || on(*tp, ISDANCE)) result *= 2;

    /* Is the character hasted? */
    if (on(*tp, ISHASTE)) result /= 2;

    /* We have a minimum of 1 */
    if (result < 1) result = 1;

    return(result);
}

/*
 * rndmove:
 *      move in a random direction if the monster/person is confused
 */

coord 
rndmove(who)
struct thing *who;
{
    register int x, y;
    register int ex, ey, nopen = 0;
    coord ret;  /* what we will be returning */
    coord dest;

    ret = who->t_pos;
    /*
     * Now go through the spaces surrounding the player and
     * set that place in the array to true if the space can be
     * moved into
     */
    ey = ret.y + 1;
    ex = ret.x + 1;
    for (y = who->t_pos.y - 1; y <= ey; y++)
        if (y > 0 && y < lines - 2)
            for (x = who->t_pos.x - 1; x <= ex; x++)
            {
                if (x < 0 || x >= cols)
                    continue;
                if (step_ok(y, x, NOMONST, who) == TRUE)
                {
                    dest.y = y;
                    dest.x = x;
                    if (!diag_ok(&who->t_pos, &dest, who))
                        continue;
                    if (rnd(++nopen) == 0)
                        ret = dest;
                }
            }
    return ret;
}

#define TRAPTYPES 9             /* 9 total trap types that can be set */
#define WIZARDTRAPS 3           /* Only wizards can set the last 3 */
                /* CTRL(C) to level 400 for POST level */
static const char *trap_types[TRAPTYPES] = {
    "Trap Door",
    "Bear Trap",
    "Sleep Trap",
    "Arrow Trap",
    "Teleport Trap",
    "Dart Trap",
    "Magic pool",
    "Maze Trap",
    "Worm Hole"
};

/*
 * set_trap:
 *      set a trap at (y, x) on screen.
 */

set_trap(tp, y, x)
register struct thing *tp;
register int y, x;
{
    register bool is_player = (tp == &player);
    register int selection = rnd(TRAPTYPES-WIZARDTRAPS) + '1';
    register int i, num_traps;
    register unsigned char ch = 0, och;
    int thief_bonus = 0;
    int s_dext;

    /* let wizard in on this too */
    if (wizard) goto can_traps;
    if (is_player && player.t_ctype != C_THIEF && player.t_ctype !=C_ASSASSIN) {
        msg("Only thieves and assassins can set traps. ");
        return;
    }
    can_traps:
    switch (och = mvinch(y, x)) {
        case WALL:
        case FLOOR:
        case PASSAGE:
            break;
        default:
            if (is_player) msg("The trap failed!");
            return;
    }

    if (is_player) {
        int state = 0, /* 0 -> current screen, 1 -> prompt screen, 2 -> done */
            units;     /* Number of movement units for the given trap */

        if (player.t_action == C_SETTRAP) {
            selection = player.t_selection;
            player.t_selection = 0;
            player.t_action = A_NIL;
        }
        else {
            msg("Which kind of trap do you wish to set? (* for a list): ");
            num_traps = TRAPTYPES - (wizard ? 0 : WIZARDTRAPS);
            do {
                selection = wgetch(cw);
                switch (selection) {
                    case '*':
                      if (state != 1) {
                        wclear(hw);
                        touchwin(hw);
                        for (i=0; i<num_traps; i++) {
                            wmove(hw, i+2, 0);
                            wprintw(hw, "[%d] %s", i+1, trap_types[i]);
                        }
                        mvwaddstr(hw, 0, 0,
                                "Which kind of trap do you wish to set? ");

                        if (menu_overlay)
                            /*
                             * Put out the selection.  The longest line is
                             * the prompt line (39 characters long).
                             */
                            over_win(cw, hw, num_traps + 3, 41, 0, 39, NULL);
                        else
                            draw(hw);
                        state = 1;      /* Now in prompt window */
                      }
                      break;

                    case ESC:
                        if (state == 1) {
                            clearok(cw, FALSE);
                            touchwin(cw);
                        }
                        msg("");

                        trap_tries--;   /* Don't count this one */
                        after = FALSE;
                        return;

                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                        if (selection < '7' || wizard) {
                            if (state == 1) {   /* In prompt window */
                                clearok(cw, FALSE); /* Set up for redraw */
                                touchwin(cw);
                            }

                            msg("");

                            /*
                             * Make sure there is a floor below us for trap
                             * doors.
                             */
                            if (selection == '1' && level >= nfloors) {
                                if (state == 1) draw(cw);
                                msg("There is no level below this one.");
                                return;
                            }
                            state = 2;  /* Finished */
                            break;
                        }

                        /* Fall through for non-wizard, unusual trap case */
                    default:
                        if (state == 1) {       /* In the prompt window */
                            wmove(hw, 0, 0);
                            wprintw(hw, 
                                "Please enter a selection between 1 and %d:  ",
                                num_traps);
                            if (menu_overlay)
                                /*
                                 * Put out the selection.  The longest line is
                                 * the prompt line (43 characters long).
                                 */
                                over_win(cw, hw, num_traps+3, 45, 0, 43, NULL);
                            else 
                                draw(hw);
                        }
                        else {  /* Normal window */
                            mpos = 0;
                            msg("Please enter a selection between 1 and %d:  ",
                                num_traps);
                        }
                }
            } while (state != 2);

            player.t_selection = selection;

            switch (selection) {
                case '1': units = 10;   /* Trap door */
                when '2': units = 5;    /* Bear trap */
                when '3': units = 7;    /* Sleeping gas trap */
                when '4': units = 5;    /* Arrow trap */
                when '5': units = 10;   /* Teleport trap */
                when '6': units = 7;    /* Dart trap */
                otherwise: units = 5;   /* Unknown trap */
            }
            player.t_no_move = units * movement(&player);
            player.t_action = C_SETTRAP;
            return;
        }
    }

    if (is_player && player.t_ctype == C_THIEF)   thief_bonus = 20;
    if (is_player && player.t_ctype == C_ASSASSIN) thief_bonus = 15;
    if (is_player && player.t_ctype == C_FIGHTER) thief_bonus = 10;

    s_dext = (tp == &player) ? dex_compute() : tp->t_stats.s_dext;

    if (ntraps >= MAXTRAPS || ++trap_tries >= MAXTRPTRY ||
        levtype == POSTLEV || levtype == OUTSIDE ||
        rnd(80) >= (s_dext + tp->t_stats.s_lvl/2 + thief_bonus)) {
        if (is_player) msg("The trap failed!");
        return;
    }

    switch (selection) {
        case '1': ch = TRAPDOOR;
        when '2': ch = BEARTRAP;
        when '3': ch = SLEEPTRAP;
        when '4': ch = ARROWTRAP;
        when '5': ch = TELTRAP;
        when '6': ch = DARTTRAP;
        when '7': ch = POOL;
        when '8': ch = MAZETRAP;
        when '9': ch = WORMHOLE;
    }

    mvaddch(y, x, ch);
    traps[ntraps].tr_show = och;
    traps[ntraps].tr_type = ch;
    traps[ntraps].tr_pos.y = y;
    traps[ntraps].tr_pos.x = x;
    if (is_player) 
        traps[ntraps].tr_flags = ISTHIEFSET;
    if (ch == POOL || ch == POST) {
        traps[ntraps].tr_flags |= ISFOUND;
    }

    ntraps++;
}

/*
 * show:
 *      returns what a certain thing will display as to the un-initiated
 */

show(y, x)
register int y, x;
{
    register unsigned char ch = winat(y, x);
    register struct linked_list *it;
    register struct thing *tp;

    if (isatrap(ch)) {
        register struct trap *trp = trap_at(y, x);

        return (trp->tr_flags & ISFOUND) ? ch : trp->tr_show;
    }
    else if (isalpha(ch)) {
        if ((it = find_mons(y, x)) == NULL) {
            msg("Show:  Can't find monster in show (%d, %d)", y, x);
            return(mvwinch(stdscr, y, x));
        }
        tp = THINGPTR(it);

        if (on(*tp, ISDISGUISE)) ch = tp->t_disguise; /* As a mimic */

        /* Hide invisible creatures */
        else if (invisible(tp)) {
            /* We can't see surprise-type creatures through "see invisible" */
            if (off(player,CANSEE) || on(*tp,CANSURPRISE))
                ch = mvwinch(stdscr, y, x); /* Invisible */
        }
        else if (on(*tp, CANINWALL)) {
            if (isrock(mvwinch(stdscr, y, x))) ch = winch(stdscr); /* As Xorn */
        }
    }
    return ch;
}


/*
 * trap_at:
 *      find the trap at (y,x) on screen.
 */

struct trap *
trap_at(y, x)
register int y, x;
{
    register struct trap *tp, *ep;

    ep = &traps[ntraps];
    for (tp = traps; tp < ep; tp++)
        if (tp->tr_pos.y == y && tp->tr_pos.x == x)
            break;
    if (tp == ep)
        debug((sprintf(prbuf, "Trap at %d,%d not in array", y, x), prbuf));
    return tp;
}

/*
 * weap_move:
 *      Calculate how many segments it will take to swing the given
 *      weapon (note that the weapon may actually be a stick or
 *      even something else).
 */

weap_move(wielder, weap)
register struct thing *wielder; /* Who's wielding the weapon */
register struct object *weap;   /* The weapon */
{
    register int weap_rate;
    int          dexterity;
    int          strength;

    if (weap == NULL) return(1); /* hand, claw, bite attacks are quick */

    switch (weap->o_type) {
        case STICK:
            if (EQUAL(ws_type[weap->o_which], "staff"))
                weap_rate = 2;
            else weap_rate = 1; /* A wand */

        when WEAPON:
            weap_rate = weaps[weap->o_which].w_rate;

            /* Adjust for blessed or cursed weapon */
            if (weap->o_hplus < 0)      /* Cursed */
                weap_rate -= (weap->o_hplus - 2) / 3;
            else if (weap_rate > 0)     /* Blessed */
                weap_rate -= (2*weap->o_hplus + weap_rate - 1) / weap_rate;

        when RELIC:
            switch (weap->o_which) {
                case MUSTY_DAGGER:
                case HRUGGEK_MSTAR:
                case AXE_AKLAD:
                case YEENOGHU_FLAIL:
                case MING_STAFF:
                case ORCUS_WAND:
                case ASMO_ROD:
                    /* These operate in the blink of an eye */
                    weap_rate = 1;
                otherwise:
                    /* What is it? */
                    weap_rate = 10;
                    debug("unknown weapon in weap_move()");
            }
        otherwise:
            /* What is it? */
            weap_rate = 10;
            debug("unknown weapon in weap_move()");
    }

    /* Put in a dexterity bonus */
    if (wielder == &player) dexterity = dex_compute();
    else dexterity = wielder->t_stats.s_dext;
    weap_rate -= dext_plus(dexterity) / 2;

    /* Put in a strength bonus */
    if (wielder == &player) strength = str_compute();
    else strength = wielder->t_stats.s_str;
    weap_rate -= str_plus(strength) / 2;

    /* It can't speed you up and it must take SOME time */
    if (weap_rate <= 0) weap_rate = 1;

    /* Do we need to adjust for fast/slow movement? */
    if (on(*wielder, ISSLOW) || on(*wielder, ISDANCE)) weap_rate *= 2;
    if (on(*wielder, ISHASTE)) weap_rate /= 2;

    /* Return the result */
    return(weap_rate);
}

