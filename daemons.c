/*
    daemons.c - All the daemon and fuse functions are in here
  
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
 * doctor:
 *      A healing daemon that restors hit points after rest
 */

doctor(tp)
register struct thing *tp;
{
    register int ohp;
    register int limit, new_points;
    register struct stats *curp; /* current stats pointer */
    register struct stats *maxp; /* max stats pointer */

    curp = &(tp->t_stats);
    maxp = &(tp->maxstats);
    if (curp->s_hpt == maxp->s_hpt) {
        tp->t_quiet = 0;
        return;
    }
    tp->t_quiet++;
    switch (tp->t_ctype) {
        case C_MAGICIAN:
            limit = 10 - curp->s_lvl;
            new_points = curp->s_lvl - 2;
        when C_CLERIC:
        case C_DRUID:
            limit = 12 - curp->s_lvl;
            new_points = curp->s_lvl - 3;
        when C_THIEF:
        case C_ASSASSIN:
            limit = 14 - curp->s_lvl;
            new_points = curp->s_lvl - 4;
        when C_MONK:
            limit = 16 - curp->s_lvl;
            new_points = curp->s_lvl - 5;
        when C_RANGER:
        case C_PALADIN:
            limit = 18 - curp->s_lvl;
            new_points = curp->s_lvl - 6;
        when C_FIGHTER:
            limit = 20 - curp->s_lvl;
            new_points = curp->s_lvl - 7;
        when C_MONSTER:
            limit = 15 - curp->s_lvl;
            new_points = curp->s_lvl - 5;
        otherwise:
            debug("what a strange character you are!");
            return;
    }
    ohp = curp->s_hpt;
    if (off(*tp, HASDISEASE) && off(*tp, DOROT)) {
        if (curp->s_lvl < 8) {
            if (tp->t_quiet > limit) {
                curp->s_hpt++;
                tp->t_quiet = 0;
            }
        }
        else {
            if (tp->t_quiet >= 3) {
                curp->s_hpt += rnd(new_points)+1;
                tp->t_quiet = 0;
            }
        }
    }
    if (tp == &player) {
        if (ISRING(LEFT_1, R_REGEN)) curp->s_hpt++;
        if (ISRING(LEFT_2, R_REGEN)) curp->s_hpt++;
        if (ISRING(LEFT_3, R_REGEN)) curp->s_hpt++;
        if (ISRING(LEFT_4, R_REGEN)) curp->s_hpt++;
        if (ISRING(RIGHT_1, R_REGEN)) curp->s_hpt++;
        if (ISRING(RIGHT_2, R_REGEN)) curp->s_hpt++;
        if (ISRING(RIGHT_3, R_REGEN)) curp->s_hpt++;
        if (ISRING(RIGHT_4, R_REGEN)) curp->s_hpt++;
    }
    if (on(*tp, ISREGEN))
        curp->s_hpt += curp->s_lvl/10 + 1;
    if (ohp != curp->s_hpt) {
        if (curp->s_hpt >= maxp->s_hpt) {
            curp->s_hpt = maxp->s_hpt;
            if (off(*tp, WASTURNED) && on(*tp, ISFLEE) && tp != &player) {
                turn_off(*tp, ISFLEE);
                tp->t_oldpos = tp->t_pos;       /* Start our trek over */
            }
        }
    }
}

/*
 * Swander:
 *      Called when it is time to start rolling for wandering monsters
 */

swander()
{
    daemon(rollwand, (VOID *)NULL, BEFORE);
}

/*
 * rollwand:
 *      Called to roll to see if a wandering monster starts up
 */

int between = 0;

rollwand()
{

    if (++between >= 4)
    {
        /* Theives may not awaken a monster */
        if ((roll(1, 6) == 4) &&
           ((player.t_ctype != C_THIEF && player.t_ctype != C_ASSASSIN) || 
            (rnd(30) >= dex_compute()))) {
            if (levtype != POSTLEV)
                wanderer();
            kill_daemon(rollwand);
            fuse(swander, (VOID *)NULL, WANDERTIME, BEFORE);
        }
        between = 0;
    }
}

/*
 * this function is a daemon called each turn when the character is a thief
 */

trap_look()
{
    if (rnd(100) < (2*dex_compute() + 5*pstats.s_lvl))
        search(TRUE, FALSE);
}

/*
 * unconfuse:
 *      Release the poor player from his confusion
 */

unconfuse()
{
    turn_off(player, ISHUH);
    msg("You feel less confused now");
}

/*
 * unsee:
 *      He lost his see invisible power
 */

unsee()
{
    if (!ISWEARING(R_SEEINVIS)) {
        turn_off(player, CANSEE);
        msg("The tingling feeling leaves your eyes");
    }
}

/*
 * unstink:
 *      Remove to-hit handicap from player
 */

unstink()
{
    turn_off(player, HASSTINK);
}

/*
 * unclrhead:
 *      Player is no longer immune to confusion
 */

unclrhead()
{
    turn_off(player, ISCLEAR);
    msg("The blue aura about your head fades away.");
}

/*
 * unphase:
 *      Player can no longer walk through walls
 */

unphase()
{
    turn_off(player, CANINWALL);
    msg("Your dizzy feeling leaves you.");
    if (!step_ok(hero.y, hero.x, NOMONST, &player)) 
        msg("You begin to feel weird.. ");
}

/*
 * land:
 *      Player can no longer fly
 */

int
land()
{
    turn_off(player, ISFLY);
    msg("You regain your normal weight");
    running = FALSE;
    return(0);
}

/*
 * sight:
 *      He gets his sight back
 */

sight()
{
    if (on(player, ISBLIND))
    {
        extinguish(sight);
        turn_off(player, ISBLIND);
        light(&hero);
        msg("The veil of darkness lifts");
    }
}

/*
 * res_strength:
 *      Restore player's strength
 */

int
res_strength(howmuch)
long howmuch;
{

    /* If lost_str is non-zero, restore that amount of strength,
     * else all of it 
     */
    if (lost_str) {
        chg_str(lost_str);
        lost_str = 0;
    }

    /* Now, add in the restoral, but no greater than maximum strength */
    if (howmuch > 0)
        pstats.s_str =
            min(pstats.s_str + howmuch, max_stats.s_str + ring_value(R_ADDSTR));

    updpack(TRUE, &player);
    return(0);
}

/*
 * nohaste:
 *      End the hasting
 */

nohaste()
{
    turn_off(player, ISHASTE);
    msg("You feel yourself slowing down.");
}

/*
 * noslow:
 *      End the slowing
 */

noslow()
{
    turn_off(player, ISSLOW);
    msg("You feel yourself speeding up.");
}

/*
 * suffocate:
 *      If this gets called, the player has suffocated
 */

suffocate()
{
    pstats.s_hpt = -1;
    death(D_SUFFOCATION);
}

/*
 * digest the hero's food
 */

stomach()
{
    register int oldfood, old_hunger, food_use, i;

    /* 
     * avoid problems of fainting while eating by just not saying it
     * takes food to eat food
     */
    if (player.t_action == C_EAT) 
        return;

    old_hunger = hungry_state;
    if (food_left <= 0)
    {
        /*
         * the hero is fainting
         */
        if (player.t_action == A_FREEZE)
                return;
        if (rnd(100) > 12)
            return;
        if (hungry_state == F_FAINT && rnd(28) == 7) /*must have fainted once*/
    {
        pstats.s_hpt = -1;
        msg("You starve to death!!  --More-- ");
        wait_for(' ');
                death(D_STARVATION);
    }
        player.t_action = A_FREEZE;
        player.t_no_move = movement(&player) * (rnd(8) + 3);
        if (!terse)
            addmsg("You feel too weak from the lack of food. ");
        msg("You faint");
        running = FALSE;
        if (fight_flush) flushinp();
        count = 0;
        hungry_state = F_FAINT;
    }
    else
    {
        oldfood = food_left;
        food_use = 0;
        for (i=0; i<MAXRELIC; i++) { /* each relic eats an additional food */
            if (cur_relic[i])
                food_use++;
        }
        /* Charge for wearing rings */
        food_use +=    (ring_eat(LEFT_1)  + ring_eat(LEFT_2)  +
                        ring_eat(LEFT_3)  + ring_eat(LEFT_4)  +
                        ring_eat(RIGHT_1) + ring_eat(RIGHT_2) +
            ring_eat(RIGHT_3) + ring_eat(RIGHT_4) +
                        foodlev);
        if (food_use < 1)
            food_use = 1;
        food_left -= food_use;
        if (food_left < MORETIME && oldfood >= MORETIME) {
            msg("You are starting to feel weak");
            running = FALSE;
            if (fight_flush) flushinp();
            count = 0;
            hungry_state = F_WEAK;
        }
        else if (food_left < 2 * MORETIME && oldfood >= 2 * MORETIME)
        {
            msg(terse ? "Getting hungry" : "You are starting to get hungry");
            running = FALSE;
            hungry_state = F_HUNGRY;
        }
        else if(food_left<STOMACHSIZE-MORETIME && oldfood>=STOMACHSIZE-MORETIME)
        {
            hungry_state = F_OKAY;
        }
    }
    if (old_hunger != hungry_state)  {
        updpack(TRUE, &player);
        status(TRUE);
    }
    wghtchk();
}

/*
 * daemon for curing the diseased
 */

cure_disease()
{
    turn_off(player, HASDISEASE);
    if (off (player, HASINFEST))
        msg(terse ? "You feel yourself improving"
                : "You begin to feel yourself improving again");
}

/*
 * appear:
 *      Become visible again
 */
 
appear()
{
    turn_off(player, ISINVIS);
    PLAYER = VPLAYER;
    msg("The tingling feeling leaves your body");
    light(&hero);
}

/*
 * dust_appear:
 *      dust of disappearance wears off
 */
 
dust_appear()
{
    turn_off(player, ISINVIS);
    PLAYER = VPLAYER;
    msg("You become visible again");
    light(&hero);
}

/*
 * unchoke:
 *      the effects of "dust of choking and sneezing" wear off
 */
 
unchoke()
{
    if (!find_slot(unconfuse))
        turn_off(player, ISHUH);
    if (!find_slot(sight))
        turn_off(player, ISBLIND);
    light(&hero);
    msg("Your throat and eyes return to normal");
}

/*
 * make some potion for the guy in the Alchemy jug
 */
 
alchemy(obj)
register struct object *obj;
{
    register struct object *tobj = NULL;
    register struct linked_list *item;

    /*
     * verify that the object pointer we have still points to an alchemy
     * jug (hopefully the right one!) because the hero could have thrown
     * it away
     */
    for (item = pack; item != NULL; item = next(item)) {
        tobj = OBJPTR(item);
        if (tobj         == obj         && 
            tobj->o_type == MM          && 
            tobj->o_which== MM_JUG      &&
            tobj->o_ac   == JUG_EMPTY   )
                break;
    }
    if (item == NULL) {         /* not in the pack, check the level */
        for (item = lvl_obj; item != NULL; item = next(item)) {
            tobj = OBJPTR(item);
            if (tobj         == obj             && 
                tobj->o_type == MM              && 
                tobj->o_which== MM_JUG          &&
                tobj->o_ac   == JUG_EMPTY       )
                    break;
        }
    }
    if (item == NULL)   /* can't find it.....too bad */
        return;
    
    switch(rnd(11)) {
        case 0: tobj->o_ac = P_PHASE;
        when 1: tobj->o_ac = P_CLEAR;
        when 2: tobj->o_ac = P_SEEINVIS;
        when 3: tobj->o_ac = P_HEALING;
        when 4: tobj->o_ac = P_MFIND;
        when 5: tobj->o_ac = P_TFIND;
        when 6: tobj->o_ac = P_HASTE;
        when 7: tobj->o_ac = P_RESTORE;
        when 8: tobj->o_ac = P_FLY;
        when 9: tobj->o_ac = P_SKILL;
        when 10:tobj->o_ac = P_FFIND;
    }
}

/*
 * otto's irresistable dance wears off 
 */
 
int
undance()
{
    turn_off(player, ISDANCE);
    msg ("Your feet take a break.....whew!");
    return(0);
}

/* 
 * if he has our favorite necklace of strangulation then take damage every turn
 */
 
strangle()
{
     if ((pstats.s_hpt -= 6) <= 0) {
     pstats.s_hpt = -1;
     death(D_STRANGLE);
     }
}

/*
 * if he has on the gauntlets of fumbling he might drop his weapon each turn
 */
 
fumble()
{
    register struct linked_list *item;

    if (cur_weapon!=NULL                        &&
        !(cur_weapon->o_flags & ISCURSED)       &&
        cur_weapon->o_type != RELIC             &&
        rnd(100)<3) {
        for (item = pack; item != NULL; item = next(item)) {
            if (OBJPTR(item) == cur_weapon)
                break;
        }
        if (item != NULL) {
            switch(mvwinch(stdscr, hero.y, hero.x)) {
            case PASSAGE: 
            case SCROLL:
            case POTION:
            case WEAPON:
            case FLOOR:
            case STICK:
            case ARMOR:
            case POOL:
            case RELIC:
            case GOLD:
            case FOOD:
            case RING:
            case MM:
                drop(item);
                running = FALSE;
                break;
            default:
                break;
            }
        }
    }
}

/*
 * This is called each turn the hero has the ring of searching on
 * it's a lot like trap_look() 
 */
 
ring_search()
{
    if (rnd(75) < (2*dex_compute() + 5*pstats.s_lvl)) search(TRUE, FALSE);
    else search(FALSE, FALSE);
}

/*
 * this is called each turn the hero has the ring of teleportation on
 */
 
ring_teleport()
{
    if (rnd(100) < 3) teleport();
}

/* 
 * this is called to charge up the quill of Nagrom
 */
 
quill_charge()
{
    register struct object *tobj = NULL;
    register struct linked_list *item;

    /*
     * find the Quill of Nagrom in the hero's pack. It should still be there
     * because it can't be dropped. If its not then don't do anything.
     */
    for (item = pack; item != NULL; item = next(item)) {
        tobj = OBJPTR(item);
        if (tobj->o_type == RELIC && tobj->o_which == QUILL_NAGROM)
                break;
    }
    if (item == NULL)
        return;
    if (tobj->o_charges < QUILLCHARGES)
        tobj->o_charges++;
    fuse (quill_charge, (VOID *)NULL, player.t_ctype == C_MAGICIAN ? 4 : 8, AFTER);
}

/*
 * take the skills away gained (or lost) by the potion of skills
 */
 
unskill()
{
    if (pstats.s_lvladj != 0) {
        pstats.s_lvl -= pstats.s_lvladj;
        pstats.s_lvladj = 0;
        msg("You feel your normal skill level return.");
        status(TRUE);
    }
}

/*
 * charge up the cloak of Emori
 */
 
int
cloak_charge(obj)
register struct object *obj;
{
    if (obj->o_charges < 1)
        obj->o_charges = 1;
    return(0);
}

/*
 * nofire:
 *      He lost his fire resistance
 */
 
nofire()
{
    if (!ISWEARING(R_FIRE)) {
        turn_off(player, NOFIRE);
        msg("Your feeling of fire resistance leaves you");
    }
}

/*
 * nocold:
 *      He lost his cold resistance
 */
 
nocold()
{
    if (!ISWEARING(R_WARMTH)) {
        turn_off(player, NOCOLD);
        msg("Your feeling of warmth leaves you");
    }
}

/*
 * nobolt:
 *      He lost his protection from lightning
 */
 
nobolt()
{
    turn_off(player, NOBOLT);
    msg("Your skin loses its bluish tint");
}

/*
 * eat_gold:
 *      an artifact eats gold 
 */
 
eat_gold(obj)
register struct object *obj;
{
    if (purse == 250)
        msg("%s.. Bids you to find some more gold. ", inv_name(obj, FALSE));
    if (purse == 100)
        msg("%s.. Demands that you find more gold! ", inv_name(obj, FALSE));
    if (purse == 50)
        msg("%s.. Commands you to find more gold!! ", inv_name(obj, FALSE));
    if (purse == 0) {
    if (rnd(10) >= 7)
        msg("You feel the artifact gnawing away... ");
        if (--pstats.s_hpt < 1) {
        pstats.s_hpt = -1;
            death(D_RELIC);
    }
    }
    else
        purse--;
}

/*
 * give the hero back some spell points
 */
 
spell_recovery()
{
    int time;

    time = SPELLTIME - max(17-pstats.s_intel, 0);
    time = max(time, 5);
    if (spell_power > 0) spell_power--;
    fuse(spell_recovery, (VOID *)NULL, time, AFTER);
}

/*
 * give the hero back some prayer points
 */
 
prayer_recovery()
{
    int time;

    time = SPELLTIME - max(17-pstats.s_wisdom, 0);
    time = max(time, 5);
    if (pray_time > 0) pray_time--;
    fuse(prayer_recovery, (VOID *)NULL, time, AFTER);
}

/*
 * give the hero back some chant points
 */
 
chant_recovery()
{
    int time;

    time = SPELLTIME - max(17-pstats.s_wisdom, 0);
    time = max(time, 5);
    if (chant_time > 0) chant_time--;
    fuse(chant_recovery, (VOID *)NULL, time, AFTER);
}

