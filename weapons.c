/*
    weapons.c - Functions for dealing with problems brought about by weapons
    
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

boomerang(ydelta, xdelta, item, tp)
int ydelta, xdelta;
register struct linked_list *item;
register struct thing *tp;
{
        register struct object *obj;
        struct thing midpoint;
        coord oldpos;

        obj = OBJPTR(item);
        oldpos = obj->o_pos;

        /*
         * make it appear to fly at the target
         */
        do_motion(obj, ydelta, xdelta, tp);
        hit_monster(unc(obj->o_pos), obj, tp);

        /*
         * Now let's make it fly back to the wielder.  We need to
         * use midpoint to fool do_motion into thinking the action
         * starts there.  Do_motion only looks at the t_pos field.
         */
        midpoint.t_pos = obj->o_pos;    /* Simulate a new start position */
        do_motion(obj, -ydelta, -xdelta, &midpoint);

        obj->o_pos = oldpos;
}

/*
 * do the actual motion on the screen done by an object traveling
 * across the room.  Note that we should not look at any field in
 * tp other than t_pos unless we change boomerang().
 */

do_motion(obj, ydelta, xdelta, tp)
register struct object *obj;
register int ydelta, xdelta;
register struct thing *tp;
{

    /*
    * Come fly with us ...
    */
    obj->o_pos = tp->t_pos;
    for (;;) {
        register int ch;
        /*
        * Erase the old one
        */
        if (!ce(obj->o_pos, tp->t_pos) &&
            cansee(unc(obj->o_pos)) &&
            mvwinch(cw, obj->o_pos.y, obj->o_pos.x) != ' ') {
                mvwaddch(cw, obj->o_pos.y, obj->o_pos.x, show(obj->o_pos.y, obj->o_pos.x));
        }
        /*
        * Get the new position
        */
        obj->o_pos.y += ydelta;
        obj->o_pos.x += xdelta;
        if (shoot_ok(ch = winat(obj->o_pos.y, obj->o_pos.x)) && ch != DOOR && !ce(obj->o_pos, hero)) {
                /*
                * It hasn't hit anything yet, so display it
                * If it alright.
                */
                if (cansee(unc(obj->o_pos)) &&
                    mvwinch(cw, obj->o_pos.y, obj->o_pos.x) != ' ') {
                        if (obj->o_type == MISSILE) nofont(cw);
                        mvwaddch(cw, obj->o_pos.y, obj->o_pos.x, obj->o_type);
                        newfont(cw);
                        draw(cw);
                }
                continue;
        }

        /*
         * Did we stop because of a monster or the hero?  If we did 
         * not, we want to move our position back one because we could
         * not actually make it this far.
         */
        if (!isalpha(ch) && 
            !(obj->o_pos.y == hero.y && obj->o_pos.x == hero.x)) {
                obj->o_pos.y -= ydelta;
                obj->o_pos.x -= xdelta;
        }

        break;
    }
}


/*
 * fall:
 *      Drop an item someplace around here.
 */

fall(item, pr)
register struct linked_list *item;
bool pr;
{
        register struct object *obj;
        register struct room *rp;
        register int i;
        struct object *tobj;
        struct linked_list *titem;
        coord *fpos = NULL;

        obj = OBJPTR(item);
        /*
         * try to drop the item, look up to 3 squares away for now
         */
        for (i=1; i<4; i++) {
            if ((fpos = fallpos(&obj->o_pos, FALSE, i)) != NULL)
                break;
        }

        if (fpos != NULL) {
                if (obj->o_group) { /* try to add groups together */
                    for(titem=lvl_obj; titem!=NULL; titem=next(titem)) {
                        tobj = OBJPTR(titem);
                        if (tobj->o_group == obj->o_group       &&
                            tobj->o_pos.y == fpos->y            &&
                            tobj->o_pos.x == fpos->x) {
                                tobj->o_count += obj->o_count;
                                o_discard(item);
                                return;
                        }
                    }
                }
                mvaddch(fpos->y, fpos->x, obj->o_type);
                obj->o_pos = *fpos;
                if ((rp = roomin(&hero)) != NULL &&
                    lit_room(rp)) {
                        light(&hero);
                        mvwaddch(cw, hero.y, hero.x, PLAYER);
                }
                attach(lvl_obj, item);
                return;
        }
        if (pr) {
            msg("The %s vanishes as it hits the ground.",
                weaps[obj->o_which].w_name);            
        }
        o_discard(item);
}

/*
 * Does the missile hit the monster
 */

hit_monster(y, x, obj, tp)
register int y, x;
struct object *obj;
register struct thing *tp;
{
        static coord mp;

        mp.y = y;
        mp.x = x;
        if (tp == &player) {
                /* Make sure there is a monster where it landed */
                if (!isalpha(mvwinch(mw, y, x))) {
                        return(FALSE);
                }

                /* Player hits monster */
                return(fight(&mp, obj, TRUE));
        } else {
                if (!ce(mp, hero)) {
                    /* Monster hits monster */
                    return(skirmish(tp, &mp, obj, TRUE));
                }

                /* Monster hits player */
                return(attack(tp, obj, TRUE));
        }
}

/*
 * init_weapon:
 *      Set up the initial goodies for a weapon
 */

init_weapon(weap, type)
register struct object *weap;
char type;
{
        register struct init_weps *iwp;

        iwp = &weaps[type];
        strcpy(weap->o_damage,iwp->w_dam);
        strcpy(weap->o_hurldmg,iwp->w_hrl);
        weap->o_launch = iwp->w_launch;
        weap->o_flags = iwp->w_flags;
        weap->o_weight = iwp->w_wght;
        if (weap->o_flags & ISMANY) {
                weap->o_count = rnd(8) + 8;
                weap->o_group = newgrp();
        } else {
                weap->o_count = 1;
        }
}

/*
 * missile:
 *      Fire a missile in a given direction
 */

missile(ydelta, xdelta, item, tp)
int ydelta, xdelta;
register struct linked_list *item;
register struct thing *tp;
{
        register struct object *obj;
        register struct linked_list *nitem;
        char ch;

        /*
        * Get which thing we are hurling
        */
        if (item == NULL) {
                return;
        }
        obj = OBJPTR(item);
        if (obj->o_type == RELIC && obj->o_which == AXE_AKLAD) {
            boomerang(ydelta, xdelta, item, tp);
            return;
        }

        if (!dropcheck(obj)) return;    /* Can we get rid of it? */

        if(!(obj->o_flags & ISMISL)) {
            for(;;) {
                msg(terse ? "Really throw? (y or n): "
                          : "Do you really want to throw %s? (y or n): ",
                                inv_name(obj, TRUE));
                mpos = 0;
                ch = wgetch(cw);
                if (ch == 'n' || ch == ESC) {
                    after = FALSE;
                    return;
                }
                if (ch == 'y')
                    break;
            }
        }
        /*
         * Get rid of the thing. If it is a non-multiple item object, or
         * if it is the last thing, just drop it. Otherwise, create a new
         * item with a count of one.
         */
        if (obj->o_count < 2) {
                detach(tp->t_pack, item);
                if (tp->t_pack == pack) {
                        inpack--;
                }
        } 
        else {
                obj->o_count--;
                nitem = (struct linked_list *) new_item(sizeof *obj);
                obj = OBJPTR(nitem);
                *obj = *(OBJPTR(item));
                obj->o_count = 1;
                item = nitem;
        }
        updpack(FALSE, tp);
        do_motion(obj, ydelta, xdelta, tp);
        /*
        * AHA! Here it has hit something. If it is a wall or a door,
        * or if it misses (combat) the monster, put it on the floor
        */
        if (!hit_monster(unc(obj->o_pos), obj, tp)) {
                fall(item, TRUE);
        }
        else
            o_discard(item);

        mvwaddch(cw, hero.y, hero.x, PLAYER);
        
}

/*
 * num:
 *      Figure out the plus number for armor/weapons
 */

char *
num(n1, n2)
register int n1, n2;
{
        static char numbuf[LINELEN/2];

        if (n1 == 0 && n2 == 0) {
                return "+0";
        }
        if (n2 == 0) {
                sprintf(numbuf, "%s%d", n1 < 0 ? "" : "+", n1);
        } else {
                sprintf(numbuf, "%s%d, %s%d", n1 < 0 ? "" : "+", n1, n2 < 0 ? "" : "+", n2);
        }
        return(numbuf);
}

/*
 * wield:
 *      Pull out a certain weapon
 */

wield()
{
        register struct linked_list *item;
        register struct object *obj, *oweapon;

        /*
         * It takes 2 movement periods to unwield a weapon and 2 movement
         * periods to wield a weapon.
         */
        if (player.t_action != C_WIELD) {
            player.t_action = C_WIELD;
            player.t_using = NULL;      /* Make sure this is NULL! */
            if (cur_weapon != NULL) {
                player.t_no_move = 2 * movement(&player);
                return;
            }
        }

        if ((oweapon = cur_weapon) != NULL) {
            /* At this point we have waited at least 2 units */
            if (!dropcheck(cur_weapon)) {
                    cur_weapon = oweapon;
                    player.t_action = A_NIL;
                    return;
            }
            if (terse)
                addmsg("Was ");
            else
                addmsg("You were ");
            msg("wielding %s", inv_name(oweapon, TRUE));
        }

        /* We we have something picked out? */
        if (player.t_using == NULL) {
            /* Now, what does he want to wield? */
            if ((item = get_item(pack, "wield", WIELDABLE, FALSE, FALSE)) == NULL) {
                    player.t_action = A_NIL;
                    after = FALSE;
                    return;
            }
            player.t_using = item;
            player.t_no_move = 2 * movement(&player);
            return;
        }

        /* We have waited our time, let's wield the weapon */
        item = player.t_using;
        player.t_using = NULL;
        player.t_action = A_NIL;

        obj = OBJPTR(item);

        if (is_current(obj)) {
                msg("Item in use.");
                after = FALSE;
                return;
        }
        if (player.t_ctype != C_FIGHTER && 
            player.t_ctype != C_RANGER  &&
            player.t_ctype != C_PALADIN &&
            obj->o_type == WEAPON       &&
            obj->o_which == TWOSWORD) {
                switch (rnd(3)) {
                case 0:
                   msg("Only fighter types can wield the two-handed sword.");
                when 1:
                   msg("Your hand does not fit the two-handed sword.");
                otherwise:
                   msg("You can not wield the two-handed sword.");
                }
                return;
        }
        if (player.t_ctype != C_FIGHTER && 
            player.t_ctype != C_ASSASSIN &&
            player.t_ctype != C_THIEF   &&
            player.t_ctype != C_MONK    &&
            obj->o_type == WEAPON       &&
            obj->o_which == BASWORD) {
                switch (rnd(3)) {
                case 0:
                   msg("Only thief types can wield the bastard sword.");
                when 1:
                   msg("Your hand does not fit the bastard sword.");
                otherwise:
                   msg("You can not wield the bastard sword.");
                }
                return;
        }
           
        if (terse) {
                addmsg("W");
        } else {
                addmsg("You are now w");
        }
        msg("ielding %s", inv_name(obj, TRUE));
        cur_weapon = obj;
}

