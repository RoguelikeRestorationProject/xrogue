/*
    potions.c - Functions for dealing with potions

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
#include "rogue.h"

/*
 * add_abil is an array of functions used to change attributes.  It must be
 * ordered according to the attribute definitions in rogue.h.
 */

int (*add_abil[NUMABILITIES])() = {
    add_intelligence, add_strength, add_wisdom, add_dexterity,
    add_constitution, add_charisma
};

/*
 * res_abil is an array of functions used to change attributes.  It must be
 * ordered according to the attribute definitions in rogue.h.
 */

int (*res_abil[NUMABILITIES])() = {
    res_intelligence, res_strength, res_wisdom, res_dexterity,
    res_constitution, res_charisma
};

/*
 * Increase player's constitution
 */

int
add_constitution(change)
int change;
{
    /* Do the potion */
    if (change < 0) {
        msg("You feel less healthy now.");
        pstats.s_const += change;
        if (pstats.s_const < 1) {
        pstats.s_hpt = -1;
        msg("You collapse!  --More--");
        wait_for(' ');
            death(D_CONSTITUTION);
    }
    }
    else {
        msg("You feel healthier now.");
        pstats.s_const = min(pstats.s_const + change, MAXATT);
    }

    /* Adjust the maximum */
    if (max_stats.s_const < pstats.s_const)
        max_stats.s_const = pstats.s_const;

    return(0);
}

/*
 * Increase player's charisma
 */

int
add_charisma(change)
int change;
{
    /* Do the potion */
    if (change < 0) msg("You feel less attractive now.");
    else msg("You feel more attractive now.");

    pstats.s_charisma += change;
    if (pstats.s_charisma > MAXATT) pstats.s_charisma = MAXATT;
    else if (pstats.s_charisma < 3) pstats.s_charisma = 3;

    /* Adjust the maximum */
    if (max_stats.s_charisma < pstats.s_charisma)
        max_stats.s_charisma = pstats.s_charisma;

    return(0);
}

/*
 * Increase player's dexterity
 */

int
add_dexterity(change)
int change;
{
    int ring_str;       /* Value of ring strengths */

    /* Undo any ring changes */
    ring_str = ring_value(R_ADDHIT);
    pstats.s_dext -= ring_str;

    /* Now do the potion */
    if (change < 0) msg("You feel less dextrous now.");
    else msg("You feel more dextrous now.  Watch those hands!");

    pstats.s_dext += change;
    if (pstats.s_dext > MAXATT) pstats.s_dext = MAXATT;
    else if (pstats.s_dext < 3) pstats.s_dext = 3;

    /* Adjust the maximum */
    if (max_stats.s_dext < pstats.s_dext)
        max_stats.s_dext = pstats.s_dext;

    /* Now put back the ring changes */
    if (ring_str)
        pstats.s_dext += ring_str;

    return(0);
}

/*
 * add_haste:
 *      add a haste to the player
 */

add_haste(blessed)
bool blessed;
{
    int hasttime;

    if (player.t_ctype == C_MONK) { /* monks cannot be slowed or hasted */
        msg(nothing);
        return;
    }

    if (blessed) hasttime = HASTETIME*2;
    else hasttime = HASTETIME;

    if (on(player, ISSLOW)) { /* Is person slow? */
        extinguish(noslow);
        noslow();

        if (blessed) hasttime = HASTETIME/2;
        else return;
    }

    if (on(player, ISHASTE)) {
        msg("You faint from exhaustion.");
        player.t_no_move += movement(&player) * rnd(hasttime);
        player.t_action = A_FREEZE;
        lengthen(nohaste, roll(hasttime,hasttime));
    }
    else {
        msg("You feel yourself moving %sfaster.", blessed ? "much " : "");
        turn_on(player, ISHASTE);
        fuse(nohaste, (VOID *)NULL, roll(hasttime, hasttime), AFTER);
    }
}

/*
 * Increase player's intelligence
 */

int
add_intelligence(change)
int change;
{
    int ring_str;       /* Value of ring strengths */

    /* Undo any ring changes */
    ring_str = ring_value(R_ADDINTEL);
    pstats.s_intel -= ring_str;

    /* Now do the potion */
    if (change < 0) msg("You feel slightly less intelligent now.");
    else msg("You feel more intelligent now.  What a mind!");

    pstats.s_intel += change;
    if (pstats.s_intel > MAXATT) pstats.s_intel = MAXATT;
    else if (pstats.s_intel < 3) pstats.s_intel = 3;

    /* Adjust the maximum */
    if (max_stats.s_intel < pstats.s_intel)
            max_stats.s_intel = pstats.s_intel;

    /* Now put back the ring changes */
    if (ring_str)
        pstats.s_intel += ring_str;

    return(0);
}

/*
 * this routine makes the hero move slower 
 */

add_slow()
{
    /* monks cannot be slowed or hasted */
    if (player.t_ctype == C_MONK || ISWEARING(R_FREEDOM)) { 
        msg(nothing);
        return;
    }

    if (on(player, ISHASTE)) { /* Already sped up */
        extinguish(nohaste);
        nohaste();
    }
    else {
        msg("You feel yourself moving %sslower.",
                on(player, ISSLOW) ? "even " : "");
        if (on(player, ISSLOW))
            lengthen(noslow, roll(HASTETIME,HASTETIME));
        else {
            turn_on(player, ISSLOW);
            fuse(noslow, (VOID *)NULL, roll(HASTETIME,HASTETIME), AFTER);
        }
    }
}

/*
 * Increase player's strength
 */

int
add_strength(change)
int change;
{

    if (change < 0) {
        msg("You feel slightly weaker now.");
        chg_str(change);
    }
    else {
        msg("You feel stronger now.  What bulging muscles!");
        chg_str(change);
    }
    return(0);
}

/*
 * Increase player's wisdom
 */

int
add_wisdom(change)
int change;
{
    int ring_str;       /* Value of ring strengths */

    /* Undo any ring changes */
    ring_str = ring_value(R_ADDWISDOM);
    pstats.s_wisdom -= ring_str;

    /* Now do the potion */
    if (change < 0) msg("You feel slightly less wise now.");
    else msg("You feel wiser now.  What a sage!");

    pstats.s_wisdom += change;
    if (pstats.s_wisdom > MAXATT) pstats.s_wisdom = MAXATT;
    else if (pstats.s_wisdom < 3) pstats.s_wisdom = 3;

    /* Adjust the maximum */
    if (max_stats.s_wisdom < pstats.s_wisdom)
        max_stats.s_wisdom = pstats.s_wisdom;

    /* Now put back the ring changes */
    if (ring_str)
        pstats.s_wisdom += ring_str;

    return(0);
}

quaff(which, kind, flags, is_potion)
int which;
int kind;
int flags;
bool is_potion;
{
    register struct object *obj;
    register struct linked_list *item, *titem;
    register struct thing *th;
    bool cursed, blessed;

    blessed = FALSE;
    cursed = FALSE;
    item = NULL;

    if (which < 0) {    /* figure out which ourselves */
        /* This is a potion.  */
        if (player.t_action != C_QUAFF) {
            int units;

            item = get_item(pack, "quaff", QUAFFABLE, FALSE, FALSE);

            /*
             * Make certain that it is somethings that we want to drink
             */
            if (item == NULL)
                return;

            /* How long does it take to quaff? */
            units = usage_time(item);
            if (units < 0) return;

            player.t_using = item;      /* Remember what it is */
            player.t_no_move = units * movement(&player);
            if ((OBJPTR(item))->o_type == POTION) player.t_action = C_QUAFF;
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

        flags = obj->o_flags;
        which = obj->o_which;
        kind = obj->o_kind;
    }
    cursed = flags & ISCURSED;
    blessed = flags & ISBLESSED;

    switch(which) {
        case P_CLEAR:
            if (cursed) {
                confus_player();
            }
            else {
                if (blessed) {  /* Make player immune for the whole game */
                    extinguish(unclrhead);  /* If we have a fuse, put it out */
                    msg("A strong blue aura surrounds your head.");
                }
                else {  /* Just light a fuse for how long player is safe */
                    if (off(player, ISCLEAR)) {
                        fuse(unclrhead, (VOID *)NULL, CLRDURATION, AFTER);
                        msg("A faint blue aura surrounds your head.");
                    }
                    else {  /* If we have a fuse lengthen it, else we
                             * are permanently clear.
                             */
                        if (find_slot(unclrhead) == 0)
                            msg("Your blue aura continues to glow strongly.");
                        else {
                            lengthen(unclrhead, CLRDURATION);
                            msg("Your blue aura brightens for a moment.");
                        }
                    }
                }
                turn_on(player, ISCLEAR);
                /* If player is confused, unconfuse him */
                if (on(player, ISHUH)) {
                    extinguish(unconfuse);
                    unconfuse();
                }
            }
        when P_HEALING:
            if (cursed) {
                msg("You feel worse now.");
                pstats.s_hpt -= roll(pstats.s_lvl, char_class[player.t_ctype].hit_pts);
                if (pstats.s_hpt < 1) {
            pstats.s_hpt = -1;
            msg("You're life passes before your eyes..  --More--");
            wait_for(' ');
                    death(D_POTION);
        }
            }
            else {
                if (blessed) {
                    pstats.s_hpt += roll(pstats.s_lvl+1, char_class[player.t_ctype].hit_pts);
                    if (pstats.s_hpt > max_stats.s_hpt) {
                        pstats.s_hpt = ++max_stats.s_hpt;
                        pstats.s_hpt = ++max_stats.s_hpt;
            }
                    if (on(player, ISHUH)) {
                        extinguish(unconfuse);
                        unconfuse();
                    }
                }
                else {
                    pstats.s_hpt += roll(pstats.s_lvl+1, char_class[player.t_ctype].hit_pts/2);
                    if (pstats.s_hpt > max_stats.s_hpt)
                        pstats.s_hpt = ++max_stats.s_hpt;
                }
                msg("You begin to feel %sbetter.",
                        blessed ? "much " : "");
                sight();
                if (is_potion) p_know[P_HEALING] = TRUE;
            }
        when P_ABIL:
            /* If it is cursed, we take a point away */
            if (cursed) {
                if (ISWEARING(R_SUSABILITY)) {
                    msg(nothing);
                    break;
                }
                else (*add_abil[kind])(-1);
            }

            /* Otherwise we add points */
            else (*add_abil[kind])(blessed ? 3 : 1);

            if (is_potion) p_know[P_ABIL] = TRUE;
        when P_MFIND:
            /*
             * Potion of monster detection, if there are monters, detect them
             */
            if (mlist != NULL)
            {
                register struct thing *tp;
                register struct linked_list *item;

                wclear(hw);
                for (item=mlist; item!=NULL; item=next(item)) {
                    tp = THINGPTR(item);
                    if (on(*tp, NODETECT))
                        continue;
                    if (off(*tp, ISRUN))/* turn off only on sleeping ones */
                        turn_off(*tp, CANSURPRISE);
                    mvwaddch(hw, tp->t_pos.y, tp->t_pos.x, 
                             monsters[tp->t_index].m_appear);
                }
                rmmsg();
                overlay(hw,cw);
                draw(cw);
                msg("You begin to sense the presence of monsters.");
                if (is_potion) p_know[P_MFIND] = TRUE;
            }
            else
                msg("You have a strange feeling for a moment, then it passes.");
        when P_TFIND:
            /*
             * Potion of magic detection.  Show the potions and scrolls
             */
            {
                register struct linked_list *mobj;
                register struct object *tp;
                bool show;

                show = FALSE;
                wclear(hw);
                for (mobj = lvl_obj; mobj != NULL; mobj = next(mobj)) {
                    tp = OBJPTR(mobj);
                    if (is_magic(tp)) {
                        char mag_type=MAGIC;

                        /* Mark cursed items or bad weapons */
                        if ((tp->o_flags & ISCURSED) ||
                            (tp->o_type == WEAPON &&
                             (tp->o_hplus < 0 || tp->o_dplus < 0)))
                                mag_type = CMAGIC;
                        else if ((tp->o_flags & ISBLESSED) ||
                                 (tp->o_type == WEAPON &&
                                  (tp->o_hplus > 0 || tp->o_dplus > 0)))
                                        mag_type = BMAGIC;
                        show = TRUE;
                        mvwaddch(hw, tp->o_pos.y, tp->o_pos.x, mag_type);
                    }
                }
                for (titem = mlist; titem != NULL; titem = next(titem)) {
                    register struct linked_list *pitem;

                    th = THINGPTR(titem);
                    if (on(*th, NODETECT)) continue;
                    for(pitem = th->t_pack; pitem != NULL; pitem = next(pitem)){
                        tp = OBJPTR(pitem);
                        if (is_magic(tp)) {
                            char mag_type=MAGIC;

                            /* Mark cursed items or bad weapons */
                            if ((tp->o_flags & ISCURSED) ||
                                (tp->o_type == WEAPON &&
                                 (tp->o_hplus < 0 || tp->o_dplus < 0)))
                                    mag_type = CMAGIC;
                            else if ((tp->o_flags & ISBLESSED) ||
                                     (tp->o_type == WEAPON &&
                                      (tp->o_hplus > 0 || tp->o_dplus > 0)))
                                            mag_type = BMAGIC;
                            show = TRUE;
                            mvwaddch(hw, th->t_pos.y, th->t_pos.x, mag_type);
                        }
                    }
                }
                if (show) {
                    if (is_potion) p_know[P_TFIND] = TRUE;
                    rmmsg();
                    overlay(hw,cw);
                    draw(cw);
                    msg("You sense the presence of magic on this level.");
                    break;
                }
                else
                    msg("You have a strange feeling for a moment, then it passes.");
            }
        when P_SEEINVIS:
            if (cursed) {
                if (!find_slot(sight))
                {
                    msg("A cloak of darkness falls around you.");
                    turn_on(player, ISBLIND);
                    fuse(sight, (VOID *)NULL, SEEDURATION, AFTER);
                    light(&hero);
                }
                else
                    msg("The darkness around you thickens. ");
                    lengthen(sight, SEEDURATION);
            }
            else {
                if (off(player, CANSEE)) {
                    turn_on(player, CANSEE);
                    msg("Your eyes begin to tingle.");
                    fuse(unsee, (VOID *)NULL, blessed ? SEEDURATION*3 :SEEDURATION, AFTER);
                    light(&hero);
                }
                else if (find_slot(unsee) != 0) {
                    msg("You eyes continue to tingle.");
                    lengthen(unsee, blessed ? SEEDURATION*3 : SEEDURATION);
        }
                sight();
            }
        when P_PHASE:
            if (cursed) {
                msg("You can't move.");
                player.t_no_move = movement(&player) * FREEZETIME;
                player.t_action = A_FREEZE;
            }
            else {
                int duration;

                if (blessed) duration = 3;
                else duration = 1;

                if (on(player, CANINWALL))
                    lengthen(unphase, duration*PHASEDURATION);
                else {
                    fuse(unphase, (VOID *)NULL, duration*PHASEDURATION, AFTER);
                    turn_on(player, CANINWALL);
                }
                msg("You feel %slight-headed!",
                    blessed ? "very " : "");
            }
        when P_FLY: {
            int duration;
            bool say_message;

            say_message = TRUE;

            if (blessed) duration = 3;
            else duration = 1;

            if (on(player, ISFLY)) {
                if (find_slot(land))
                    lengthen(land, duration*FLYTIME);
                else {
                    msg("Nothing happens.");    /* Flying by cloak */
                    say_message = FALSE;
                }
            }
            else {
                fuse(land, (VOID *)NULL, duration*FLYTIME, AFTER);
                turn_on(player, ISFLY);
            }
            if (say_message) {
                if (is_potion) p_know[P_FLY] = TRUE;
                msg("You feel %slighter than air!", blessed ? "much " : "");
            }
        }
        when P_RAISE:
            if (cursed) lower_level(D_POTION);
            else {
                msg("You suddenly feel %smore skillful",
                        blessed ? "much " : "");
                p_know[P_RAISE] = TRUE;
                raise_level();
                do_panic(NULL);         /* this startles them */
                if (blessed) raise_level();
            }
        when P_HASTE:
            if (cursed) {       /* Slow player down */
                add_slow();
            }
            else {
                add_haste(blessed);
                if (is_potion) p_know[P_HASTE] = TRUE;
            }
        when P_RESTORE: {
            register int i, howmuch, strength_tally;

            msg("Hey, this tastes great.  It make you feel %swarm all over.",
                blessed ? "really " : "");
            howmuch = blessed ? 3 : 1;

            for (i=0; i<NUMABILITIES; i++) {
                if (i == A_STRENGTH) {
                    if (lost_str) {
                        if (lost_str > howmuch) {
                            lost_str -= howmuch;

                            /*
                             * Save the lost strength.  We have to set
                             * temporarilty set it to 0 so that res_strength
                             * will not restore it.
                             */
                            strength_tally = lost_str;
                            lost_str = 0;
                            res_strength(howmuch);
                            lost_str = strength_tally;
                        }
                        else {
                        lost_str = 0;
                            extinguish(res_strength);
                            res_strength(howmuch);
                        }
                    }
                    else res_strength(howmuch);
                }
                else (*res_abil[i])(howmuch);
            }
        }
        when P_INVIS:
            if (off(player, ISINVIS)) {
                turn_on(player, ISINVIS);
                msg("You have a tingling feeling all over your body");
                fuse(appear, (VOID *)NULL, blessed ? GONETIME*3 : GONETIME, AFTER);
                PLAYER = IPLAYER;
                light(&hero);
            }
            else {
                if (find_slot(appear)) {
                    msg("Your tingling feeling surges.");
                    lengthen(appear, blessed ? GONETIME*3 : GONETIME);
                }
                else msg("Nothing happens.");   /* Using cloak */
            }

        when P_FFIND:
            {
                register struct linked_list *nitem;
                register struct object *nobj;
                bool show;

                show = FALSE;
                wclear(hw);
                for (nitem = lvl_obj; nitem != NULL; nitem = next(nitem)) {
                    nobj = OBJPTR(nitem);
                    if (nobj->o_type == FOOD) {
                        show = TRUE;
                        mvwaddch(hw, nobj->o_pos.y, nobj->o_pos.x, FOOD);
                    }
                }
                for (nitem = mlist; nitem != NULL; nitem = next(nitem)) {
                    register struct linked_list *pitem;
                    register struct thing *th;

                    th = THINGPTR(nitem);
                    if (on(*th, NODETECT)) continue;
                    for(pitem = th->t_pack; pitem != NULL; pitem = next(pitem)){
                        nobj = OBJPTR(pitem);
                        if (nobj->o_type == FOOD) {
                            show = TRUE;
                            mvwaddch(hw, th->t_pos.y, th->t_pos.x, FOOD);
                        }
                    }
                }
                if (is_potion) p_know[P_FFIND] = TRUE;
                if (show) {
                    rmmsg();
                    msg("Your nose tingles.");
                    rmmsg();
                    overlay(hw,cw);
                    draw(cw);
                    msg("You sense the presence of food on this level.");
                }
                else
                    msg("You have a strange feeling for a moment, then it passes.");
            }

        when P_SKILL:
            if (cursed) {
                msg("You feel less skillful.");

                /* Does he currently have an artifical skill? */
                if (!find_slot(unskill)) {      /* No skill */
                    pstats.s_lvladj = -2;
                    pstats.s_lvl += pstats.s_lvladj;
                    fuse(unskill, (VOID *)NULL, SKILLDURATION, AFTER);
                }
                else {  /* Has an artifical skill */
                    /* Is the skill beneficial? */
                    if (pstats.s_lvladj > 0) {
                        /* Decrease the previous skill advantage */
                        pstats.s_lvl -= 2;
                        pstats.s_lvladj -= 2;

                        /* If there is now a negative skill, lengthen time */
                        if (pstats.s_lvladj < 0)
                            lengthen(unskill, SKILLDURATION);

                        /* If there is no skill advantage, unfuse us */
                        else if (pstats.s_lvladj == 0) extinguish(unskill);
                    }
                    else {      /* Already bad */
                        /* Make it a little worse, and lengthen it */
                        pstats.s_lvl--;
                        pstats.s_lvladj--;
                        lengthen(unskill, SKILLDURATION);
                    }
                }

                /* Is our level too low now? */
                if (pstats.s_lvl < 1) {
            pstats.s_hpt = -1;
            msg("You cough, choke, and finally die.  --More--");
            wait_for(' ');
            death(D_POTION);
        }
            }
            else {
                int adjust;

                msg("You feel more skillful.");
                max_stats.s_hpt++;
                pstats.s_hpt++;

                /* Get the adjustment */
                if (blessed)
                    adjust = rnd(4) + 2;
                else
                    adjust = rnd(2) + 1;

        /* The Fighter likes this */
            if (player.t_ctype == C_FIGHTER) {
                    max_stats.s_hpt++;
                    pstats.s_hpt++;
                    adjust = rnd(2) + 1;
            }

                /* Does he currently have an artifical skill? */
                if (!find_slot(unskill)) {
                    pstats.s_lvladj = adjust;
                    pstats.s_lvl += pstats.s_lvladj;
                    fuse(unskill, (VOID *)NULL, 
                         blessed ? SKILLDURATION*2 : SKILLDURATION, AFTER);
                }
                else {  /* Has an artifical skill */
                    /* Is the skill detrimental? */
                    if (pstats.s_lvladj < 0) {
                        /* Decrease the previous skill advantage */
                        pstats.s_lvl += adjust;
                        pstats.s_lvladj += adjust;

                        /* If there is now a positive skill, lengthen time */
                        if (pstats.s_lvladj < 0)
                            lengthen(unskill, SKILLDURATION);

                        /* If there is no skill advantage, unfuse us */
                        else if (pstats.s_lvladj == 0) extinguish(unskill);
                    }
                    else {      /* Already good */
                        /*
                         * Make the skill the maximum of the current good
                         * skill and what the adjust would give him.
                         */
                        pstats.s_lvl -= pstats.s_lvladj;
                        pstats.s_lvladj = max(pstats.s_lvladj, adjust);
                        pstats.s_lvl += pstats.s_lvladj;
                        lengthen(unskill,
                                 blessed ? SKILLDURATION*2 : SKILLDURATION);
                    }
                }
            }

        when P_FIRE: {
            int duration;
            bool say_message;

            say_message = TRUE;

            if (blessed) duration = 8;
            else duration = 3;

            if (on(player, NOFIRE)) {
            if (cursed) {
            msg("You quench your thirst. ");
            say_message = FALSE;
            }
                else if (find_slot(nofire)) {
                    lengthen(nofire, duration*FIRETIME);
            msg("Your feeling of fire resistance increases. ");
            say_message = FALSE;
        }
                else {
                    msg("You experience heat waves. ");    /* has on a ring */
                    say_message = FALSE;
                }
            }
        else if (cursed) {
        msg("You quench your thirst. ");
        say_message = FALSE;
        }
            else {
                fuse(nofire, (VOID *)NULL, duration*FIRETIME, AFTER);
                turn_on(player, NOFIRE);
            }
            if (say_message)  {
                if (is_potion) p_know[P_FIRE] = TRUE;
                msg("You feel %sfire resistant", blessed ? "very " : "");
            }
        }
        when P_COLD: {
            int duration;
            bool say_message;

            say_message = TRUE;

            if (blessed) duration = 8;
            else duration = 3;

            if (on(player, NOCOLD)) {
            if (cursed) {
            msg("You quench your thirst. ");
            say_message = FALSE;
            }
                else if (find_slot(nocold)) {
                    lengthen(nocold, duration*COLDTIME);
            msg("Your feeling of cold resistance increases. ");
            say_message = FALSE;
        }
                else {
                    msg("You feel a cold chill. ");    /* has on a ring */
                    say_message = FALSE;
                }
            }
        else if (cursed) {
        msg("You quench your thirst. ");
        say_message = FALSE;
        }
            else {
                fuse(nocold, (VOID *)NULL, duration*COLDTIME, AFTER);
                turn_on(player, NOCOLD);
            }
            if (say_message)  {
                if (is_potion) p_know[P_COLD] = TRUE;
                msg("You feel %scold resistant", blessed ? "very " : "");
            }
        }
        when P_LIGHTNING: {
            int duration;
            bool say_message;

            say_message = TRUE;

            if (blessed) duration = 8;
            else duration = 3;

            if (on(player, NOBOLT)) {
            if (cursed) {
            msg("You quench your thirst. ");
            say_message = FALSE;
            }
                else if (find_slot(nobolt)) {
                    lengthen(nobolt, duration*BOLTTIME);
            msg("Your blue skin deepens in hue. ");
            say_message = FALSE;
        }
        else msg(nothing);
            }
        else if (cursed) {
        msg("You quench your thirst. ");
        say_message = FALSE;
        }
            else {
                fuse(nobolt, (VOID *)NULL, duration*BOLTTIME, AFTER);
                turn_on(player, NOBOLT);
            }
            if (say_message) {
                if (is_potion) p_know[P_LIGHTNING] = TRUE;
                msg("Your skin turns %sblue!", blessed ? "very " : "");
        }
        }
        when P_POISON:
            if (!save(VS_POISON, &player, -2)) {
                msg("You feel very sick now.");
                pstats.s_hpt /= 2;
                if (!ISWEARING(R_SUSABILITY))
                    pstats.s_const--;
            }
            else {
                msg("You feel sick now.");
                pstats.s_hpt -= (pstats.s_hpt / 3);
            }
            if (pstats.s_const < 1 || pstats.s_hpt < 1) {
        pstats.s_hpt = -1;
        msg("You didn't survive!  --More--");
        wait_for(' ');
                death(D_POISON);
        }
        otherwise:
            msg("What an odd tasting potion!");
            return;
    }
    status(FALSE);
    if (is_potion && item && p_know[which] && p_guess[which])
    {
        free(p_guess[which]);
        p_guess[which] = NULL;
    }
    else if (is_potion                  && 
             !p_know[which]             && 
             item                       &&
             askme                      &&
             (flags & ISKNOW) == 0      &&
             (flags & ISPOST) == 0      &&
             p_guess[which] == NULL) {
        nameitem(item, FALSE);
    }
    if (item != NULL) o_discard(item);
    updpack(TRUE, &player);
}


/*
 * res_dexterity:
 *      Restore player's dexterity
 *      if called with zero the restore fully
 */

int
res_dexterity(howmuch)
int howmuch;
{
    short save_max;
    int ring_str;

    if (howmuch < 0) return(0);

    /* Discount the ring value */
    ring_str = ring_value(R_ADDHIT);
    pstats.s_dext -= ring_str;

    if (pstats.s_dext < max_stats.s_dext ) {
        if (howmuch == 0)
            pstats.s_dext = max_stats.s_dext;
        else
            pstats.s_dext = min(pstats.s_dext+howmuch, max_stats.s_dext);
    }

    /* Redo the rings */
    if (ring_str) {
        save_max = max_stats.s_dext;
        pstats.s_dext += ring_str;
        max_stats.s_dext = save_max;
    }
    return(0);
}

/*
 * res_intelligence:
 *      Restore player's intelligence
 */

int
res_intelligence(howmuch)
int howmuch;
{
    short save_max;
    int ring_str;

    if (howmuch <= 0) return(0);

    /* Discount the ring value */
    ring_str = ring_value(R_ADDINTEL);
    pstats.s_intel -= ring_str;

    pstats.s_intel = min(pstats.s_intel + howmuch, max_stats.s_intel);

    /* Redo the rings */
    if (ring_str) {
        save_max = max_stats.s_intel;
        pstats.s_intel += ring_str;
        max_stats.s_intel = save_max;
    }
    return(0);
}

/*
 * res_wisdom:
 *      Restore player's wisdom
 */

int
res_wisdom(howmuch)
int howmuch;
{
    short save_max;
    int ring_str;

    if (howmuch <= 0) return(0);

    /* Discount the ring value */
    ring_str = ring_value(R_ADDWISDOM);
    pstats.s_wisdom -= ring_str;

    pstats.s_wisdom = min(pstats.s_wisdom + howmuch, max_stats.s_wisdom);

    /* Redo the rings */
    if (ring_str) {
        save_max = max_stats.s_wisdom;
        pstats.s_wisdom += ring_str;
        max_stats.s_wisdom = save_max;
    }
    return(0);
}

/*
 * res_constitution:
 *      Restore the players constitution.
 */

int
res_constitution(howmuch)
int howmuch;
{
    if (howmuch > 0)
        pstats.s_const = min(pstats.s_const + howmuch, max_stats.s_const);

    return(0);
}

/*
 * res_charisma:
 *      Restore the players charisma.
 */

int
res_charisma(howmuch)
int howmuch;
{
    if (howmuch > 0)
        pstats.s_charisma =
            min(pstats.s_charisma + howmuch, max_stats.s_charisma);

    return(0);
}
