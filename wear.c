/*
    wear.c  -  functions for dealing with armor
    
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
 * take_off:
 *      Get the armor off of the players back
 */

take_off()
{
    register struct object *obj;
    register struct linked_list *item;

    /* It takes time to take things off */
    if (player.t_action != C_TAKEOFF) {
        /* What does player want to take off? */
        if ((item = get_item(pack, "take off", REMOVABLE, FALSE, FALSE))==NULL)
            return;

        obj = OBJPTR(item);
        if (!is_current(obj)) {
            msg("Not wearing %c) %s", pack_char(pack, obj),inv_name(obj, TRUE));
            return;
        }

        player.t_using = item;        /* Remember what it is */
        player.t_action = C_TAKEOFF;  /* We are taking something off */

        /* Cursed items take almost no time */
        if (obj->o_flags & ISCURSED) player.t_no_move = movement(&player);
        else player.t_no_move = dress_units(item) * movement(&player);
        return;
    }

    /* We have waited our time, let's take off our item */
    item = player.t_using;
    player.t_using = NULL;
    player.t_action = A_NIL;

    obj = OBJPTR(item);
    if (!is_current(obj)) {     /* Just to be on the safe side */
        msg("Not wearing %c) %s", pack_char(pack, obj),inv_name(obj, TRUE));
        return;
    }

    /* Can the player remove the item? */
    if (!dropcheck(obj)) return;
    updpack(TRUE, &player);

    msg("Was wearing %c) %s", pack_char(pack, obj),inv_name(obj,TRUE));
}

/*
 * wear:
 *      The player wants to wear something, so let him/her put it on.
 */

wear()
{
    register struct linked_list *item;
    register struct object *obj;
    register int i;

    /* It takes time to put things on */
    if (player.t_action != C_WEAR) {
        /* What does player want to wear? */
        if ((item = get_item(pack, "wear", WEARABLE, FALSE, FALSE)) == NULL)
            return;

        obj = OBJPTR(item);

        switch (obj->o_type) {
            case ARMOR:
                if (cur_armor != NULL) {
                    addmsg("You are already wearing armor");
                    if (!terse) addmsg(".  You'll have to take it off first.");
                    endmsg();
                    after = FALSE;
                    return;
                }
                if (player.t_ctype == C_MONK) {
                    msg("Monks can't wear armor!");
                    return;
                }
                if (cur_misc[WEAR_BRACERS] != NULL) {
                    msg("You can't wear armor with bracers of defense.");
                    return;
                }
                if (cur_misc[WEAR_CLOAK] != NULL || cur_relic[EMORI_CLOAK]) {
                    msg("You can't wear armor with a cloak.");
                    return;
                }
                if (player.t_ctype == C_THIEF   &&
                    (obj->o_which != LEATHER    &&
                     obj->o_which != STUDDED_LEATHER)) {
                    if (terse) msg("Thieves can't wear that type of armor.");
                    else
                 msg("Thieves can wear leather and studded leather armor.");
                    return;
                }
                if (player.t_ctype == C_ASSASSIN &&
                    (obj->o_which != LEATHER    &&
                     obj->o_which != STUDDED_LEATHER)) {
                    if (terse) msg("Assassins can't wear that type of armor.");
                    else
                 msg("Assassins can wear leather and studded leather armor.");
                    return;
                }

            when MM:
                switch (obj->o_which) {
                /*
                 * when wearing the boots of elvenkind the player will not
                 * set off any traps
                 */
                case MM_ELF_BOOTS:
                    if (cur_misc[WEAR_BOOTS] != NULL) {
                        msg("Already wearing a pair of boots. ");
                        return;
                    }
                /*
                 * when wearing the boots of dancing the player will dance
                 * uncontrollably
                 */
                when MM_DANCE:
                    if (cur_misc[WEAR_BOOTS] != NULL) {
                        msg("Already wearing a pair of boots.");
                        return;
                    }
                /*
                 * bracers give the hero protection in he same way armor does.
                 * they cannot be used with armor but can be used with cloaks
                 */
                when MM_BRACERS:
                    if (cur_misc[WEAR_BRACERS] != NULL) {
                        msg("Already wearing bracers.");
                        return;
                    }
                    else {
                        if (cur_armor != NULL) {
                           msg("You can't wear bracers of defense with armor.");
                           return;
                        }
                    }

                /*
                 * The robe (cloak) of powerlessness disallows any spell casting
                 */
                when MM_R_POWERLESS:
                /*
                 * the cloak of displacement gives the hero an extra +2 on AC
                 * and saving throws. Cloaks cannot be used with armor.
                 */
                case MM_DISP:
                /*
                 * the cloak of protection gives the hero +n on AC and saving
                 * throws with a max of +3 on saves
                 */
                case MM_PROTECT:
                    if (cur_misc[WEAR_CLOAK] != NULL ||
                        cur_relic[EMORI_CLOAK]) {
                        msg("%slready wearing a cloak.", terse ? "A"
                                                               : "You are a");
                        return;
                    }
                    else {
                        if (cur_armor != NULL) {
                            msg("You can't wear a cloak with armor.");
                            return;
                        }
                    }
                /*
                 * the gauntlets of dexterity and ogre power give the hero
                 * a dexterity of 21, the gauntlets of fumbling cause the
                 * hero to drop his weapon.
                 */
                when MM_G_DEXTERITY:
                case MM_G_OGRE:
                case MM_FUMBLE:
                    if (cur_misc[WEAR_GAUNTLET] != NULL) {
                        msg("Already wearing a pair of gauntlets.");
                        return;
                    }
                /*
                 * the jewel of attacks does an aggavate monster
                 */
                when MM_JEWEL:
                    if (cur_misc[WEAR_JEWEL] != NULL    ||
                        cur_relic[YENDOR_AMULET]        ||
                        cur_relic[STONEBONES_AMULET]) {
                        msg("Already wearing an amulet.");
                        return;
                    }
                /*
                 * the necklace of adaption makes the hero immune to
                 * chlorine gas and acid breath.
                 */
                when MM_ADAPTION:
                    if (cur_misc[WEAR_NECKLACE] != NULL) {
                        msg("Already wearing a necklace.");
                        return;
                    }
                /*
                 * the necklace of stragulation will try to strangle the
                 * hero to death
                 */
                when MM_STRANGLE:
                    if (cur_misc[WEAR_NECKLACE] != NULL) {
                        msg("Already wearing a necklace.");
                        return;
                    }
                otherwise:
                    msg("What a strange item you have!");
                    return;
                }

            when RING:
                if (cur_misc[WEAR_GAUNTLET] != NULL) {
                    msg ("You have to remove your gauntlets first!");
                    return;
                }

                /* Is there room to put the ring on */
                for (i=0; i<NUM_FINGERS; i++)
                    if (cur_ring[i] == NULL) {
                        break;
                    }
                if (i == NUM_FINGERS) { /* Not enough fingers */
                    if (terse) msg("Wearing enough rings.");
                    else msg("You are already wearing eight rings.");
                    return;
                }
        }

        player.t_using = item;      /* Remember what it is */
        player.t_action = C_WEAR;   /* We are taking something off */
        player.t_no_move = dress_units(item) * movement(&player);
        return;
    }

    /* We have waited our time, let's put on our item */
    item = player.t_using;
    player.t_using = NULL;
    player.t_action = A_NIL;

    obj = OBJPTR(item);

    switch (obj->o_type) {
        case ARMOR:
            obj->o_flags |= ISKNOW;
            cur_armor = obj;
            addmsg(terse ? "W" : "You are now w");
            msg("earing %s.", armors[obj->o_which].a_name);

        when MM:
            switch (obj->o_which) {
            /*
             * when wearing the boots of elvenkind the player will not
             * set off any traps
             */
            case MM_ELF_BOOTS:
                msg("Wearing %s",inv_name(obj,TRUE));
                cur_misc[WEAR_BOOTS] = obj;
            /*
             * when wearing the boots of dancing the player will dance
             * uncontrollably
             */
            when MM_DANCE:
                msg("Wearing %s",inv_name(obj,TRUE));
                cur_misc[WEAR_BOOTS] = obj;
                msg("You begin to dance uncontrollably!");
                turn_on(player, ISDANCE);
            /*
             * bracers give the hero protection in he same way armor does.
             * they cannot be used with armor but can be used with cloaks
             */
            when MM_BRACERS:
                msg("wearing %s",inv_name(obj,TRUE));
                cur_misc[WEAR_BRACERS] = obj;

            /*
             * The robe (cloak) of powerlessness disallows any spell casting
             */
            when MM_R_POWERLESS:
            /*
             * the cloak of displacement gives the hero an extra +2 on AC
             * and saving throws. Cloaks cannot be used with armor.
             */
            case MM_DISP:
            /*
             * the cloak of protection gives the hero +n on AC and saving
             * throws with a max of +3 on saves
             */
            case MM_PROTECT:
                msg("wearing %s",inv_name(obj,TRUE));
                cur_misc[WEAR_CLOAK] = obj;
            /*
             * the gauntlets of dexterity and ogre power give the hero
             * a dexterity of 21, the gauntlets of fumbling cause the
             * hero to drop his weapon.
             */
            when MM_G_DEXTERITY:
            case MM_G_OGRE:
            case MM_FUMBLE:
                msg("Wearing %s", inv_name(obj,TRUE));
                cur_misc[WEAR_GAUNTLET] = obj;
                if (obj->o_which == MM_FUMBLE)
                    daemon(fumble, (VOID *)NULL, AFTER);
            /*
             * the jewel of attacks does an aggavate monster
             */
            when MM_JEWEL:
                msg("Wearing %s",inv_name(obj,TRUE));
                cur_misc[WEAR_JEWEL] = obj;
                aggravate(TRUE, TRUE); /* affect all charactors */
                if (player.t_ctype == C_PALADIN ||
                    player.t_ctype == C_RANGER  || player.t_ctype == C_MONK)
                        msg("A chill runs down your spine! ");

            /*
             * the necklace of adaption makes the hero immune to
             * chlorine gas and acid
             */
            when MM_ADAPTION:
                msg("Wearing %s",inv_name(obj,TRUE));
                cur_misc[WEAR_NECKLACE] = obj;
                turn_on(player, NOGAS);
                turn_on(player, NOACID);

            /*
             * the necklace of stragulation will try to strangle the
             * hero to death
             */
            when MM_STRANGLE:
                msg("Wearing %s",inv_name(obj,TRUE));
                cur_misc[WEAR_NECKLACE] = obj;
                msg("The necklace is beginning to strangle you!");
                daemon(strangle, (VOID *)NULL, AFTER);
            otherwise:
                msg("What a strange item you have!");
            }
            status(FALSE);
            if (m_know[obj->o_which] && m_guess[obj->o_which]) {
                free(m_guess[obj->o_which]);
                m_guess[obj->o_which] = NULL;
            }
            else if (!m_know[obj->o_which] && 
                     askme &&
                     (obj->o_flags & ISKNOW) == 0  &&
                     m_guess[obj->o_which] == NULL) {
                nameitem(item, FALSE);
            }

        when RING:
            /* If there is room, put on the ring */
            for (i=0; i<NUM_FINGERS; i++)
                if (cur_ring[i] == NULL) {
                    cur_ring[i] = obj;
                    break;
                }
            if (i == NUM_FINGERS) {     /* Not enough fingers */
                if (terse) msg("Wearing enough rings.");
                else msg("You are already wearing eight rings.");
                return;
            }

            /* Calculate the effect of the ring */
            ring_on(item);
    }
    updpack(TRUE, &player);
}

/*
 * dress_units:
 *      How many movements periods does it take to put on or remove the
 *      given item of "clothing"?
 */

dress_units(item)
struct linked_list *item;
{
    register struct object *obj;

    obj = OBJPTR(item);

    switch (obj->o_type) {
        case ARMOR:
            return(10-armors[obj->o_which].a_class);
        when RING:
            return(2);
        when MM:
            switch (obj->o_which) {
                case MM_ELF_BOOTS:
                case MM_DANCE:
                    /* Boots */
                    return(5);
                when MM_R_POWERLESS:
                case MM_DISP:
                case MM_PROTECT:
                    /* Robes */
                    return(4);
                when MM_BRACERS:
                case MM_G_DEXTERITY:
                case MM_G_OGRE:
                case MM_FUMBLE:
                    /* Hand garments */
                    return(3);
                when MM_JEWEL:
                case MM_ADAPTION:
                case MM_STRANGLE:
                    /* Jewelry */
                    return(2);
                otherwise:
                    return(1);  /* What is it? */
        }
        otherwise:
            return(1);  /* What is it? */
    }
}

