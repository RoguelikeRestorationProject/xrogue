/*
    eat.c  -  Functions for dealing with digestion

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
 * eat:
 *      He wants to eat something, so let him try
 */

eat()
{
    register struct linked_list *item;
    int which;
    unsigned long temp;

    if (player.t_action != C_EAT) {
        if ((item = get_item(pack, "eat", FOOD, FALSE, FALSE)) == NULL)
            return;

        player.t_using = item;  /* Remember what it is */
        player.t_action = C_EAT;        /* We are eating */
        which = (OBJPTR(item))->o_which;
        player.t_no_move = max(foods[which].mi_food/100, 1) * movement(&player);
        return;
    }

    /* We have waited our time, let's eat the food */
    item = player.t_using;
    player.t_using = NULL;
    player.t_action = A_NIL;

    which = (OBJPTR(item))->o_which;
    if ((food_left += foods[which].mi_food) > STOMACHSIZE)
        food_left = STOMACHSIZE;
    del_pack(item);
    if (hungry_state == F_SATIATED && food_left == STOMACHSIZE && rnd(4) == 1) {
        pstats.s_hpt = -1;
        msg ("Cough!  Ack!  You choke on all that food and die!  --More--");
        wait_for(' ');
        death(D_FOOD_CHOKE);
    }
    if (food_left >= STOMACHSIZE-MORETIME) {
        hungry_state = F_SATIATED;
        msg ("You have trouble getting that food down!");
        msg ("Your stomach feels like it's about to burst!");
    }
    else if (which != E_SLIMEMOLD) {
        hungry_state = F_OKAY;
        switch (rnd(10)) {
        case 0: msg("Yuck, what a foul tasting %s! ", foods[which].mi_name);
        when 1: msg("Mmmm, what a tasty %s. ", foods[which].mi_name);
        when 2: msg("Wow, what a scrumptious %s! ", foods[which].mi_name);
        when 3: msg("Hmmm, %s heaven! ", foods[which].mi_name);
        when 4: msg("You've eaten better %s. ", foods[which].mi_name);
        when 5: msg("You smack your lips ");
        when 6: msg("Yum-yum-yum ");
        when 7: msg("Gulp! ");
        when 8: msg("Your tongue flips out! ");
        when 9: msg("You lick your chin ");
        }
    }
    updpack(TRUE, &player);
    switch(which) {
    case E_WHORTLEBERRY:    /* add 1 to intelligence */
        (*add_abil[A_INTELLIGENCE])(1);
    when E_SWEETSOP:    /* add 1 to strength */
    case E_SOURSOP: /* add 1 to strength */
        (*add_abil[A_STRENGTH])(1);
    when E_SAPODILLA:   /* add 1 to wisdom */
        (*add_abil[A_WISDOM])(1);
    when E_APPLE:   /* add 1 to dexterity */
        (*add_abil[A_DEXTERITY])(1);
    when E_PRICKLEY:    /* add 1 to constitution */
        (*add_abil[A_CONSTITUTION])(1);
    when E_PEACH:   /* add 1 to charisma */
        (*add_abil[A_CHARISMA])(1);
    when E_PITANGA: /* add 1 hit point */
        max_stats.s_hpt++;
        pstats.s_hpt = max_stats.s_hpt;
        msg("You feel a bit tougher now. ");
    when E_HAGBERRY:    /* armor class */
    case E_JABOTICABA:  /* armor class */
        pstats.s_arm--;
        msg("Your skin feels more resilient now. ");
    when E_STRAWBERRY:  /* add 10% experience points */
    case E_RAMBUTAN:    /* add 10% experience points */
        temp = pstats.s_exp/100 + 10;
        pstats.s_exp += temp;
        msg("You feel slightly more experienced now. ");
        check_level();
    when E_DEWBERRY:    /* encourage him to do more magic */
        if (chant_time > 0) {
            chant_time -= 80;
            if (chant_time < 0)
                chant_time = 0;
            msg("You feel you have more chant ability. ");
        }
        if (pray_time > 0) {
            pray_time -= 80;
            if (pray_time < 0)
                pray_time = 0;
            msg("You feel you have more prayer ability. ");
        }
        if (spell_power > 0) {
            spell_power -= 80;
            if (spell_power < 0)
                spell_power = 0;
            msg("You feel you have more spell casting ability. ");
        }
    when E_CANDLEBERRY: /* cure him */
        if (on(player, HASINFEST) || 
            on(player, HASDISEASE)|| 
            on(player, DOROT)) {
            if (on(player, HASDISEASE)) {
                extinguish(cure_disease);
                cure_disease();
            }
            if (on(player, HASINFEST)) {
                msg("You feel yourself improving. ");
                turn_off(player, HASINFEST);
                infest_dam = 0;
            }
            if (on(player, DOROT)) {
                msg("You feel your skin returning to normal. ");
                turn_off(player, DOROT);
            }
        }
    when E_SLIMEMOLD: /* monster food */
    msg("The slime-mold quivers around in your mouth. ");
        player.t_no_move = 3*movement(&player);
        if (off(player, HASDISEASE)) {
            if (ISWEARING(R_HEALTH) || player.t_ctype == C_PALADIN ||
                player.t_ctype == C_RANGER) {
        msg("You feel lousy. ");
            }
            else {
                turn_on(player, HASDISEASE);
                fuse(cure_disease, (VOID *)NULL, roll(HEALTIME,SICKTIME),AFTER);
                msg("You become ill. ");
            }
    }
        pstats.s_const -= rnd(2)+1;
    if (pstats.s_const <= 3) pstats.s_const = 3;
    
    otherwise: /* not all the foods have to do something */
        break;
    }
}

