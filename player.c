/*
    player.c - functions for dealing with special player abilities

    XRogue: Expeditions into the Dungeons of Doom
    Copyright (C) 1991 Robert Pietkivitch
    All rights reserved.
    
    Based on "Advanced Rogue"
    Copyright (C) 1984, 1985 Michael Morgan, Ken Dalka and AT&T
    All rights reserved.

    See the file LICENSE.TXT for full copyright and licensing information.
*/

#include <ctype.h>
#include <curses.h>
#include "rogue.h"

/*
 * affect:
 *      cleric affecting undead
 */

affect()
{
    register struct linked_list *item;
    register struct thing *tp;
    register char *mname;
    bool see;
    coord new_pos;
    int lvl;

    if (!(player.t_ctype == C_CLERIC  ||
          (player.t_ctype == C_PALADIN && pstats.s_lvl > 4) ||
          cur_relic[HEIL_ANKH] != 0)) {
        msg("You cannot affect undead.");
        return;
    }

    new_pos.y = hero.y + player.t_newpos.y;
    new_pos.x = hero.x + player.t_newpos.x;

    if (cansee(new_pos.y, new_pos.x)) see = TRUE;
    else see = FALSE;

    /* Anything there? */
    if (new_pos.y < 0 || new_pos.y > lines-3 ||
        new_pos.x < 0 || new_pos.x > cols-1 ||
        mvwinch(mw, new_pos.y, new_pos.x) == ' ') {
        msg("Nothing to affect.");
        return;
    }

    if ((item = find_mons(new_pos.y, new_pos.x)) == 0) {
        debug("Affect what @ %d,%d?", new_pos.y, new_pos.x);
        return;
    }
    tp = THINGPTR(item);
    mname = monster_name(tp);

    if (on(player, ISINVIS) && off(*tp, CANSEE)) {
        msg("%s%s cannot see you", see ? "The " : "It",
            see ? mname : "");
        return;
    }

    if (off(*tp, TURNABLE) || on(*tp, WASTURNED)) 
        goto annoy;
    turn_off(*tp, TURNABLE);

    lvl = pstats.s_lvl;
    if (player.t_ctype == C_PALADIN && cur_relic[HEIL_ANKH] == 0) {
        lvl -= 4;
    }
    /* Can cleric kill it? */
    if (lvl >= 3 * tp->t_stats.s_lvl) {
        unsigned long test;      /* For overflow check */

        msg("You have destroyed %s%s.", see ? "the " : "it", see ? mname : "");
        test = pstats.s_exp + tp->t_stats.s_exp;

        /* Be sure there is no overflow before increasing experience */
        if (test > pstats.s_exp) pstats.s_exp = test;
        killed(item, FALSE, TRUE, TRUE);
        check_level();
        return;
    }

    /* Can cleric turn it? */
    if (rnd(100) + 1 >
         (100 * ((2 * tp->t_stats.s_lvl) - lvl)) / lvl) {
        unsigned long test;      /* Overflow test */

        /* Make the monster flee */
        turn_on(*tp, WASTURNED);        /* No more fleeing after this */
        turn_on(*tp, ISFLEE);
        runto(tp, &hero);

        /* Disrupt it */
        dsrpt_monster(tp, TRUE, TRUE);

        /* Let player know */
        msg("You have turned %s%s.", see ? "the " : "it", see ? mname : "");

        /* get points for turning monster -- but check overflow first */
        test = pstats.s_exp + tp->t_stats.s_exp/2;
        if (test > pstats.s_exp) pstats.s_exp = test;
        check_level();

        /* If monster was suffocating, stop it */
        if (on(*tp, DIDSUFFOCATE)) {
            turn_off(*tp, DIDSUFFOCATE);
            extinguish(suffocate);
        }

        /* If monster held us, stop it */
        if (on(*tp, DIDHOLD) && (--hold_count == 0))
                turn_off(player, ISHELD);
        turn_off(*tp, DIDHOLD);

        /* It is okay to turn tail */
        tp->t_oldpos = tp->t_pos;

        return;
    }

    /* Otherwise -- no go */
annoy:
    if (see && tp->t_stats.s_intel > 16)
        msg("%s laughs at you...", prname(mname, TRUE));
    else
        msg("You do not affect %s%s.", see ? "the " : "it", see ? mname : "");

    /* Annoy monster */
   if (off(*tp, ISFLEE)) runto(tp, &hero);
}

/* 
 * the cleric asks his deity for a spell
 */

pray()
{
    register int num_prayers, prayer_ability, which_prayer;

    which_prayer = num_prayers = prayer_ability =  0;

    if (player.t_ctype != C_CLERIC  && player.t_ctype != C_PALADIN &&
        cur_relic[HEIL_ANKH] == 0) {
            msg("You are not permitted to pray.");
            return;
    }
    if (cur_misc[WEAR_CLOAK] != NULL &&
        cur_misc[WEAR_CLOAK]->o_which == MM_R_POWERLESS) {
        msg("You can't seem to pray!");
        return;
    }

    prayer_ability = pstats.s_lvl * pstats.s_wisdom - 5;
    if (player.t_ctype != C_CLERIC)
        prayer_ability /= 2;

    if (cur_relic[HEIL_ANKH]) prayer_ability += 75;

    if (player.t_action != C_PRAY) {
        num_prayers = 0;

        /* Get the number of avilable prayers */
        if (pstats.s_wisdom > 16) 
            num_prayers += pstats.s_wisdom - 16;

        num_prayers += pstats.s_lvl;
        if (cur_relic[HEIL_ANKH])
            num_prayers += pstats.s_wisdom - 18;

        if (player.t_ctype != C_CLERIC) 
            num_prayers /= 2;

        if (num_prayers > MAXPRAYERS) 
            num_prayers = MAXPRAYERS;
        if (num_prayers < 1) {
            msg("You are not permitted to pray yet.");
            return;
        }

        /* Prompt for prayer */
        if (pick_spell( cleric_spells, 
                        prayer_ability, 
                        num_prayers, 
                        pray_time,
                        "offer",
                        "prayer"))
            player.t_action = C_PRAY;

        return;
    }

    /* We've waited our required praying time. */
    which_prayer = player.t_selection;
    player.t_selection = 0;
    player.t_action = A_NIL;

    if (cleric_spells[which_prayer].s_cost + pray_time > prayer_ability) {
        msg("Your prayer fails.");
        return;
    }

    msg("Your prayer has been granted. ");

    if (cleric_spells[which_prayer].s_type == TYP_POTION)
        quaff(          cleric_spells[which_prayer].s_which,
                        NULL,
                        cleric_spells[which_prayer].s_flag,
                        FALSE);
    else if (cleric_spells[which_prayer].s_type == TYP_SCROLL)
        read_scroll(    cleric_spells[which_prayer].s_which,
                        cleric_spells[which_prayer].s_flag,
                        FALSE);
    else if (cleric_spells[which_prayer].s_type == TYP_STICK) {
         if (!player_zap(cleric_spells[which_prayer].s_which,
                         cleric_spells[which_prayer].s_flag)) {
             after = FALSE;
             return;
         }
    }
    pray_time += cleric_spells[which_prayer].s_cost;
}

/*
 * the magician is going to try and cast a spell
 */

cast()
{
    register int spell_ability, which_spell, num_spells;

    if (player.t_ctype != C_MAGICIAN && player.t_ctype != C_RANGER) {
        msg("You are not permitted to cast spells.");
        return;
    }
    if (cur_misc[WEAR_CLOAK] != NULL &&
        cur_misc[WEAR_CLOAK]->o_which == MM_R_POWERLESS) {
        msg("You can't seem to cast spells!");
        return;
    }
    spell_ability = pstats.s_lvl * pstats.s_intel - 5;
    if (player.t_ctype != C_MAGICIAN)
        spell_ability /= 2;

    if (player.t_action != C_CAST) {
        /* 
         * Get the number of avilable spells 
         */
        num_spells = 0;
        if (pstats.s_intel > 16) 
            num_spells += pstats.s_intel - 16;

        num_spells += pstats.s_lvl;
        if (player.t_ctype != C_MAGICIAN) 
            num_spells /= 2;
        if (num_spells > MAXSPELLS)
            num_spells = MAXSPELLS;
        if (num_spells < 1) {
            msg("You are not allowed to cast spells yet.");
            return;
        }

    /* prompt for spell */
        if (pick_spell( magic_spells, 
                        spell_ability, 
                        num_spells, 
                        spell_power,
                        "cast",
                        "spell"))
            player.t_action = C_CAST;
        return;
    }

    /* We've waited our required casting time. */
    which_spell = player.t_selection;
    player.t_selection = 0;
    player.t_action = A_NIL;

    if ((spell_power + magic_spells[which_spell].s_cost) > spell_ability) {
        msg("Your attempt fails.");
        return;
    }

    msg("Your spell is successful. ");

    if (magic_spells[which_spell].s_type == TYP_POTION)
        quaff(  magic_spells[which_spell].s_which,
                NULL,
                magic_spells[which_spell].s_flag,
                FALSE);
    else if (magic_spells[which_spell].s_type == TYP_SCROLL)
        read_scroll(    magic_spells[which_spell].s_which,
                        magic_spells[which_spell].s_flag,
                        FALSE);
    else if (magic_spells[which_spell].s_type == TYP_STICK) {
         if (!player_zap(magic_spells[which_spell].s_which,
                         magic_spells[which_spell].s_flag)) {
             after = FALSE;
             return;
         }
    }
    spell_power += magic_spells[which_spell].s_cost;
}

/* 
 * the druid asks his deity for a spell
 */

chant()
{
    register int num_chants, chant_ability, which_chant;

    which_chant = num_chants = chant_ability = 0;

    if (player.t_ctype != C_DRUID && player.t_ctype != C_MONK) {
        msg("You are not permitted to chant.");
        return;
    }
    if (cur_misc[WEAR_CLOAK] != NULL &&
        cur_misc[WEAR_CLOAK]->o_which == MM_R_POWERLESS) {
        msg("You can't seem to chant!");
        return;
    }
    chant_ability = pstats.s_lvl * pstats.s_wisdom - 5;
    if (player.t_ctype != C_DRUID)
        chant_ability /= 2;

    if (player.t_action != C_CHANT) {
        num_chants = 0;

        /* Get the number of avilable chants */
        if (pstats.s_wisdom > 16) 
            num_chants += pstats.s_wisdom - 16;

        num_chants += pstats.s_lvl;

        if (player.t_ctype != C_DRUID) 
            num_chants /= 2;

        if (num_chants > MAXCHANTS) 
            num_chants = MAXCHANTS;

        if (num_chants < 1) {
            msg("You are not permitted to chant yet.");
            return;
        }

        /* Prompt for chant */
        if (pick_spell( druid_spells, 
                        chant_ability, 
                        num_chants, 
                        chant_time,
                        "sing",
                        "chant"))
            player.t_action = C_CHANT;

        return;
    }

    /* We've waited our required chanting time. */
    which_chant = player.t_selection;
    player.t_selection = 0;
    player.t_action = A_NIL;

    if (druid_spells[which_chant].s_cost + chant_time > chant_ability) {
        msg("Your chant fails.");
        return;
    }

    msg("Your chant has been granted. ");

    if (druid_spells[which_chant].s_type == TYP_POTION)
        quaff(          druid_spells[which_chant].s_which,
                        NULL,
                        druid_spells[which_chant].s_flag,
                        FALSE);
    else if (druid_spells[which_chant].s_type == TYP_SCROLL)
        read_scroll(    druid_spells[which_chant].s_which,
                        druid_spells[which_chant].s_flag,
                        FALSE);
    else if (druid_spells[which_chant].s_type == TYP_STICK) {
         if (!player_zap(druid_spells[which_chant].s_which,
                         druid_spells[which_chant].s_flag)) {
             after = FALSE;
             return;
         }
    }
    chant_time += druid_spells[which_chant].s_cost;
}

/* Constitution bonus */

const_bonus()   /* Hit point adjustment for changing levels */
{
    register int bonus;
    if (pstats.s_const > 9 && pstats.s_const < 18) 
        bonus = 0;
    else if (pstats.s_const >= 18 && pstats.s_const < 20) 
        bonus = 1;
    else if (pstats.s_const >= 20 && pstats.s_const < 26) 
        bonus = 2;
    else if (pstats.s_const >= 26 && pstats.s_const < 36) 
        bonus = 3;
    else if (pstats.s_const >= 36) 
        bonus = 4;
    else if (pstats.s_const > 7) 
        bonus = -1;
    else
        bonus = -2;
    switch(player.t_ctype) {
        case C_FIGHTER:         bonus = min(bonus, 11);
        when C_RANGER:          bonus = min(bonus,  9);
        when C_PALADIN:         bonus = min(bonus,  9);
        when C_MAGICIAN:        bonus = min(bonus,  8);
        when C_CLERIC:          bonus = min(bonus,  8);
        when C_THIEF:           bonus = min(bonus, 10);
        when C_ASSASSIN:        bonus = min(bonus, 10);
        when C_DRUID:           bonus = min(bonus,  8);
        when C_MONK:            bonus = min(bonus,  9);
        otherwise:              bonus = min(bonus,  7);
    }
    return(bonus);
}

/*
 * Give away slime-molds to monsters.  If monster is friendly, 
 * it will give you a "regular" food ration in return.  You have
 * to give a slime-mold to Alteran (a unique monster) in order to 
 * get the special "Card of Alteran" quest item.  There's no other
 * way to get this artifact and remain alive.
 */

give(th)
register struct thing *th;
{
    /*
     * Find any monster within one space of you
     */
    struct linked_list *ll;
    struct object *lb;
    register int x,y;
    register struct linked_list *mon = NULL;
    bool gotone = FALSE;

    if (levtype != POSTLEV) {  /* no monsters at trading post  */
        for (x = hero.x-1; x <= hero.x+1; x++) {
            for (y = hero.y-1; y <= hero.y+1; y++) {
                if (y < 1 || x < 0 || y > lines - 3 || x > cols - 1)
                    continue;
                if (isalpha(mvwinch(mw, y, x))) {
                    if ((mon = find_mons(y, x)) != NULL) {
                        gotone = TRUE;  /* found a monster to give away to */
                        th = THINGPTR(mon);
                    }
                }
            }
        }
    }
    if (gotone) {
        if ((ll=get_item(pack, "give away", ALL, FALSE, FALSE)) != NULL) {
            lb = OBJPTR(ll);
            mpos = 0;
            switch(lb->o_type) {
                case FOOD:
                    switch (lb->o_which) {
                        case E_SLIMEMOLD:   /* only slime-molds for now */
                if (on(*th, CANSELL)) {  /* quartermaster */
                msg("%s laughs at you. ");
                return;
                }
                if ((on(*th, ISFRIENDLY) || off(*th, ISMEAN)) &&
                off(*th, ISUNIQUE) && off(*th, CANSELL)) {
                turn_on(*th, ISRUN); /* we want him awake */
                                msg("%s accepts and promptly eats your gift of food.  --More--", prname(monster_name(th), TRUE));
                wait_for(' ');
                    del_pack(ll); /* delete slime-mold */
                          /* and add a food ration */
                create_obj(FALSE, FOOD, E_RATION);
                msg("%s gives you food in return and nods off to sleep. ", prname(monster_name(th), TRUE));
                turn_off(*th, ISRUN); /* put him to sleep */
                return;
                }
                else if (on(*th, CARRYCARD) && on(*th, ISUNIQUE)) {
                /* Now you get the Card of Alteran */
                msg("%s gives you a strange rectangular card. --More--", prname(monster_name(th), TRUE));
                wait_for(' ');
                    del_pack(ll); /* get rid of slime-mold */
                create_obj(FALSE, RELIC, ALTERAN_CARD);
                msg("%s bids you farewell. ", prname(monster_name(th), TRUE));
                killed(mon, FALSE, FALSE, FALSE);
                return;
                }
                else if (on(*th, ISUNIQUE) && off(*th, ISMEAN)) {
                /* Dragons */
                msg("%s is set free by your generosity. ", prname(monster_name(th), TRUE));
                    del_pack(ll);  /* get rid of it */
                /* just let him roam around */
                turn_on(*th, ISRUN);
                if (on(*th, ISFLEE)) turn_off(*th, ISFLEE);
                runto(th, &player);
                th->t_action = A_NIL;
                return;
                }
                else if (on(*th, ISRUN) && off(*th, ISUNIQUE)) {
                /* if NOT sleeping and not a unique */
                switch (rnd(2)) {
                    case 0: msg("%s ignores you. ", prname(monster_name(th), TRUE));
                    when 1: {
                    msg("%s nips at your hand. ", prname(monster_name(th), TRUE));
                    if (rnd(100) < 10) { 
                                del_pack(ll); /* delete it */
                        if (off(*th, ISMEAN)) {
                        msg("The slime-mold makes %s sleepy. ", prname(monster_name(th), TRUE));
                        /* put him to sleep */
                        turn_off(*th, ISRUN);
                        return;
                        }
                        else {  
                        switch (rnd(2)) {
                            case 0: msg("%s's eyes roll back. ", prname(monster_name(th), TRUE));
                                when 1: msg("%s becomes wanderlust. ", prname(monster_name(th), TRUE));
                        }
                        /* just let him roam around */
                            turn_on(*th, ISRUN);
                        if (on(*th, ISFLEE))
                                        turn_off(*th, ISFLEE);
                        runto(th, &player);
                        th->t_action = A_NIL;
                        return;
                        }
                    }
                    }
                }
                }
                else {
                msg("%s's mouth waters. ", prname(monster_name(th), TRUE));
                /* this wakes him up */
                if (off(*th, ISUNIQUE)) turn_on(*th, ISRUN);
                return;
                }
                otherwise:
                switch (rnd(3)) {  /* mention food (hint hint) */
                    case 0: msg("You cannot give away the %s! ", foods[lb->o_which].mi_name);
                    when 1: msg("The %s looks rancid! ", foods[lb->o_which].mi_name);
                    when 2: msg("You change your mind. ");
                }
                return;
                    }
            otherwise:
            switch (rnd(3)) {  /* do not mention other items */
                    case 0: msg("You feel foolish. ");
                    when 1: msg("You change your mind. ");
                    when 2: msg("%s ignores you. ", prname(monster_name(th), TRUE));
            }

        return;
            }
        }
    }
    else msg("Your efforts are futile. ");
    return;
}

/*
 * Frighten a monster.  Useful for the 'good' characters.
 */

fright(th)
register struct thing *th;
{
    /*
     * Find any monster within one space of you
     */
    register int x,y;
    register struct linked_list *mon;
    bool gotone = FALSE;

    if (levtype != POSTLEV) {  /* no monsters at trading post  */
        for (x = hero.x-1; x <= hero.x+1; x++) {
            for (y = hero.y-1; y <= hero.y+1; y++) {
                if (y < 1 || x < 0 || y > lines - 3 || x > cols - 1)
                    continue;
                if (isalpha(mvwinch(mw, y, x))) {
                    if ((mon = find_mons(y, x)) != NULL) {
                        gotone = TRUE;  /* found a monster to give away to */
                        th = THINGPTR(mon);
                    }
                }
            }
        }
    }
    if (gotone) {  /* If 'good' character or is wearing a ring of fear */
        if (player.t_ctype == C_RANGER || player.t_ctype == C_PALADIN ||
            player.t_ctype == C_MONK   || ISWEARING(R_FEAR) != 0) { 

            player.t_action = A_NIL;
            player.t_no_move = movement(&player);
            switch (player.t_ctype) {
                case C_FIGHTER:   /* loss of strength */
                    pstats.s_str--;
                    if (pstats.s_str < 3) pstats.s_str = 3;
                when C_RANGER:    /* loss of charisma */
        case C_PALADIN:
                    pstats.s_charisma--;
                    if (pstats.s_charisma < 3) pstats.s_charisma = 3;
                when C_CLERIC:    /* loss of wisdom */
        case C_DRUID:
                    pstats.s_wisdom--;
                    if (pstats.s_wisdom < 3) pstats.s_wisdom = 3;
                when C_MAGICIAN:  /* loss of wisdom intelligence */
            pstats.s_intel--;
                    if (pstats.s_intel < 3) pstats.s_intel = 3;
                when C_THIEF:     /* loss of dexterity */
                case C_ASSASSIN:
                    pstats.s_dext--;
                    if (pstats.s_dext < 3) pstats.s_dext = 3;
                when C_MONK:      /* loss of constitution */
                    pstats.s_const--;
                    if (pstats.s_const < 3) pstats.s_const = 3;
                otherwise:        /* this msg can induce great fear */
                    msg("You miss. ");
        }

        /* Cause a panic.  Good thru level 16. */
            if (level < 17) { 
        msg("You wave your arms and yell! ");
                do_panic(th->t_index);
                pstats.s_hpt -= (pstats.s_hpt/2)+1;
        if (pstats.s_hpt < 25) msg("You heart quivers... ");
                if (pstats.s_hpt < 1) {
            msg("Your heart stops!!  --More--");
            wait_for(' ');
            pstats.s_hpt = -1;
            death(D_FRIGHT);
        }
            return;
        }
        else {
        /* He can't do it after level 16 */
        switch (rnd(20)) { 
            case 0: case 2:
            msg("You stamp your foot!! ");
            when 4: case 8:
            msg("%s laughs at you! ",prname(monster_name(th),TRUE));
            when 10: case 12:
            msg("You forget what you are doing? ");
                otherwise:
                msg(nothing);
        }
            return;
        }
    }
        else {
        switch (rnd(25)) {
        case 0: case 2: case 4:
        msg("You motion angrily! ");
        when 6: case 8: case 10:
        msg("You can't frighten anything. ");
        when 12: case 14: case 16:
        msg("Your puff up your face. ");
        otherwise:
        msg(nothing);
        }
    return;
    }
    }
    else {
    msg("There is nothing to fear but fear itself. ");
    return;
    }
}

/* Routines for thieves */

/*
 * gsense: Sense gold
 */

gsense()
{
    /* Thief & assassin can do this, but fighter & ranger can later */
    if (player.t_ctype == C_THIEF     || player.t_ctype == C_ASSASSIN ||
        ((player.t_ctype == C_FIGHTER || player.t_ctype == C_RANGER)  &&
    pstats.s_lvl >= 12)) {
          read_scroll(S_GFIND, NULL, FALSE);
    }
    else msg("You seem to have no gold sense.");
    return;
}

/*
 * xsense: Sense traps
 */

xsense()
{
    /* Only thief can do this, but assassin, fighter, & monk can later */
    if (player.t_ctype == C_THIEF   || ((player.t_ctype == C_ASSASSIN ||
    player.t_ctype == C_FIGHTER || player.t_ctype == C_MONK)      &&
    pstats.s_lvl >= 14)) {
        read_scroll(S_FINDTRAPS, NULL, FALSE);
    }
    else msg("You seem not to be able to sense traps.");
    return;
}

/*
 * steal:
 *      Steal in direction given in delta
 */

steal()
{
    register struct linked_list *item;
    register struct thing *tp;
    register char *mname;
    coord new_pos;
    int thief_bonus = -50;
    bool isinvisible = FALSE;


    /* let the fighter steal after level 15 */
    if (player.t_ctype == C_FIGHTER && pstats.s_lvl < 15) {
        msg(nothing);
        return;
    }
    else if (player.t_ctype != C_THIEF    &&
        player.t_ctype  != C_ASSASSIN &&
            player.t_ctype  != C_FIGHTER) {
            msg("Only thieves and assassins can steal.");
            return;
    }
    if (on(player, ISBLIND)) {
        msg("You can't see anything.");
        return;
    }

    new_pos.y = hero.y + player.t_newpos.y;
    new_pos.x = hero.x + player.t_newpos.x;

    /* Anything there? */
    if (new_pos.y < 0 || new_pos.y > lines-3 ||
        new_pos.x < 0 || new_pos.x > cols-1 ||
        mvwinch(mw, new_pos.y, new_pos.x) == ' ') {
        msg("Nothing to steal from.");
        return;
    }

    if ((item = find_mons(new_pos.y, new_pos.x)) == NULL)
        debug("Steal from what @ %d,%d?", new_pos.y, new_pos.x);
    tp = THINGPTR(item);
    if (on(*tp, ISSTONE)) {
        msg ("You can't steal from stone!");
        return;
    }

    if (on(*tp, ISFLEE)) {
        msg("You can't get your hand in anywhere! ");
        return;
    }

    isinvisible = invisible(tp);
    if (isinvisible) mname = "creature";
    else mname = monster_name(tp);

    /* Can player steal something unnoticed? */
    if (player.t_ctype == C_THIEF)    thief_bonus = 9;
    if (player.t_ctype == C_ASSASSIN) thief_bonus = 6;
    if (player.t_ctype == C_FIGHTER)  thief_bonus = 3;
    if (on(*tp, ISUNIQUE)) thief_bonus -= 15;
    if (isinvisible) thief_bonus -= 20;
    if (on(*tp, ISINWALL) && off(player, CANINWALL)) thief_bonus -= 50;

    if (on(*tp, ISHELD) || tp->t_action == A_FREEZE ||
        rnd(100) <
        (thief_bonus + 2*dex_compute() + 5*pstats.s_lvl -
         5*(tp->t_stats.s_lvl - 3))) {
        register struct linked_list *s_item, *pack_ptr;
        int count = 0;
        unsigned long test;      /* Overflow check */

        s_item = NULL;  /* Start stolen goods out as nothing */

        /* Find a good item to take */
        for (pack_ptr=tp->t_pack; pack_ptr != NULL; pack_ptr=next(pack_ptr))
            if ((OBJPTR(pack_ptr))->o_type != RELIC &&
                pack_ptr != tp->t_using &&  /* Monster can't be using it */
                rnd(++count) == 0)
                s_item = pack_ptr;

        /* 
         * Find anything?
         */
        if (s_item == NULL) {
            msg("%s apparently has nothing to steal.", prname(mname, TRUE));
            return;
        }

        /* Take it from monster */
        if (tp->t_pack) detach(tp->t_pack, s_item);

        /* Recalculate the monster's encumberance */
        updpack(TRUE, tp);

        /* Give it to player */
        if (add_pack(s_item, FALSE) == FALSE) {
           (OBJPTR(s_item))->o_pos = hero;
           fall(s_item, TRUE);
        }

        /* Get points for stealing -- but first check for overflow */
        test = pstats.s_exp + tp->t_stats.s_exp/2;
        if (test > pstats.s_exp) pstats.s_exp = test;

        /*
         * Do adjustments if player went up a level
         */
        check_level();
    }

    else {
        msg("Your attempt fails.");

        /* Annoy monster (maybe) */
        if (rnd(40) >= dex_compute() + thief_bonus) {
            /*
             * If this is a charmed creature, there is a chance it
             * will become uncharmed.
             */
            if (on(*tp, ISCHARMED) && save(VS_MAGIC, tp, 0)) {
                msg("The eyes of %s turn clear.", prname(mname, FALSE));
                turn_off(*tp, ISCHARMED);
            }
            if (on(*tp, CANSELL)) {
                turn_off(*tp, CANSELL);
                tp->t_action = A_NIL;
                tp->t_movement = 0;
                if (rnd(100) < 50) /* make him steal something */
                    turn_on(*tp, STEALMAGIC);
                else
                    turn_on(*tp, STEALGOLD);
                if (!isinvisible)
                    msg("%s looks insulted.", prname(mname, TRUE));
            }
            runto(tp, &hero);
        }
    }
}

/*
 * Take charmed monsters with you via up or down commands.
 */

take_with()
{
    register struct thing *tp;
    register struct linked_list *item;
    struct linked_list *nitem;
    register int t;

    t = 0;
    for (item = mlist; item != NULL; item = nitem) {
    nitem = next(item);
    t++;
    if (t > 5) break;
    tp = THINGPTR(item);
        if (on(*tp, ISCHARMED)) {
            monsters[tp->t_index].m_normal = TRUE;
            turn_on(*tp, ISELSEWHERE);
            detach(mlist, item);
            attach(tlist, item);         /* remember him next level */
            check_residue(tp);
            continue;
        }
    }
}

/*
 * this routine lets the player pick the spell that they
 * want to cast regardless of character class
 */

pick_spell(spells, ability, num_spells, power, prompt, type)
struct spells   spells[];       /* spell list                            */
int             ability;        /* spell ability                         */
int             num_spells;     /* number of spells that can be cast     */
int             power;          /* spell power                           */
const char      *prompt;        /* prompt for spell list                 */
const char      *type;          /* type of thing--> spell, prayer, chant */
{
    bool                nohw = FALSE;
    register int        i;
    int                 curlen,
                        maxlen = 0,
                        dummy = 0,
                        which_spell,
                        spell_left;
    if (cur_misc[WEAR_CLOAK] != NULL &&
        cur_misc[WEAR_CLOAK]->o_which == MM_R_POWERLESS) {
        msg("You can't seem to start a %s!", type);
        return(FALSE);
    }

    /* Prompt for spells */
    msg("Which %s are you %sing? (* for list): ", type, prompt);

    which_spell = (int) (wgetch(cw) - 'a');
    msg("");    /* Get rid of the prompt */
    if (which_spell == (int) ESC - (int) 'a') {
        after = FALSE;
        return(FALSE);
    }
    if (which_spell >= 0 && which_spell < num_spells) nohw = TRUE;

    else if (slow_invent) {
        register char c;

        nohw = TRUE;
        do {
            for (i=0; i<num_spells; i++) {
                msg("");
                mvwaddch(msgw, 0, 0, '[');
                waddch(msgw, (char) ((int) 'a' + i));
                wprintw(msgw, "] A %s of ", type);
                if (spells[i].s_type == TYP_POTION)
                    waddstr(msgw, p_magic[spells[i].s_which].mi_name);
                else if (spells[i].s_type == TYP_SCROLL)
                    waddstr(msgw, s_magic[spells[i].s_which].mi_name);
                else if (spells[i].s_type == TYP_STICK)
                    waddstr(msgw, ws_magic[spells[i].s_which].mi_name);
                waddstr(msgw, morestr);
                wclrtobot(msgw);
                clearok(msgw, FALSE);
                draw(msgw);
                do {
                    c = wgetch(cw);
                } while (c != ' ' && c != ESC);
                if (c == ESC)
                    break;
            }
            msg("");
            wmove(msgw, 0, 0);
            wprintw(msgw, "Which %s are you %sing? ", type, prompt);
            clearok(msgw, FALSE);
            draw(msgw);

            which_spell = (int) (wgetch(cw) - 'a');
        } while (which_spell != (int) (ESC - 'a') &&
                 (which_spell < 0 || which_spell >= num_spells));

        if (which_spell == (int) (ESC - 'a')) {
            mpos = 0;
            msg("");
            after = FALSE;
            return(FALSE);
        }
    }
    else {
        /* Now display the possible spells */
        wclear(hw);
        touchwin(hw);
        wmove(hw, 2, 0);
        wprintw(hw, "   Cost    %c%s", toupper(*type),type+1);
        mvwaddstr(hw, 3, 0,
                "-----------------------------------------------");
        maxlen = 47;    /* Maximum width of header */

        for (i=0; i<num_spells; i++) {
            sprintf(prbuf, "[%c]        %3d     A %s of ",
                        (char) ((int) 'a' + i), spells[i].s_cost, type);
            if (spells[i].s_type == TYP_POTION)
                strcat(prbuf, p_magic[spells[i].s_which].mi_name);
            else if (spells[i].s_type == TYP_SCROLL)
                strcat(prbuf, s_magic[spells[i].s_which].mi_name);
            else if (spells[i].s_type == TYP_STICK)
                strcat(prbuf, ws_magic[spells[i].s_which].mi_name);
            mvwaddstr(hw, i+4, 0, prbuf);

            /* Get the length of the line */
            getyx(hw, dummy, curlen);
            if (maxlen < curlen) maxlen = curlen;
        }

        spell_left = ability - power;
        if (spell_left < 0) {
            spell_left = 0;
            if (spell_left < -20) power = ability + 20;
        }
        sprintf(prbuf, "[Current %s power = %d]", type, spell_left);

        mvwaddstr(hw, 0, 0, prbuf);
        wprintw(hw, " Which %s are you %sing? ", type, prompt);
        getyx(hw, dummy, curlen);
        if (maxlen < curlen) maxlen = curlen;

        /* Should we overlay? */
        if (menu_overlay && num_spells + 3 < lines - 3) {
            over_win(cw, hw, num_spells + 5, maxlen + 3, 0, curlen, NULL);
        }
        else draw(hw);
    }

    if (!nohw) {
        which_spell = (int) (wgetch(cw) - 'a');
        while (which_spell < 0 || which_spell >= num_spells) {
            if (which_spell == (int) ESC - (int) 'a') {
                after = FALSE;

                /* Restore the screen */
                if (num_spells + 3 < lines / 2) {
                    clearok(cw, FALSE);
                    touchwin(cw);
                }
                else restscr(cw);
                return(FALSE);
            }
            wmove(hw, 0, 0);
            wclrtoeol(hw);
            wprintw(hw, "Please enter one of the listed %ss. ", type);
            getyx(hw, dummy, curlen);
            if (maxlen < curlen) maxlen = curlen;

            /* Should we overlay? */
            if (menu_overlay && num_spells + 3 < lines - 3) {
                over_win(cw, hw, num_spells + 5, maxlen + 3,
                            0, curlen, NULL);
            }
            else draw(hw);

            which_spell = (int) (wgetch(cw) - 'a');
        }
    }

    /* Now restore the screen if we have to */
    if (!nohw) {
        if (num_spells + 3 < lines / 2) {
            touchwin(cw);
            clearok(cw, FALSE);
        }
        else {
            restscr(cw);
        }
    }

    if (spells[which_spell].s_type == TYP_STICK && 
        need_dir(STICK, spells[which_spell].s_which)) {
            if (!get_dir(&player.t_newpos)) {
                after = FALSE;
                return(FALSE);
            }
    }
    player.t_selection = which_spell;
    player.t_no_move = (which_spell/3 + 1) * movement(&player);

    spell_left = dummy; /* hack to stop IRIX complaint about dummy */
                        /* not being used.                         */
    return(TRUE);
}

/*
 * opt_player:
 * Let the player know what's happening with himself
 */

opt_player()
{
    int i = 1;  /* initialize counters */
    int j = 2;

    wclear(hw);
    wmove(hw, 0, 0);
    wprintw(hw, "Current player effects:");
    wmove(hw, 2, 0);

        /*   Print a list of what is happening.
     *   If longer than 16 lines, make it two columns.
     *   Currently, a maximum of 32 (out of 39) "effects"
     *   can be happening all at once to a player.
     */

    /* 1 - Sense gold */
    if (player.t_ctype == C_THIEF    || player.t_ctype == C_ASSASSIN ||
        ((player.t_ctype == C_FIGHTER || player.t_ctype == C_RANGER) &&
    pstats.s_lvl >= 12)) { 
    wprintw(hw, "You can sense gold\n");
    i++;
    }
    /* 2 - Sense traps */
    if (player.t_ctype == C_THIEF   || ((player.t_ctype == C_ASSASSIN ||
        player.t_ctype == C_FIGHTER || player.t_ctype == C_MONK)      &&
    pstats.s_lvl >= 14)) {
    wprintw(hw, "You can sense traps\n");
    i++;
    }
    /* 3 - Steal */
    if (player.t_ctype == C_THIEF    || player.t_ctype == C_ASSASSIN || 
        (player.t_ctype == C_FIGHTER && pstats.s_lvl >= 15)) { 
    wprintw(hw, "You can steal\n");
    i++;
    }
    /* 4 - Cast spells */
    if (player.t_ctype == C_MAGICIAN ||
        (player.t_ctype == C_RANGER  && pstats.s_lvl > 1)) { 
    wprintw(hw, "You can cast spells\n");
    i++;
    }
    /* 5 - Make chants */
    if (player.t_ctype == C_DRUID ||
        (player.t_ctype == C_MONK && pstats.s_lvl > 1)) {
    wprintw(hw, "You can chant\n");
    i++;
    }
    /* 6 - Give prayers */
    if (cur_relic[HEIL_ANKH] != 0 || player.t_ctype == C_CLERIC ||
        (player.t_ctype == C_PALADIN && pstats.s_lvl > 1)) {
    wprintw(hw, "You can pray\n");
    i++;
    }
    /* 7 - Affect the undead */
    if (cur_relic[HEIL_ANKH] != 0 || player.t_ctype == C_CLERIC ||
        (player.t_ctype == C_PALADIN && pstats.s_lvl > 4)) { 
    wprintw(hw, "You can affect the undead\n");
    i++;
    }
    /* 8 - Cause fear */
    if (ISWEARING(R_FEAR) != 0   || ((player.t_ctype == C_RANGER ||
        player.t_ctype == C_PALADIN || player.t_ctype == C_MONK)    &&
        pstats.s_lvl > 1)) {
    wprintw(hw, "You are fearful\n");
    i++;
    }
    /* 9 - Confuse monster */
    if (on(player, CANHUH) != 0) {
    wprintw(hw, "You have multi-colored hands\n");
    i++;
    }
    /* 10 - Confused yourself */
    if (on(player, ISHUH) != 0) {
    wprintw(hw, "You are confused\n");
    i++;
    }                  /* really ISHUH or ISCLEAR */
    /* 11 - Clear thought */
    if (on(player, ISCLEAR) != 0) {
    wprintw(hw, "You are clear headed\n");
    i++;
    }
    /* 12 - Slow */
    if (on(player, ISSLOW) != 0) {
    wprintw(hw, "You are moving slow\n");
    i++;
    }                 /* really ISSLOW or ISHASTE */
    /* 13 - Haste */
    if (on(player, ISHASTE) != 0) {
    wprintw(hw, "You are moving fast\n");
    i++;
    }
    /* 14 - Flying */
    if (on(player, ISFLY) != 0) {
    wprintw(hw, "You are flying\n");
    i++;
    }
    /* 15 - Blind */
    if (on(player, ISBLIND) != 0) {
    wprintw(hw, "You are blind\n");
    i++;
    }                 /* really ISBLIND or CANSEE */
    /* 16 - Extra sight */
    if (on(player, CANSEE) != 0) {
    wprintw(hw, "You have extra sight\n");
    i++;
    }
    /* 17 - Invisibility */
    if (on(player, ISINVIS) != 0) {
    /* Okay, start a second column of effects to the screen. */
        if (i > 16) {
            mvwaddstr(hw, j, 37, "You are invisible");
        j++;
        }
    else {
        wprintw(hw, "You are invisible\n");
        i++;
    }
    }
    /* 18 - Regeneration and vampiric regen */
    if (ISWEARING(R_VAMPREGEN) != 0 || ISWEARING(R_REGEN) != 0) {
        if (i > 16) {
            mvwaddstr(hw, j, 37, "You have regenerative powers");
        j++;
        }
    else {
        wprintw(hw, "You have regenerative powers\n");
        i++;
    }
    }
    /* 19 - Phasing */
    if (on(player, CANINWALL) != 0) {
        if (i > 16) {
            mvwaddstr(hw, j, 37, "You can walk through walls");
        j++;
        }
    else {
        wprintw(hw, "You can walk through walls\n");
        i++;
    }
    }
    /* 20 - Skill (good or bad, it won't last) */
    if (find_slot(unskill) != 0) {
        if (i > 16) {
            mvwaddstr(hw, j, 37, "You feel skillful");
        j++;
        }
    else {
        wprintw(hw, "You feel skillful\n");
        i++;
    }
    }
    /* 21 - Stealthy */
    if (ISWEARING(R_STEALTH) != 0) {
        if (i > 16) {
            mvwaddstr(hw, j, 37, "You have stealth");
        j++;
        }
    else {
        wprintw(hw, "You have stealth\n");
        i++;
    }
    }
    /* 22 - Alertness */
    if (ISWEARING(R_ALERT) != 0) {  
        if (i > 16) {
            mvwaddstr(hw, j, 37, "You are awake and alert");
        j++;
        }
    else {
        wprintw(hw, "You are awake and alert\n");
        i++;
    }
    }
    /* 23 - Free action */
    if (ISWEARING(R_FREEDOM) != 0) {
        if (i > 16) {
            mvwaddstr(hw, j, 37, "You feel free");
        j++;
        }
    else {
        wprintw(hw, "You feel free\n");
        i++;
    }
    }
    /* 24 - Heroism */
    if (ISWEARING(R_HEROISM) != 0) {
        if (i > 16) {
            mvwaddstr(hw, j, 37, "You are brave");
        j++;
        }
    else {
        wprintw(hw, "You are brave\n");
        i++;
    }
    }
    /* 25 - Ice protection */
    if (on(player, NOCOLD) != 0) {
        if (i > 16) {
            mvwaddstr(hw, j, 37, "You are protected from ice");
        j++;
        }
    else {
        wprintw(hw, "You are protected from ice\n");
        i++;
    }
    }
    /* 26 - Fire protection */
    if (on(player, NOFIRE) != 0) {
        if (i > 16) {
            mvwaddstr(hw, j, 37, "You are protected from fire");
        j++;
        }
    else {
        wprintw(hw, "You are protected from fire\n");
        i++;
    }
    }
    /* 27 - Lightning protection */
    if (on(player, NOBOLT) != 0) {
        if (i > 16) {
            mvwaddstr(hw, j, 37, "You are protected from lightning");
        j++;
        }
    else {
        wprintw(hw, "You are protected from lightning\n");
        i++;
    }
    }
    /* 28 - Gas protection */
    if (on(player, NOGAS) != 0) {
        if (i > 16) {
            mvwaddstr(hw, j, 37, "You are protected from gas");
        j++;
        }
    else {
        wprintw(hw, "You are protected from gas\n");
        i++;
    }
    }
    /* 29 - Acid protection */
    if (on(player, NOACID) != 0) { 
        if (i > 16) {
            mvwaddstr(hw, j, 37, "You are protected from acid");
        j++;
        }
    else {
        wprintw(hw, "You are protected from acid\n");
        i++;
    }
    }
    /* 30 - Breath protection */
    if (cur_relic[YENDOR_AMULET] != 0) {
        if (i > 16) {
            mvwaddstr(hw, j, 37, "You are protected from monster breath");
        j++;
        }
    else {
        wprintw(hw, "You are protected from monster breath\n");
        i++;
    }             /* really only YENDOR or STONEBONES */
    }           
    /* 31 - Magic missile protection */
    if (cur_relic[STONEBONES_AMULET] != 0) { 
        if (i > 16) {
            mvwaddstr(hw, j, 37, "You are protected from magic missiles");
        j++;
        }
    else {
        wprintw(hw, "You are protected from magic missiles\n");
        i++;
    }
    }
    /* 32 - Sustain health */
    if (ISWEARING(R_HEALTH) != 0 && (off(player, HASDISEASE) &&
    off(player, HASINFEST) && off(player, DOROT))) {  
        if (i > 16) {     /* he's really healthy */
            mvwaddstr(hw, j, 37, "You are in good health");
        j++;
        }
    else {
        wprintw(hw, "You are in good health\n"); 
        i++;
    }
    }
    /* 33 - Being held */
    if (on(player, ISHELD) != 0) {
        if (i > 16) {
            mvwaddstr(hw, j, 37, "You are being held");
        j++;
        }
    else {
        wprintw(hw, "You are being held\n");
        i++;
    }
    }
    /* 34 - Stinks */
    if (on(player, HASSTINK) != 0) {
        if (i > 16) {
            mvwaddstr(hw, j, 37, "You are affronted by a bad smell");
        j++;
        }
    else {
        wprintw(hw, "You are affronted by a bad smell\n");
        i++;
    }
    }
    /* 35 - Any attribute that is down */
    if (pstats.s_intel    < max_stats.s_intel  ||
        pstats.s_str      < max_stats.s_str    ||
        pstats.s_wisdom   < max_stats.s_wisdom ||
        pstats.s_dext     < max_stats.s_dext   ||
        pstats.s_const    < max_stats.s_const  ||
        pstats.s_charisma < max_stats.s_charisma) {
        if (i > 16) {
            mvwaddstr(hw, j, 37, "You are afflicted");
        j++;
        }
    else {
        wprintw(hw, "You are afflicted\n");
        i++;
    }
    }
    /* 36 - Diseased */
    if (on(player, HASDISEASE) != 0) {  
        if (i > 16) {
            mvwaddstr(hw, j, 37, "You have a disease");
        j++;
        }
    else {
        wprintw(hw, "You have a disease\n");
        i++;
    }
    }
    /* 37 - Infested */
    if (on(player, HASINFEST) != 0) {  
        if (i > 16) {
            mvwaddstr(hw, j, 37, "You have an infestation");
        j++;
        }
    else {
        wprintw(hw, "You have an infestation\n");
        i++;
    }
    }
    /* 38 - Body rot */
    if (on(player, DOROT) != 0) {  
        if (i > 16) {
            mvwaddstr(hw, j, 37, "You have body rot");
        j++;
        }
    else {
        wprintw(hw, "You have body rot\n");
        i++;
    }
    }
    /* 39 - Dancing */
    if (on(player, ISDANCE) != 0) {  
        if (i > 16) {
            mvwaddstr(hw, j, 37, "You are a dancing fool");
        j++;
        }
    else {
        wprintw(hw, "You are a dancing fool\n");
        i++;
    }
    }
    if (i == 1) {
    wclear(hw);
        msg("No player effects. ");
    return;
    }
    else {
    if (i > 1 && i < 17) {
        j = 39;
            if (menu_overlay) {     /* Print the list. */
                wmove(hw, i+2, 0);
                wprintw(hw, spacemsg);
                over_win(cw, hw, i+3, j, i+2, 27, NULL);
        }
            else {
                wmove(hw, i+2, 0);
                wprintw(hw, spacemsg);
            draw(hw);
        }
    }
    else {
        i = 17;
            if (menu_overlay) {     /* Print the list. */
                wmove(hw, i+2, 0);
                wprintw(hw, spacemsg);
        if (j > 2) j = 78;
        else j = 39;
                over_win(cw, hw, i+3, j, i+2, 27, NULL);
        }
            else {
                wmove(hw, i+2, 0);
                wprintw(hw, spacemsg);
            draw(hw);
        }
    }
        wait_for(' ');
    wclear(hw);
    status(FALSE);
    touchwin(cw);
        return;
    }
}

