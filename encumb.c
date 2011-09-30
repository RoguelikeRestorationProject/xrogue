/*
    encumb.c - Stuff to do with encumberance
    
    XRogue: Expeditions into the Dungeons of Doom
    Copyright (C) 1991 Robert Pietkivitch
    All rights reserved.
    
    Based on "Advanced Rogue"
    Copyright (C) 1984, 1985 Michael Morgan, Ken Dalka and AT&T
    All rights reserved.

    See the file LICENSE.TXT for full copyright and licensing information.
*/

#include <curses.h>
#include "rogue.h"

/*
 * updpack:
 *      Update his pack weight and adjust fooduse accordingly
 */

updpack(getmax, tp)
int getmax;
struct thing *tp;
{

        reg int topcarry, curcarry;

        if (getmax)
            tp->t_stats.s_carry = totalenc(tp); /* get total encumb */
        curcarry = packweight(tp);              /* get pack weight */

        /* Only update food use for the player (for now) */
        if (tp == &player) {
            topcarry = tp->t_stats.s_carry / 5; /* 20% of total carry */
            if(curcarry > 4 * topcarry) {
                if(rnd(100) < 80)
                    foodlev = 3;                        /* > 80% of pack */
            } else if(curcarry > 3 * topcarry) {
                if(rnd(100) < 60)
                    foodlev = 2;                        /* > 60% of pack */
            } else
                foodlev = 1;                    /* <= 60% of pack */
        }
        tp->t_stats.s_pack = curcarry;          /* update pack weight */
}


/*
 * packweight:
 *      Get the total weight of the hero's pack
 */

packweight(tp)
register struct thing *tp;
{
        reg struct object *obj;
        reg struct linked_list *pc;
        reg int weight;

        weight = 0;
        for (pc = tp->t_pack ; pc != NULL ; pc = next(pc)) {
            obj = OBJPTR(pc);
            weight += itemweight(obj);
        }
        if (weight < 0)         /* in case of amulet */
             weight = 0;

        /* If this is the player, is he wearing a ring of carrying? */
        if (tp == &player && ISWEARING(R_CARRY)) {
            register int temp, i;

            temp = 0;
            for (i=0; i<NUM_FINGERS; i++) {
                if (cur_ring[i] != NULL && cur_ring[i]->o_which == R_CARRY) {
                    if (cur_ring[i]->o_flags & ISCURSED) temp--;
                    else temp += 2;
                }
            }
            weight -= (temp * weight) / 4;
        }

        return(weight);
}

/*
 * itemweight:
 *      Get the weight of an object
 */

itemweight(wh)
reg struct object *wh;
{
        reg int weight;
        reg int ac;

        weight = wh->o_weight;          /* get base weight */
        switch(wh->o_type) {
            case ARMOR:
                /*
                 * subtract 10% for each enchantment
                 * this will add weight for negative items
                 */
                ac = armors[wh->o_which].a_class - wh->o_ac;
                weight = ((weight*10) - (weight*ac)) / 10;
                if (weight < 0) weight = 0;
            when WEAPON:
                if ((wh->o_hplus + wh->o_dplus) > 0)
                        weight /= 2;
        }
        if(wh->o_flags & ISCURSED)
                weight += weight / 5;   /* 20% more for cursed */
        weight *= wh->o_count;
        return(weight);
}

/*
 * playenc:
 *      Get hero's carrying ability above norm
 */

playenc(tp)
register struct thing *tp;
{
        register int strength;

        if (tp == &player) strength = str_compute();
        else strength = tp->t_stats.s_str;

        return ((strength-8)*50);
}

/*
 * totalenc:
 *      Get total weight that the hero can carry
 */

totalenc(tp)
register struct thing *tp;
{
        reg int wtotal;

        wtotal = NORMENCB + playenc(tp);
        if (tp == &player) switch(hungry_state) {
                case F_SATIATED:
                case F_OKAY:
                case F_HUNGRY:  ;                       /* no change */
                when F_WEAK:    wtotal -= wtotal / 10;  /* 10% off weak */
                when F_FAINT:   wtotal /= 2;            /* 50% off faint */
        }
        return(wtotal);
}

/*
 * whgtchk:
 *      See if the hero can carry his pack
 */

wghtchk()
{
        reg int dropchk, err = TRUE;
        reg char ch;
        int wghtchk();

        inwhgt = TRUE;
        if (pstats.s_pack > pstats.s_carry) {
            ch = mvwinch(stdscr, hero.y, hero.x);
            if((ch != FLOOR && ch != PASSAGE)) {
                extinguish(wghtchk);
                fuse(wghtchk, (VOID *)NULL, 1, AFTER);
                inwhgt = FALSE;
                return;
            }
            extinguish(wghtchk);
            msg("Your pack is far too heavy for you.. ");
            do {
                dropchk = drop((struct linked_list *)NULL);
                if(dropchk == 0) {
                    mpos = 0;
                    msg("You must drop something");
                }
                if(dropchk == TRUE)
                    err = FALSE;
            } while(err);
        }
        inwhgt = FALSE;
}

/*
 * hitweight:
 *      Gets the fighting ability according to current weight
 *      This returns a  +1 hit for light pack weight
 *                       0 hit for medium pack weight
 *                      -1 hit for heavy pack weight
 */

hitweight()
{
        return(2 - foodlev);
}

