/*
    chase.c  -  Code for one object to chase another

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

#include <ctype.h>
#include <curses.h>
#include <limits.h>
#include "rogue.h"

/*
 * Canblink checks if the monster can teleport (blink).  If so, it will
 * try to blink the monster next to the player.
 */

bool
can_blink(tp)
register struct thing *tp;
{
    register int y, x, index=9;
    coord tryp; /* To hold the coordinates for use in diag_ok */
    bool spots[9], found_one=FALSE;

    /*
     * First, can the monster even blink?  And if so, there is only a 50%
     * chance that it will do so.  And it won't blink if it is running or
     * held.
     */
    if (off(*tp, CANBLINK) || (on(*tp, ISHELD)) ||
        on(*tp, ISFLEE) ||
        tp->t_action == A_FREEZE ||
        (rnd(12) < 6)) return(FALSE);


    /* Initialize the spots as illegal */
    do {
        spots[--index] = FALSE;
    } while (index > 0);

    /* Find a suitable spot next to the player */
    for (y=hero.y-1; y<hero.y+2; y++)
        for (x=hero.x-1; x<hero.x+2; x++, index++) {
            /* Make sure x coordinate is in range and that we are
             * not at the player's position
             */
            if (x<0 || x >= cols || index == 4) continue;

            /* Is it OK to move there? */
            if (step_ok(y, x, NOMONST, tp) &&
                (!isatrap(mvwinch(cw, y, x)) ||
                  rnd(10) >= tp->t_stats.s_intel ||
                  on(*tp, ISFLY))) {
                /* OK, we can go here.  But don't go there if
                 * monster can't get at player from there
                 */
                tryp.y = y;
                tryp.x = x;
                if (diag_ok(&tryp, &hero, tp)) {
                    spots[index] = TRUE;
                    found_one = TRUE;
                }
            }
        }

    /* If we found one, go to it */
    if (found_one) {
        unsigned char rch;       /* What's really where the creatures moves to */

        /* Find a legal spot */
        while (spots[index=rnd(9)] == FALSE) continue;

        /* Get the coordinates */
        y = hero.y + (index/3) - 1;
        x = hero.x + (index % 3) - 1;

        /* Move the monster from the old space */
        mvwaddch(cw, tp->t_pos.y, tp->t_pos.x, tp->t_oldch);

        /* Move it to the new space */
        tp->t_oldch = mvwinch(cw, y, x);

        /* Display the creature if our hero can see it */
        if (cansee(y, x) &&
            off(*tp, ISINWALL) &&
            !invisible(tp))
            mvwaddch(cw, y, x, tp->t_type);

        /* Fix the monster window */
        mvwaddch(mw, tp->t_pos.y, tp->t_pos.x, ' '); /* Clear old position */
        mvwaddch(mw, y, x, tp->t_type);

        /* Record the new position */
        tp->t_pos.y = y;
        tp->t_pos.x = x;

        /* If the monster is on a trap, trap it */
        rch = mvinch(y, x);
        if (isatrap(rch)) {
            if (cansee(y, x)) tp->t_oldch = rch;
            be_trapped(tp, &(tp->t_pos));
        }
    }

    return(found_one);
}

/* 
 * Can_shoot determines if the monster (er) has a direct line of shot 
 * at the prey (ee).  If so, it returns the direction in which to shoot.
 */

int
can_shoot(er, ee, shoot_dir)
register coord *er, *ee, *shoot_dir;
{
    /* 
     * They must be in the same room or very close (at door)
     */
    if (roomin(er) != roomin(ee) && DISTANCE(er->y,er->x,ee->y,ee->x) > 1)
    {
        shoot_dir->x = shoot_dir->y = 0;
        return(-1);
    }

    /* Do we have a straight shot? */
    if (!straight_shot(er->y, er->x, ee->y, ee->x, shoot_dir)) 
    {
        shoot_dir->x = shoot_dir->y = 0;
        return(-2);
    }
    else 
        return(0);
}

/*
 * chase:
 *      Find the spot for the chaser(er) to move closer to the
 *      chasee(ee).  Rer is the room of the chaser, and ree is the
 *      room of the creature being chased (chasee).
 */

chase(tp, ee, rer, ree, flee)
register struct thing *tp;
register coord *ee;
register struct room *rer, *ree;
bool flee; /* True if destination (ee) is player and monster is running away
            * or the player is in a wall and the monster can't get to it
            */
{
    int dist, thisdist, monst_dist = INT_MAX; 
    register coord *er = &tp->t_pos; 
    struct thing *prey;                 /* What we are chasing */
    coord ch_ret;                       /* Where chasing takes you */
    unsigned char ch, mch;
    bool next_player = FALSE;

    /* 
     * set the distance from the chas(er) to the chas(ee) here and then
     * we won't have to reset it unless the chas(er) moves (instead of shoots)
     */
    dist = DISTANCE(er->y, er->x, ee->y, ee->x);

    /*
     * See if our destination is a monster or player.  If so, make "prey" point
     * to it.
     */
    if (ce(hero, *ee)) prey = &player;  /* Is it the player? */
    else if (tp->t_dest && ce(*(tp->t_dest), *ee)) {    /* Is it a monster? */
        struct linked_list *item;

        /* What is the monster we're chasing? */
        item = find_mons(ee->y, ee->x);
        if (item != NULL) prey = THINGPTR(item);
        else prey = NULL;
    }
    else prey = NULL;

    /* We will use at least one movement period */
    tp->t_no_move = movement(tp);
    if (on(*tp, ISFLY)) /* If the creature is flying, speed it up */
        tp->t_no_move /= 2;

    /*
     * If the thing is confused or it can't see the player,
     * let it move randomly. 
     */
    if ((on(*tp, ISHUH) && rnd(10) < 8) ||
        (prey && on(*prey, ISINVIS) && off(*tp, CANSEE))) { /* invisible prey */
        /*
         * get a valid random move
         */
        tp->t_newpos = rndmove(tp);
        dist = DISTANCE(tp->t_newpos.y, tp->t_newpos.x, ee->y, ee->x);
    }

    /*
     * Otherwise, find the empty spot next to the chaser that is
     * closest to the chasee.
     */
    else {
        register int ey, ex, x, y;
        int dist_to_old = INT_MIN; /* Dist from goal to old position */

        /*
         * This will eventually hold where we move to get closer
         * If we can't find an empty spot, we stay where we are.
         */
        dist = flee ? 0 : INT_MAX;
        ch_ret = *er;

        /* Are we at our goal already? */
        if (!flee && ce(ch_ret, *ee)) {
            turn_off(*tp, ISRUN);       /* So stop running! */
            return;
        }

        ey = er->y + 1;
        ex = er->x + 1;

        /* Check all possible moves */
        for (x = er->x - 1; x <= ex; x++) {
            if (x < 0 || x >= cols) /* Don't try off the board */
                continue;
            for (y = er->y - 1; y <= ey; y++) {
                coord tryp;

                if ((y < 1) || (y >= lines - 2)) /* Don't try off the board */
                    continue;

                /* Don't try the player if not going after the player */
                if ((flee || !ce(hero, *ee) || on(*tp, ISFRIENDLY)) &&
                    x == hero.x && y == hero.y) {
                    next_player = TRUE;
                    continue;
                }

                tryp.x = x;
                tryp.y = y;

                /* Is there a monster on this spot closer to our goal?
                 * Don't look in our spot or where we were.
                 */
                if (!ce(tryp, *er) && !ce(tryp, tp->t_oldpos) &&
                    isalpha(mch = mvwinch(mw, y, x))) {
                    int test_dist;

                    test_dist = DISTANCE(y, x, ee->y, ee->x);
                    if (test_dist <= 25 &&   /* Let's be fairly close */
                        test_dist < monst_dist) {
                        /* Could we really move there? */
                        mvwaddch(mw, y, x, ' '); /* Temporarily blank monst */
                        if (diag_ok(er, &tryp, tp)) monst_dist = test_dist;
                        mvwaddch(mw, y, x, mch); /* Restore monster */
                    }
                }

                /* Can we move onto the spot? */        
                if (!diag_ok(er, &tryp, tp)) continue;

                ch = mvwinch(cw, y, x); /* Screen character */

                /*
                 * Stepping on player is NOT okay if we are fleeing.
                 * If we are friendly to the player and there is a monster
                 * in the way that is not of our race, it is okay to move
                 * there.
                 */
                if (step_ok(y, x, FIGHTOK, tp) &&
                    (off(*tp, ISFLEE) || ch != PLAYER))
                {
                    /*
                     * If it is a trap, an intelligent monster may not
                     * step on it (unless our hero is on top!)
                     */
                    if ((isatrap(ch))                   && 
                        (rnd(10) < tp->t_stats.s_intel) &&
                        (!on(*tp, ISFLY))               &&
                        (y != hero.y || x != hero.x)) 
                            continue;

                    /*
                     * OK -- this place counts
                     */
                    thisdist = DISTANCE(y, x, ee->y, ee->x);

                    /* Adjust distance if we are being shot at */
                    if (tp->t_wasshot && tp->t_stats.s_intel > 5 &&
                        prey != NULL) {
                        /* Move out of line of sight */
                        if (straight_shot(tryp.y, tryp.x, ee->y, ee->x, (coord *)NULL)) {
                            if (flee) thisdist -= SHOTPENALTY;
                            else thisdist += SHOTPENALTY;
                        }

                        /* But do we want to leave the room? */
                        else if (rer && rer == ree && ch == DOOR)
                            thisdist += DOORPENALTY;
                    }

                    /* Don't move to the last position if we can help it
                     * (unless out prey just moved there)
                     */
                    if (ce(tryp, tp->t_oldpos) && (flee || !ce(tryp, hero)))
                        dist_to_old = thisdist;

                    else if ((flee && (thisdist > dist)) ||
                        (!flee && (thisdist < dist)))
                    {
                        ch_ret = tryp;
                        dist = thisdist;
                    }
                }
            }
        }

        /* If we aren't trying to get the player, but he is in our way,
         * hit him (unless we have been turned or are friendly).  next_player
         * being TRUE -> we are next to the player but don't want to hit him.
         *
         * If we are friendly to the player, following him, and standing next
         * to him, we will try to help him out in battle.
         */
        if (next_player && off(*tp, WASTURNED)) {
            if (off(*tp, ISFRIENDLY) &&
                ((flee && ce(ch_ret, *er)) ||
                 (!flee && DISTANCE(er->y, er->x, ee->y, ee->x) < dist)) &&
                step_ok(tp->t_dest->y, tp->t_dest->x, NOMONST, tp)) {
                /* Okay to hit player */
                debug("Switching to hero.");
                tp->t_newpos = hero;
                tp->t_action = A_MOVE;
                return;
            }
            else if (on(*tp, ISFRIENDLY) && !flee && ce(*ee, hero)) {
                /*
                 * Look all around the player.  If there is a fightable
                 * creature next to both of us, hit it.  Otherwise, if
                 * there is a fightable creature next to the player, try
                 * to move next to it.
                 */
                dist = INT_MAX;
                for (x = hero.x - 1; x <= hero.x + 1; x++) {
                    if (x < 0 || x >= cols) /* Don't try off the board */
                        continue;
                    for (y = hero.y - 1; y <= hero.y + 1; y++) {
                        if ((y < 1) || (y >= lines - 2)) /* Stay on the board */
                            continue;

                        /* Is there a fightable monster here? */
                        if (isalpha(mvwinch(mw, y, x)) &&
                            step_ok(y, x, FIGHTOK, tp) &&
                            off(*tp, ISSTONE)) {
                            thisdist = DISTANCE(er->y, er->x, y, x);
                            if (thisdist < dist) {
                                dist = thisdist;
                                ch_ret.y = y;
                                ch_ret.x = x;
                            }
                        }
                    }
                }

                /* Are we next to a bad guy? */
                if (dist <= 2) {        /* Get him! */
                    tp->t_newpos = ch_ret;
                    tp->t_action = A_MOVE;
                }

                /* Try to move to the bad guy */
                else if (dist < INT_MAX)
                    chase(tp, &ch_ret,
                          roomin(&tp->t_pos), roomin(&ch_ret), FALSE);

                else tp->t_action = A_NIL;

                return;
            }
        }

        /*
         * If we have decided that we can move onto a monster (we are
         * friendly to the player, go to it.
         */
        if (!ce(ch_ret, *er) && isalpha(mvwinch(mw, ch_ret.y, ch_ret.x))) {
            debug("Attack monster");
            tp->t_newpos = ch_ret;
            tp->t_action = A_MOVE;
            return;
        }

        /* If we can't get closer to the player (if that's our goal)
         * because other monsters are in the way, just stay put
         */
        if (!flee && ce(hero, *ee) && monst_dist < INT_MAX &&
            DISTANCE(er->y, er->x, hero.y, hero.x) < dist) {
                tp->t_action = A_NIL; /* do nothing for awhile */
                return;
        }

        /* Do we want to go back to the last position? */
        else if (dist_to_old != INT_MIN &&      /* It is possible to move back */
            ((flee && dist == 0) ||     /* No other possible moves */
             (!flee && dist == INT_MAX))) {
            /* Do we move back or just stay put (default)? */
            dist = DISTANCE(er->y, er->x, ee->y, ee->x); /* Current distance */
            if (!flee || (flee && (dist_to_old > dist))) ch_ret = tp->t_oldpos;
        }

        /* Record the new destination */
        tp->t_newpos = ch_ret;
    }

    /*
     * Do we want to fight or move?  If our selected destination (ch_ret)
     * is our hero, then we want to fight.  Otherwise, we want to move.
     */
    if (ce(tp->t_newpos, hero)) {
        /* Fight! (or sell) */
        if (on(*tp, CANSELL)) {
            tp->t_action = A_SELL;
            tp->t_no_move += movement(tp); /* takes a little time to sell */
        }
        else {
            tp->t_action = A_ATTACK;

            /*
             * Try to find a weapon to wield.  Wield_weap will return a
             * projector if weapon is a projectile (eg. bow for arrow).
             * If weapon is NULL (the case here), it will try to find
             * a suitable weapon.
             *
             *          Add in rest of time. Fight is 
             *          movement() + weap_move() + FIGHTBASE
             */
            tp->t_using = wield_weap((struct object *)NULL, tp);
            if (tp->t_using == NULL)
                tp->t_no_move += weap_move(tp, (struct object *)NULL);
            else
                tp->t_no_move += weap_move(tp, OBJPTR(tp->t_using));

            if (on(*tp, ISHASTE))
                    tp->t_no_move += FIGHTBASE/2;
            else if (on(*tp, ISSLOW))
                    tp->t_no_move += FIGHTBASE*2;
            else
                    tp->t_no_move += FIGHTBASE;
        }
    }
    else {
        /* Move */
        tp->t_action = A_MOVE;

        /*
         * Check if the creature is not next to the player.  If it
         * is not and has held or suffocated the player, then stop it!
         * Note that this code should more appropriately appear in
         * the area that actually moves the monster, but for now it
         * is okay here because the player can't move while held or
         * suffocating.
         */
        if (dist > 2) {
            if (on(*tp, DIDHOLD)) {
                 turn_off(*tp, DIDHOLD);
                 turn_on(*tp, CANHOLD);
                 if (--hold_count == 0) 
                     turn_off(player, ISHELD);
            }

            /* If monster was suffocating, stop it */
            if (on(*tp, DIDSUFFOCATE)) {
                turn_off(*tp, DIDSUFFOCATE);
                turn_on(*tp, CANSUFFOCATE);
                extinguish(suffocate);
                msg("You can breathe again.....Whew!");
            }
        }
    }
}

/*
 * do_chase:
 *      Make one thing chase another.
 */

do_chase(th)
register struct thing *th;
{
    register struct room *orig_rer,     /* Original room of chaser */
                         *new_room;     /* new room of monster */
    unsigned char floor, rch, sch;
    coord old_pos,                      /* Old position of monster */
          ch_ret;                       /* Where we want to go */

    if (on(*th, NOMOVE)) return;

    ch_ret = th->t_newpos;      /* Record our desired new position */

    /*
     * Make sure we have an open spot (no other monster's gotten in our way,
     * someone didn't just drop a scare monster there, our prey didn't just
     * get there, etc.)
     */
    if (!step_ok(th->t_newpos.y, th->t_newpos.x, FIGHTOK, th)) {
        /*
         * Most monsters get upset now.  Guardians are all friends,
         * and we don't want to see 50 messages in a row!
         */
        if (th->t_stats.s_intel > 4 &&
            off(*th, ISUNDEAD)      &&
            off(*th, ISGUARDIAN)    &&
            off(*th, AREMANY)       &&
            off(*th, ISHUH)         &&
            off(*th, ISCHARMED)     &&
            off(player, ISBLIND)    &&
            cansee(unc(th->t_pos))  &&
        !invisible(th) && (rnd(15) < 5)) {
            switch (rnd(10)) {
              case 0: case 1:
            msg("%s lashes out at you! ",prname(monster_name(th),TRUE));
          when 2: case 3:
            msg("%s scrambles around. ",prname(monster_name(th), TRUE));
                  otherwise:
            msg("%s motions angrily. ", prname(monster_name(th), TRUE));
            }
    }
        return;
    }
    else if (ce(th->t_newpos, hero) ||  /* Player just got in our way */
             isalpha(mvwinch(mw, th->t_newpos.y, th->t_newpos.x))) {
        bool fightplayer = ce(th->t_newpos, hero);

        /* If we were turned or are friendly, we just have to sit here! */
        if (fightplayer && (on(*th, WASTURNED) || on(*th, ISFRIENDLY))) return;

        /* Do we want to sell something? */
        if (fightplayer && on(*th, CANSELL)) {
            th->t_action = A_SELL;
            th->t_no_move += movement(th); /* takes a little time to sell */
            return;
        }

        /* Let's hit him */
        th->t_action = A_ATTACK;

        /*
         * Try to find a weapon to wield.  Wield_weap will return a
         * projector if weapon is a projectile (eg. bow for arrow).
         * If weapon is NULL (the case here), it will try to find
         * a suitable weapon.
         */
        th->t_using = wield_weap((struct object *)NULL, th);
        /*
         * add in rest of time
         */
        if (th->t_using == NULL)
            th->t_no_move += weap_move(th, (struct object *)NULL);
        else
            th->t_no_move += weap_move(th, OBJPTR(th->t_using));
        if (on(*th, ISHASTE))
                th->t_no_move += FIGHTBASE/2;
        else if (on(*th, ISSLOW))
                th->t_no_move += FIGHTBASE*2;
        else
                th->t_no_move += FIGHTBASE;
        return;
    }

    /*
     * Blank out the old position and record the new position --
     * the blanking must be done first in case the positions are the same.
     */
    mvwaddch(mw, th->t_pos.y, th->t_pos.x, ' ');
    mvwaddch(mw, ch_ret.y, ch_ret.x, th->t_type);

    /* Get new and old rooms of monster */
    new_room = roomin(&ch_ret);
    orig_rer = roomin(&th->t_pos);

    /* Store the critter's old position and update the current one */
    old_pos = th->t_pos;
    th->t_pos = ch_ret;
    floor = (roomin(&ch_ret) == NULL) ? PASSAGE : FLOOR;

    /* If we have a scavenger, it can pick something up */
    if (off(*th, ISGUARDIAN)) {
        register struct linked_list *n_item, *o_item;
        register int item_count = 0;
        bool want_something = FALSE;

        while ((n_item = find_obj(ch_ret.y, ch_ret.x)) != NULL) {
            register struct object *n_obj, *o_obj;
            bool wants_it;

            /* Does this monster want anything? */
            if (want_something == FALSE) {
                if (on(*th, ISSCAVENGE)  || on(*th, CARRYFOOD)   ||
                    on(*th, CARRYGOLD)   || on(*th, CARRYSCROLL) ||
                    on(*th, CARRYPOTION) || on(*th, CARRYRING)   ||
                    on(*th, CARRYSTICK)  || on(*th, CARRYMISC)   ||
                    on(*th, CARRYWEAPON) || on(*th, CARRYARMOR)  ||
                    on(*th, CARRYDAGGER))  {
                        want_something = TRUE;

                        /*
                         * Blank the area.  We have to do it only before the
                         * first item in case an item gets dropped in same
                         * place.  We don't want to blank it out after it get
                         * dropped.
                         */
                        mvaddch(ch_ret.y, ch_ret.x, floor);

                        /* Were we specifically after something here? */
                        if (ce(*th->t_dest, ch_ret)) {
                            /* If we're mean, we go after the hero */
                            if (on(*th, ISMEAN)) runto(th, &hero);

                            /* Otherwise just go back to sleep */
                            else {
                                turn_off(*th, ISRUN);
                                th->t_dest = NULL;
                            }
                        }
                }
                else break;
            }

            item_count++;       /* Count the number of items */

            /*
             * see if he's got one of this group already
             */
            o_item = NULL;
            n_obj = OBJPTR(n_item);
            detach(lvl_obj, n_item);

            /* See if he wants it */
            if (n_obj->o_type == SCROLL && n_obj->o_which == S_SCARE &&
                th->t_stats.s_intel < 16)
                wants_it = FALSE; /* Most monsters don't want a scare monster */
            else if (on(*th, ISSCAVENGE)) wants_it = TRUE;
            else {
                wants_it = FALSE;       /* Default case */
                switch (n_obj->o_type) {
                    case FOOD:  if(on(*th, CARRYFOOD))   wants_it = TRUE;
                    when GOLD:  if(on(*th, CARRYGOLD))   wants_it = TRUE;
                    when SCROLL:if(on(*th, CARRYSCROLL)) wants_it = TRUE;
                    when POTION:if(on(*th, CARRYPOTION)) wants_it = TRUE;
                    when RING:  if(on(*th, CARRYRING))   wants_it = TRUE;
                    when STICK: if(on(*th, CARRYSTICK))  wants_it = TRUE;
                    when MM:    if(on(*th, CARRYMISC))   wants_it = TRUE;
                    when ARMOR: if(on(*th, CARRYARMOR))  wants_it = TRUE;
                    when WEAPON:if(on(*th, CARRYWEAPON) ||
                                  (on(*th,CARRYDAGGER)&&n_obj->o_which==DAGGER))
                                        wants_it = TRUE;
                }
            }
            /*
             * The quartermaster doesn't sell cursed stuff so he won't
             * pick it up
             */
            if (on(*th, CANSELL) && (n_obj->o_flags & ISCURSED))
                wants_it = FALSE;

            /* If he doesn't want it, throw it away */
            if (wants_it == FALSE) {
                fall(n_item, FALSE);
                continue;
            }

            /* Otherwise, let's pick it up */
            if (n_obj->o_group) {
                for(o_item = th->t_pack; o_item != NULL; o_item = next(o_item)){
                    o_obj = OBJPTR(o_item);
                    if (o_obj->o_group == n_obj->o_group) {
                        o_obj->o_count += n_obj->o_count;
                        o_discard(n_item);
                        break;
                    }
                }
            }
            if (o_item == NULL) {       /* didn't find it */
                attach(th->t_pack, n_item);
            }
        }

        /* If there was anything here, we may have to update the screen */
        if (item_count) {
            if (cansee(ch_ret.y, ch_ret.x))
                mvwaddch(cw, ch_ret.y, ch_ret.x, mvinch(ch_ret.y, ch_ret.x));
            updpack(TRUE, th); /* Update the monster's encumberance, too */
        }
    }

    rch = mvwinch(stdscr, old_pos.y, old_pos.x); 
    if (th->t_oldch == floor && rch != floor && !isatrap(rch))
        mvwaddch(cw, old_pos.y, old_pos.x, rch);
    else
        mvwaddch(cw, old_pos.y, old_pos.x, th->t_oldch);
    sch = mvwinch(cw, ch_ret.y, ch_ret.x); /* What player sees */
    rch = mvwinch(stdscr, ch_ret.y, ch_ret.x); /* What's really there */

    /* If we have a tunneling monster, it may be making a tunnel */
    if (on(*th, CANTUNNEL)      &&
        (rch==SECRETDOOR || rch==WALL || rch==VERTWALL || rch==HORZWALL)) {
        unsigned char nch;       /* The new look to the tunnel */

        if (rch == WALL && levtype == OUTSIDE) nch = FLOOR;
        else if (rch == WALL) nch = PASSAGE;
        else if (levtype == MAZELEV || levtype == OUTSIDE) nch = FLOOR;
        else nch = DOOR;
        addch(nch);

        if (cansee(ch_ret.y, ch_ret.x)) sch = nch; /* Can player see this? */

        /* Does this make a new exit? */
        if (rch == VERTWALL || rch == HORZWALL) {
            struct linked_list *newroom;
            coord *exit;

            newroom = new_item(sizeof(coord));
            exit = DOORPTR(newroom);
            *exit = ch_ret;
            attach(new_room->r_exit, newroom);
        }
    }

    /* Mark if the monster is inside a wall */
    if (isrock(mvinch(ch_ret.y, ch_ret.x))) turn_on(*th, ISINWALL);
    else turn_off(*th, ISINWALL);

    /* If the monster can illuminate rooms, check for a change */
    if (on(*th, HASFIRE)) {
        register struct linked_list *fire_item;

        /* Is monster entering a room? */
        if (orig_rer != new_room && new_room != NULL) {
            fire_item = creat_item();   /* Get an item-only structure */
            ldata(fire_item) = (char *) th;

            attach(new_room->r_fires, fire_item);
            new_room->r_flags |= HASFIRE;

            if (cansee(ch_ret.y, ch_ret.x) && next(new_room->r_fires) == NULL)
                light(&hero);
        }

        /* Is monster leaving a room? */
        if (orig_rer != new_room && orig_rer != NULL) {
            /* Find the bugger in the list and delete him */
            for (fire_item = orig_rer->r_fires; fire_item != NULL;
                 fire_item = next(fire_item)) {
                if (THINGPTR(fire_item) == th)  {       /* Found him! */
                    detach(orig_rer->r_fires, fire_item);
                    destroy_item(fire_item);
                    if (orig_rer->r_fires == NULL) {
                        orig_rer->r_flags &= ~HASFIRE;
                        if (cansee(old_pos.y, old_pos.x))
                            light(&old_pos);
                    }
                    break;
                }
            }
        }
    }

    /* If monster is entering player's room and player can see it,
     * stop the player's running.
     */
    if (new_room != orig_rer && new_room != NULL  &&
        new_room == roomin(th->t_dest) && cansee(unc(ch_ret))    &&
        (off(*th, ISINVIS)     || on(player, CANSEE)) &&
        (off(*th, ISSHADOW)    || on(player, CANSEE)) &&
        (off(*th, CANSURPRISE) || ISWEARING(R_ALERT))) {
                running = FALSE;
                if (fight_flush) flushinp();
    }

    th->t_oldch = sch;

    /* Let's display those creatures that we can see. */
    if (cansee(unc(ch_ret)) &&
        off(*th, ISINWALL) &&
        !invisible(th))
        mvwaddch(cw, ch_ret.y, ch_ret.x, th->t_type);

    /* Record monster's last position (if new one is different) */
    if (!ce(ch_ret, old_pos)) th->t_oldpos = old_pos;

    /* If the monster is on a trap, trap it */
    sch = mvinch(ch_ret.y, ch_ret.x);
    if (isatrap(sch)) {
        if (cansee(ch_ret.y, ch_ret.x)) th->t_oldch = sch;
        be_trapped(th, &ch_ret);
    }
}

/* 
 * Get_hurl returns the weapon that the monster will "throw" if he has one 
 */

struct linked_list *
get_hurl(tp)
register struct thing *tp;
{
    struct linked_list *arrow=NULL, *bolt=NULL, *rock=NULL,
        *spear = NULL, *dagger=NULL, *dart=NULL, *aklad=NULL;
    register struct linked_list *pitem;
    register struct object *obj;
    bool bow=FALSE, crossbow=FALSE, sling=FALSE;

    for (pitem=tp->t_pack; pitem; pitem=next(pitem)) {
        obj = OBJPTR(pitem);
        if (obj->o_type == WEAPON)
            switch (obj->o_which) {
                case BOW:       bow = TRUE;
                when CROSSBOW:  crossbow = TRUE;
                when SLING:     sling = TRUE;
                when ROCK:      rock = pitem;
                when ARROW:     arrow = pitem;
                when BOLT:      bolt = pitem;
                when SPEAR:     spear = pitem;
                when DAGGER:
                    /* Don't throw the dagger if it's our last one */
                    if (obj->o_count > 1) dagger = pitem;
                when DART:      dart = pitem;
            }
        else if (obj->o_type == RELIC &&
                 obj->o_which == AXE_AKLAD)
                    aklad = pitem;
    }
    
    /* Do we have that all-powerful Aklad Axe? */
    if (aklad) return(aklad);

    /* Use crossbow bolt if possible */
    if (crossbow && bolt) return(bolt);
    if (bow && arrow) return(arrow);
    if (spear) return(spear);
    if (dagger) return(dagger);
    if (sling && rock) return(rock);
    if (dart) return(dart);
    return(NULL);
}

/*
 * runto:
 *      Set a monster running after something
 */

runto(runner, spot)
register struct thing *runner;
coord *spot;
{
    if (on(*runner, ISSTONE))
        return;

    /* If we are chasing a new creature, forget about thrown weapons */
    if (runner->t_dest && !ce(*runner->t_dest, *spot)) runner->t_wasshot=FALSE;

    /*
     * Start the beastie running
     */
    runner->t_dest = spot;
    turn_on(*runner, ISRUN);
    turn_off(*runner, ISDISGUISE);
}

/*
 * straight_shot:
 *      See if there is a straight line of sight between the two
 *      given coordinates.  If shooting is not NULL, it is a pointer
 *      to a structure which should be filled with the direction
 *      to shoot (if there is a line of sight).  If shooting, monsters
 *      get in the way.  Otherwise, they do not.
 */

bool
straight_shot(ery, erx, eey, eex, shooting)
register int ery, erx, eey, eex;
register coord *shooting;
{
    register int dy, dx;        /* Deltas */
    unsigned char ch;

    /* Does the monster have a straight shot at prey */
    if ((ery != eey) && (erx != eex) &&
        (abs(ery - eey) != abs(erx - eex))) return(FALSE);

    /* Get the direction to shoot */
    if (eey > ery) dy = 1;
    else if (eey == ery) dy = 0;
    else dy = -1;

    if (eex > erx) dx = 1;
    else if (eex == erx) dx = 0;
    else dx = -1;

    /* Make sure we have free area all the way to the player */
    ery += dy;
    erx += dx;
    while ((ery != eey) || (erx != eex)) {
        switch (ch = winat(ery, erx)) {
            case VERTWALL:
            case HORZWALL:
            case WALL:
            case DOOR:
            case SECRETDOOR:
            case FOREST:
                return(FALSE);
            default:
                if (shooting && isalpha(ch)) return(FALSE);
        }
        ery += dy;
        erx += dx;
    }

    if (shooting) {     /* If we are shooting -- put in the directions */
        shooting->y = dy;
        shooting->x = dx;
    }
    return(TRUE);
}

