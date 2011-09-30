/*
    effects.c  -  functions for dealing with appllying effects to monsters

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
 * effect:
 *      Check for effects of one thing hitting another thing.  Return
 *      the reason code if the defender is killed.  Otherwise return 0.
 */

effect(att, def, weap, thrown, see_att, see_def)
register struct thing *att, *def;
struct object *weap;
bool thrown;
register bool see_att, see_def;
{
    register bool att_player, def_player;
    char attname[LINELEN+1], defname[LINELEN+1];

    /* See if the attacker or defender is the player */
    att_player = (att == &player);
    def_player = (def == &player);

    /*
     * If the player could see the attacker or defender, they can't
     * surprise anymore (don't bother checking if they could).
     */
    if (see_att) turn_off(*att, CANSURPRISE);
    if (see_def) turn_off(*def, CANSURPRISE);

    /* What are the attacker and defender names? */
    if (att_player) strcpy(attname, "you");
    else {
        if (see_att) strcpy(attname, monster_name(att));
        else strcpy(attname, "something");
    }

    if (def_player) strcpy(defname, "you");
    else {
        if (see_def) strcpy(defname, monster_name(def));
        else strcpy(defname, "something");
    }

    /*
     * See what happens to the attacker first.  We can skip this
     * whole section, however, if the defender is the player.
     * Nothing happens (yet) to anyone just for hitting the player.
     */
    if (!def_player) {
        if (!thrown) {  /* Some things require a direct hit. */
            /*
             * If the attacker hits a rusting monster, The weapon
             * may be damaged
             */
            if (on(*def, CANRUST)       && weap                         &&
                weap->o_type != RELIC   && (weap->o_flags & ISMETAL)    &&
                !(weap->o_flags & ISPROT)) {
                    if ((weap->o_hplus < 1 && weap->o_dplus < 1) ||
                        roll(1,20) < weap->o_hplus+weap->o_dplus+10) {
                            if (rnd(100) < 50) weap->o_hplus--;
                            else               weap->o_dplus--;
                            if (att_player)
                                msg(terse ? "Your %s weakens!"
                                          : "Your %s gets weaker!",
                                    weaps[weap->o_which].w_name);
                    }
            }
        }
                
        /* If the attacker hit something that shrieks, wake the dungeon */
        if (on(*def, CANSHRIEK)) {
            if (see_def)
                msg("%s emits an ear piercing shriek! ", prname(defname, TRUE));
            else
                msg("You hear an ear piercing shriek!");

            /* Friendly charactors should be immune */
            if (player.t_ctype == C_PALADIN ||
                player.t_ctype == C_RANGER  || player.t_ctype == C_MONK)
                    aggravate(TRUE, FALSE);
            else
                aggravate(TRUE, TRUE);
        }

        /*
         * does the creature explode when hit?
         */
        if (on(*def, CANEXPLODE)) {
            if (see_def) msg("%s explodes!", prname(defname, TRUE));
            else msg("You hear a tremendous explosion!");
            explode(def);
            if (pstats.s_hpt < 1) {
        pstats.s_hpt = -1;
                death(def->t_index);
        }
        }
    }

    /*
     * Now let's see what happens to the defender.  Start out with
     * the things that everyone can do.  Then exit if the attacker
     * is the player.
     */
    if (!thrown) {
        /* 
         * Can the player confuse? 
         */
        if (on(*att, CANHUH) && att_player) {
            msg("Your hands return to normal. ");
            if (off(*def, ISCLEAR) && 
               (off(*def, ISUNIQUE) || !save(VS_MAGIC, def, 0))) {
                if (see_def) msg("%s appears confused!", prname(defname, TRUE));
                turn_on(*def, ISHUH);
            }
            turn_off(*att, CANHUH);
        }

        /* Return now if the attacker is the player. */
        if (att_player) return(0);

        /*
         * Some monsters may take half your hit points
         */
        if (on(*att, CANSUCK) && !save(VS_MAGIC, def, 0)) {
            if (def->t_stats.s_hpt == 1) return(att->t_index); /* Killed! */
            else {
                def->t_stats.s_hpt /= 2;
                if (def_player)
                    msg("Your life force is being drained out of you.");
            }
        }

        /*
         * If a hugging monster hits, it may SQUEEEEEEEZE.
         */
        if (on(*att, CANHUG)) {
            if (roll(1,20) >= 18 || roll(1,20) >= 18) {
                if (def_player)
                    msg("%s squeezes itself nastily against you!",
                                prname(attname, TRUE));
                else if (see_att)
                    msg("%s squeezes real hard!", prname(attname, TRUE));

                if ((def->t_stats.s_hpt -= roll(2,8)) <= 0)
                    return(att->t_index);
            }
        }

        /*
         * Some monsters have poisonous bites.
         */
        if (on(*att, CANPOISON) && !save(VS_POISON, def, 0)) {
            if (def_player) {
                if (ISWEARING(R_SUSABILITY))
                    msg(terse ? "Sting has no effect."
                              : "A sting momentarily weakens your arm.");
                else {
                    chg_str(-1);
                    msg(terse ? "A sting has weakened you." :
                    "You get stung in the arm!  You feel weaker. ");
                }
            }
            else {
                /* Subtract a strength point and see if it kills it */
                if (--def->t_stats.s_str <= 0) return(D_STRENGTH);
            }
        }

        /*
         * Turning to stone:
         */
        if (on(*att, TOUCHSTONE)) {
            if (def_player) turn_off(*att, TOUCHSTONE);
            if (on(*def, CANINWALL)) {
                if (def_player)
                    msg("%s's touch has no effect.", prname(attname, TRUE));
            }
            else {
                if (!save(VS_PETRIFICATION, def, 0) && rnd(100) < 10) {
                    if (def_player) {
                        msg("Your body begins to solidify.. ");
                        msg("You are transformed into stone!! --More--");
                        wait_for(' ');
                        return(D_PETRIFY);
                    }
                    else {
                        /* The monster got stoned! */
                        turn_on(*def, ISSTONE);
                        turn_off(*def, ISRUN);
                        turn_off(*def, ISINVIS);
                        turn_off(*def, ISDISGUISE);
                        if (def->t_stats.s_intel > 15)
                            msg("%s staggers.. ", prname(defname, TRUE));
                        else if (see_def)
                            msg("%s turns to stone! ", prname(defname, TRUE));
                        else if (cansee(unc(def->t_pos)))
                            msg("A statue appears out of nowhere! ");
                    }
                }
                else if (def->t_action != A_FREEZE) {
                    if (def_player)
                        msg("%s's touch stiffens your limbs.",
                                        prname(attname, TRUE));
                    else if (see_def)
                        msg("%s appears to freeze over.", prname(defname, TRUE));

                    def->t_no_move += movement(def) * STONETIME;
                    def->t_action = A_FREEZE;
                }
            }
        }

        /*
         * Wraiths might drain energy levels
         */
        if ((on(*att, CANDRAIN) || on(*att, DOUBLEDRAIN)) && 
            !save(VS_POISON, def, 3-(att->t_stats.s_lvl/5))) {
            if (def_player) {
                lower_level(att->t_index);
                if (on(*att, DOUBLEDRAIN)) lower_level(att->t_index);
                turn_on(*att, DIDDRAIN);  
            }
            else {
                def->t_stats.s_hpt -= roll(1, 8);
                def->t_stats.s_lvl--;
                if (on(*att, DOUBLEDRAIN)) {
                    def->t_stats.s_hpt -= roll(1, 8);
                    def->t_stats.s_lvl--;
                }
                if (see_def)
                    msg("%s appears less skillful.", prname(defname, TRUE));

                /* Did it kill it? */
                if (def->t_stats.s_hpt <= 0 ||
                    def->t_stats.s_lvl <= 0)
                    return(att->t_index);
            }
        }

        /*
         * Paralyzation:
         */
        if (on(*att, CANPARALYZE) && def->t_action != A_FREEZE) {
            if (def_player) turn_off(*att, CANPARALYZE);
            if (!save(VS_PARALYZATION, def, 0)) {
                if (on(*def, CANINWALL)) {
                    if (def_player)
                        msg("%s's touch has no effect.", prname(attname, TRUE));
                }
                else {
                    if (def_player)
                        msg("%s's touch paralyzes you.", prname(attname, TRUE));
                    else if (see_def)
                        msg("%s appears to freeze over!", prname(defname, TRUE));

                    def->t_no_move += movement(def) * FREEZETIME;
                    def->t_action = A_FREEZE;
                }
            }
        }

        /*
         * Painful wounds make the defendant faint
         */
         if (on(*att, CANPAIN) && def->t_action != A_FREEZE) {
            if (def_player) turn_off(*att, CANPAIN);
            if (!ISWEARING(R_ALERT) && !save(VS_POISON, def, 0)) {
                    if (def_player)
                        msg("You faint from the painful wound!");
                    else if (see_def)
                        msg("%s appears to faint!", prname(defname, TRUE));

                    def->t_no_move += movement(def) * PAINTIME;
                    def->t_action = A_FREEZE;
            }
        }

        /*
         * Some things currently affect only the player.  Let's make
         * a check here so we don't have to check for each thing.
         */
        if (def_player) {
        /*
         * Stinking monsters make the defender weaker (to hit).  For now
         * this will only affect the player.  We may later add the HASSTINK
         * effect to monsters, too.
         */
            if (on(*att, CANSTINK)) {
                turn_off(*att, CANSTINK);
                if (!save(VS_POISON, def, 0)) {
                    msg("The stench of %s sickens you.  Blech!",
                                prname(attname, FALSE));
                    if (on(player, HASSTINK)) lengthen(unstink, STINKTIME);
                    else {
                        turn_on(player, HASSTINK);
                        fuse(unstink, (VOID *)NULL, STINKTIME, AFTER);
                    }
                }
            }

            /*
             * Chilling monster reduces strength each time.  This only
             * affects the player for now because of its temporary nature.
             */
            if (on(*att, CANCHILL)) {
                if (!ISWEARING(R_SUSABILITY) && !save(VS_POISON, def, 0)) {
                    msg("You cringe at %s's chilling touch.",
                                prname(attname, FALSE));
                    chg_str(-1);
                    if (lost_str++ == 0)
                        fuse(res_strength, (VOID *)NULL, CHILLTIME, AFTER);
                    else lengthen(res_strength, CHILLTIME);
                }
            }

            /*
             * Itching monsters reduce dexterity (temporarily).  This only
             * affects the player for now because of its temporary nature.
             */
            if (on(*att, CANITCH) && !save(VS_POISON, def, 0)) {
                msg("The claws of %s scratch you!", prname(attname, FALSE));
                if (ISWEARING(R_SUSABILITY)) {
                    msg("The scratch has no effect.");
                }
                else {
                    (*add_abil[A_DEXTERITY])(-1);
                }
            }

            /*
             * If a disease-carrying monster hits, there is a chance the
             * defender will catch the disease.  This only applies to the
             * player for now because of the temporary nature. Don't affect
             * the Ranger or Paladin.
             */
            if (on(*att, CANDISEASE) &&
                (rnd(def->t_stats.s_const) < att->t_stats.s_lvl) &&
                off(*def, HASDISEASE)) {
                    if (ISWEARING(R_HEALTH)             ||
                        player.t_ctype == C_PALADIN     ||
                        player.t_ctype == C_RANGER) {
                            msg("The wound heals quickly.");
                    }
                    else {
                        turn_on(*def, HASDISEASE);
                        fuse(cure_disease, (VOID *)NULL, roll(HEALTIME,SICKTIME), AFTER);
                        msg(terse ? "You have been diseased!"
                            : "You have contracted an annoying disease!");
                    }
            }

            /*
             * If a rusting monster hits, you lose armor.  This only applies to
             * the player because monsters don't wear armor (for now).
             */
            if (on(*att, CANRUST)) { 
                if (cur_armor != NULL                           &&
                    cur_armor->o_which != LEATHER               &&
                    cur_armor->o_which != STUDDED_LEATHER       &&
                    cur_armor->o_which != PADDED_ARMOR          &&
                    !(cur_armor->o_flags & ISPROT)              &&
                    cur_armor->o_ac < def->t_stats.s_arm+1) {
                        msg(terse ? "Your armor weakens."
                            : "Your armor becomes weaker.");
                        cur_armor->o_ac++;
                }
                if (cur_misc[WEAR_BRACERS] != NULL              &&
                    cur_misc[WEAR_BRACERS]->o_ac > 0            &&
                    !(cur_misc[WEAR_BRACERS]->o_flags & ISPROT)) {
                        cur_misc[WEAR_BRACERS]->o_ac--;
                        if (cur_misc[WEAR_BRACERS]->o_ac == 0) {
                            register struct linked_list *item;

                            for (item=pack; item!=NULL; item=next(item)) {
                                if (OBJPTR(item) == cur_misc[WEAR_BRACERS]) {
                                    detach(pack, item);
                                    o_discard(item);
                                    break;
                                }
                            }
                            msg ("Your bracers crumble apart!");
                            cur_misc[WEAR_BRACERS] = NULL;
                            inpack--;
                        }
                        else {
                            msg("Your bracers weaken!");
                        }
                }
            }

            /*
             * If can dissolve and hero has leather type armor.  This
             * also only applies to the player for now because of the
             * armor.
             */
            if (on(*att, CANDISSOLVE) && cur_armor != NULL &&
                (cur_armor->o_which == LEATHER            ||
                 cur_armor->o_which == STUDDED_LEATHER    ||
                 cur_armor->o_which == PADDED_ARMOR)      &&
                !(cur_armor->o_flags & ISPROT) &&
                cur_armor->o_ac < def->t_stats.s_arm+1) {
                msg(terse ? "Your armor dissolves!"
                    : "Your armor appears to have dissolved!");
                cur_armor->o_ac++;
            }

            /*
             * If an infesting monster hits you, you get a parasite or rot.
             * This will only affect the player until we figure out how to
             * make it affect monsters.  Don't affect the Monk.
             */
            if (on(*att, CANINFEST) &&
                rnd(def->t_stats.s_const) < att->t_stats.s_lvl) {
                if (ISWEARING(R_HEALTH) || player.t_ctype == C_MONK) {
                        msg("The wound heals quickly.");
                }
                else {
                    turn_off(*att, CANINFEST);
                    msg(terse ? "You have been infested."
                        : "You have contracted a parasitic infestation!");
                    infest_dam++;
                    turn_on(*def, HASINFEST);
                }
            }

            /*
             * Does it take wisdom away?  This currently affects only
             * the player because of its temporary nature.
             */
            if (on(*att, TAKEWISDOM)            && 
                !save(VS_MAGIC, def, 0) &&
                !ISWEARING(R_SUSABILITY)) {
                        (*add_abil[A_WISDOM])(-1);
            }

            /*
             * Does it take intelligence away?  This currently affects
             * only the player because of its temporary nature.
             */
            if (on(*att, TAKEINTEL)             && 
                !save(VS_MAGIC, &player, 0)     &&
                !ISWEARING(R_SUSABILITY)) {
                        (*add_abil[A_INTELLIGENCE])(-1);
            }

            /*
             * Cause fear by touching.  This currently affects only
             * the player until we figure out how we want it to
             * affect monsters.
             */
            if (on(*att, TOUCHFEAR)) {
                turn_off(*att, TOUCHFEAR);
                if (!ISWEARING(R_HEROISM)       &&
                    !save(VS_WAND, def, 0)      &&
                    !(on(*def, ISFLEE) && (def->t_dest == &att->t_pos))) {
                        turn_on(*def, ISFLEE);
                        def->t_dest = &att->t_pos;
                        msg("%s's touch terrifies you!", prname(attname, TRUE));

                        /* It is okay to turn tail */
                        if (!def_player) def->t_oldpos = def->t_pos;
                }
            }

            /*
             * Make the hero dance (as in otto's irresistable dance)
             * This should be fairly easy to do to monsters, but
             * we'll restrict it to players until we decide what to
             * do about the temporary nature.
             */
            if (on(*att, CANDANCE)              && 
                !on(*def, ISDANCE)              &&
                def->t_action != A_FREEZE       &&
                !save(VS_MAGIC, def, -4)) {
                    turn_off(*att, CANDANCE);
                    turn_on(*def, ISDANCE);
                    msg("You begin to dance uncontrollably!");
                    fuse(undance, (VOID *)NULL, roll(2,4), AFTER);
            }

            /*
             * Suffocating our hero.  Monsters don't get suffocated.
             * That's too hard for now.
             */
            if (on(*att, CANSUFFOCATE)          && 
                !ISWEARING(R_FREEDOM)           && 
                rnd(100) < 30                   &&
                (find_slot(suffocate) == 0)) {
                turn_on(*att, DIDSUFFOCATE);
                msg("%s is beginning to suffocate you!", prname(attname, TRUE));
                fuse(suffocate, (VOID *)NULL, roll(9,3), AFTER);
            }

            /*
             * some creatures stops the poor guy from moving.
             * How can we do this to a monster?
             */
            if (on(*att,CANHOLD) && off(*att,DIDHOLD) && !ISWEARING(R_FREEDOM)){
                turn_on(*def, ISHELD);
                turn_on(*att, DIDHOLD);
                hold_count++;
            }

            /*
             * Sucker will suck blood and run.  This
             * should be easy to have happen to a monster,
             * but we have to decide how to handle the fleeing.
             */
            if (on(*att, CANDRAW)) {
                turn_off(*att, CANDRAW);
                turn_on(*att, ISFLEE);
                msg("%s sates itself with your blood!", prname(attname, TRUE));
                if ((def->t_stats.s_hpt -= 12) <= 0) return(att->t_index);

                /* It is okay to turn tail */
                att->t_oldpos = att->t_pos;
            }

            /*
             * Bad smell will force a reduction in strength.
             * This will happen only to the player because of
             * the temporary nature.
             */
            if (on(*att, CANSMELL)) {
                turn_off(*att, CANSMELL);
                if (save(VS_MAGIC, def, 0) || ISWEARING(R_SUSABILITY)) {
                    if (terse)
                        msg("Pheww!");
                    else
                        msg("You smell an unpleasant odor.  Phew!");
                    }

                else {
                    int odor_str = -(rnd(6)+1);

                    msg("You are overcome by a foul odor!");
                    if (lost_str == 0) {
                        chg_str(odor_str);
                        fuse(res_strength, (VOID *)NULL, SMELLTIME, AFTER);
                        lost_str -= odor_str;
                    }
                    else lengthen(res_strength, SMELLTIME);
                }
            }

            /*
             * The monsters touch slows the defendant down.
             */
             if (on(*att, TOUCHSLOW)) {
                turn_off(*att, TOUCHSLOW);
                if (!save(VS_PARALYZATION, def, 0)) 
                        add_slow();
            }

            /*
             * Rotting only affects the player.  Don't affect the Monk,
             * Paladin, or Ranger.
             */
            if (on(*att, CANROT)) {
                if (!ISWEARING(R_HEALTH)        && 
                    player.t_ctype != C_MONK    &&
                    player.t_ctype != C_RANGER  &&
                    player.t_ctype != C_PALADIN &&
                    !save(VS_POISON, def, 0)    && 
                    off(*def, DOROT)) {
                    turn_on(*def, DOROT);
                    msg("You feel your skin starting to rot and peel away!");
                }
            }

            /*
             * Monsters should be able to steal gold from anyone,
             * but until this is rewritten, they will only steal
             * from the player (tough break).
             */
            if (on(*att, STEALGOLD)) {
                /*
                 * steal some gold
                 */
                register long lastpurse;
                register struct linked_list *item;
                register struct object *obj;

                lastpurse = purse;
                purse -= (GOLDCALC * 2);
                if (!save(VS_MAGIC, def, att->t_stats.s_lvl/10)) {
                    if (on(*att, ISUNIQUE))
                        purse -= (GOLDCALC * 5);
                    else
            purse -= (GOLDCALC * 3);
                }
                if (purse < 0)
                    purse = 0;
                if (purse != lastpurse) {
                    msg("You lost some gold! ");

                    /* Give the gold to the thief */
                    for (item=att->t_pack; item != NULL; item=next(item)) {
                        obj = OBJPTR(item);
                        if (obj->o_type == GOLD) {
                            obj->o_count += lastpurse - purse;
                            break;
                        }
                    }

                    /* Did we do it? */
                    if (item == NULL) { /* Then make some */
                        item = new_item(sizeof *obj);
                        obj = OBJPTR(item);
                        obj->o_type = GOLD;
                        obj->o_count = lastpurse - purse;
                        obj->o_hplus = obj->o_dplus = 0;
                        strcpy(obj->o_damage,"0d0");
                        strcpy(obj->o_hurldmg,"0d0");
                        obj->o_ac = 11;
                        obj->contents = NULL;
                        obj->o_group = 0;
                        obj->o_flags = 0;
                        obj->o_mark[0] = '\0';
                        obj->o_pos = att->t_pos;

                        attach(att->t_pack, item);
                    }
                }

                turn_on(*att, ISFLEE);
                turn_on(*att, ISINVIS);

                /* It is okay to turn tail */
                att->t_oldpos = att->t_pos;
            }
        }

        /*
         * Stealing happens last since the monster disappears
         * after the act.
         */
        if (on(*att, STEALMAGIC)) {
            register struct linked_list *list, *steal;
            register struct object *obj;
            register int nobj;

            /*
             * steal a magic item, look through the pack
             * and pick out one we like.
             */
            steal = NULL;
            for (nobj = 0, list = def->t_pack; list != NULL; list = next(list))
            {
                obj = OBJPTR(list);
                if (!is_current(obj)     &&
                    list != def->t_using &&
                    obj->o_type != RELIC &&
                    is_magic(obj)        && 
                    rnd(++nobj) == 0)
                        steal = list;
            }
            if (steal != NULL)
            {
                register struct object *obj;
                struct linked_list *item;

                obj = OBJPTR(steal);
                if (on(*att, ISUNIQUE))
                    monsters[att->t_index].m_normal = TRUE;
                item = find_mons(att->t_pos.y, att->t_pos.x);

                killed(item, FALSE, FALSE, FALSE); /* Remove the attacker */

                if (obj->o_count > 1 && obj->o_group == 0) {
                    register int oc;

                    oc = --(obj->o_count);
                    obj->o_count = 1;
                    if (def_player)
                        msg("%s stole %s!", prname(attname, TRUE),
                                        inv_name(obj, TRUE));
                    obj->o_count = oc;
                }
                else {
                    if (def_player) {
                        msg("%s stole %s!", prname(attname, TRUE),
                                        inv_name(obj, TRUE));

                        /* If this is a relic, clear its holding field */
                        if (obj->o_type == RELIC)
                            cur_relic[obj->o_which] = 0;

                        inpack--;
                    }

                    detach(def->t_pack, steal);
                    o_discard(steal);
                }

                updpack(FALSE, def);
            }
        }
    }

    /* Didn't kill the defender */
    return(0);
}

