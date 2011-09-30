/*
    sticks.c - Functions to implement the various sticks one might find
    
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
 * zap a stick and see what happens
 */

do_zap(zapper, obj, direction, which, flags)
struct thing *zapper;
struct object *obj;
coord *direction;
int which;
int flags;
{
    register struct linked_list *item = NULL;
    register struct thing *tp;
    register int y = 0, x = 0, bonus;
    struct linked_list *nitem;
    struct object *nobj;
    bool cursed, blessed, is_player = FALSE;
    char *mname = NULL;

    cursed = flags & ISCURSED;
    blessed = flags & ISBLESSED;

    if (obj && obj->o_type != RELIC) { /* all relics are chargeless */
        if (obj->o_charges < 1) {
            msg(nothing);
            return;
        }
        obj->o_charges--;
    }
    if (which == WS_WONDER) {
        switch (rnd(19)) {
            case  0: which = WS_ELECT;
            when  1: which = WS_FIRE;
            when  2: which = WS_COLD;
            when  3: which = WS_POLYMORPH;
            when  4: which = WS_MISSILE;
            when  5: which = WS_SLOW_M;
            when  6: which = WS_TELMON;
            when  7: which = WS_CANCEL;
            when  8: which = WS_CONFMON;
            when  9: which = WS_DISINTEGRATE;
            when 10: which = WS_PETRIFY;
            when 11: which = WS_PARALYZE;
            when 12: which = WS_MDEG;
            when 13: which = WS_FEAR;
            when 14: which = WS_CURING;
            when 15: which = WS_LIGHT;
            when 16: which = WS_HIT;
            when 17: which = WS_DRAIN;
            when 18: which = WS_CHARGE;
        }
        if(ws_magic[which].mi_curse>0 && rnd(100)<=ws_magic[which].mi_curse){
            cursed = TRUE;
            blessed = FALSE;
        }
    }

    tp = NULL;
    switch (which) {
        case WS_POLYMORPH:
        case WS_SLOW_M:
        case WS_TELMON:
        case WS_CANCEL:
        case WS_CONFMON:
        case WS_DISINTEGRATE:
        case WS_PETRIFY:
        case WS_PARALYZE:
        case WS_MDEG:
        case WS_FEAR:
            y = zapper->t_pos.y;
            x = zapper->t_pos.x;

            do {
                y += direction->y;
                x += direction->x;
            }
            while (shoot_ok(winat(y, x)) && !(y == hero.y && x == hero.x));

            if (y == hero.y && x == hero.x)
                is_player = TRUE;
            else if (isalpha(mvwinch(mw, y, x))) {
                item = find_mons(y, x);
                tp = THINGPTR(item);
                runto(tp, &hero);
                turn_off(*tp, CANSURPRISE);
                mname = monster_name(tp);
                is_player = FALSE;

                /* The monster may not like being shot at */
                if ((zapper == &player) &&
                    on(*tp, ISCHARMED)  &&
                    save(VS_MAGIC, tp, 0)) {
                    msg("The eyes of %s turn clear.", prname(mname, FALSE));
                    turn_off(*tp, ISCHARMED);
                    mname = monster_name(tp);
                }
            }
            else {
                /*
                 * if monster misses player because the player dodged then lessen
                 * the chances he will use the wand again since the player appears
                 * to be rather dextrous
                 */
                if (zapper != &player) 
                    zapper->t_wand = zapper->t_wand * 3 / 5;
            }
    }
    switch (which) {
        case WS_LIGHT:
            /*
             * Reddy Kilowat wand.  Light up the room
             */
            blue_light(blessed, cursed);
        when WS_DRAIN:
            /*
             * Take away 1/2 of hero's hit points, then take it away
             * evenly from the monsters in the room or next to hero
             * if he is in a passage (but leave the monsters alone
             * if the stick is cursed)
             */
            if (pstats.s_hpt < 2) {
                msg("You are too weak to use it.");
            }
            else if (cursed)
                pstats.s_hpt /= 2;
        if (pstats.s_hpt <= 0) {
            pstats.s_hpt = -1;
            msg("You drain your own life away.  --More--");
            death(D_STRENGTH);
        }
            else
                drain(hero.y-1, hero.y+1, hero.x-1, hero.x+1);

        when WS_POLYMORPH:
        {
            register char oldch;
            register struct room *rp;
            register struct linked_list *pitem;
            coord delta;

            if (tp == NULL)
                break;
            if (save(VS_MAGIC, tp, 0)) {
                msg(nothing);
                break;
            }
            rp = roomin(&tp->t_pos);
            check_residue(tp);
            delta.x = x;
            delta.y = y;
            detach(mlist, item);
            oldch = tp->t_oldch;
            pitem = tp->t_pack; /* save his pack */
            tp->t_pack = NULL;

            if (levtype == OUTSIDE) 
                new_monster(item,rnd(NUMDINOS)+NUMMONST-NUMDINOS,&delta,FALSE);
            else
                new_monster(item,rnd(NUMMONST-NUMUNIQUE-NUMDINOS-1)+1,&delta,FALSE);

            if (tp->t_pack != NULL) 
                o_free_list (tp->t_pack);
            tp->t_pack = pitem;
            if (isalpha(mvwinch(cw, y, x)))
                mvwaddch(cw, y, x, tp->t_type);
            tp->t_oldch = oldch;
            /*
             * should the room light up?
             */
            if (on(*tp, HASFIRE)) {
                if (rp) {
                    register struct linked_list *fire_item;

                    fire_item = creat_item();
                    ldata(fire_item) = (char *) tp;
                    attach(rp->r_fires, fire_item);
                    rp->r_flags |= HASFIRE;
                    if (cansee(tp->t_pos.y,tp->t_pos.x) &&
                        next(rp->r_fires) == NULL) light(&hero);
                }
            }
            runto(tp, &hero);
            msg(terse ? "A new %s!" 
                      : "You have created a new %s!",
                      monster_name(tp));
        }

        when WS_PETRIFY:
            if (tp == NULL)
                break;
            if (save(VS_MAGIC, tp, 0)) {
                msg(nothing);
                break;
            }
            check_residue(tp);
            turn_on(*tp, ISSTONE);
            turn_on(*tp, NOSTONE);
            turn_off(*tp, ISRUN);
            turn_off(*tp, ISINVIS);
            turn_off(*tp, CANSURPRISE);
            turn_off(*tp, ISDISGUISE);
            tp->t_action = A_NIL;
            tp->t_no_move = 0;
            msg("%s is turned to stone!",prname(mname, TRUE));

        when WS_TELMON:
        {
            register int rm;
            register struct room *rp;

            if (tp == NULL)
                break;
            if (save(VS_MAGIC, tp, 0)) {
                msg(nothing);
                break;
            }
            rp = NULL;
            check_residue(tp);
            tp->t_action = A_FREEZE; /* creature is disoriented */
            tp->t_no_move = 2;
            if (cursed) {       /* Teleport monster to player */
                if ((y == (hero.y + direction->y)) &&
                    (x == (hero.x + direction->x)))
                        msg(nothing);
                else {
                    tp->t_pos.y = hero.y + direction->y;
                    tp->t_pos.x = hero.x + direction->x;
                }
            }
            else if (blessed) { /* Get rid of monster */
                killed(item, FALSE, TRUE, TRUE);
                return;
            }
            else {
                register int i=0;

                do {    /* Move monster to another room */
                    rm = rnd_room();
                    rnd_pos(&rooms[rm], &tp->t_pos);
                }until(winat(tp->t_pos.y,tp->t_pos.x)==FLOOR ||i++>500);
                rp = &rooms[rm];
            }

            /* Now move the monster */
            if (isalpha(mvwinch(cw, y, x)))
                mvwaddch(cw, y, x, tp->t_oldch);
            mvwaddch(mw, y, x, ' ');
            mvwaddch(mw, tp->t_pos.y, tp->t_pos.x, tp->t_type);
            if (tp->t_pos.y != y || tp->t_pos.x != x)
                tp->t_oldch = mvwinch(cw, tp->t_pos.y, tp->t_pos.x);
            /*
             * check to see if room that creature appears in should
             * light up
             */
            if (on(*tp, HASFIRE)) {
                if (rp) {
                    register struct linked_list *fire_item;

                    fire_item = creat_item();
                    ldata(fire_item) = (char *) tp;
                    attach(rp->r_fires, fire_item);
                    rp->r_flags |= HASFIRE;
                    if(cansee(tp->t_pos.y, tp->t_pos.x) && 
                       next(rp->r_fires) == NULL)
                        light(&hero);
                }
            }
        }
        when WS_CANCEL:
            if (tp == NULL)
                break;
            if (save(VS_MAGIC, tp, 0)) {
                msg(nothing);
                break;
            }
            check_residue(tp);
            tp->t_flags[0] &= CANC0MASK;
            tp->t_flags[1] &= CANC1MASK;
            tp->t_flags[2] &= CANC2MASK;
            tp->t_flags[3] &= CANC3MASK;
            tp->t_flags[4] &= CANC4MASK;
            tp->t_flags[5] &= CANC5MASK;
            tp->t_flags[6] &= CANC6MASK;
            tp->t_flags[7] &= CANC7MASK;
            tp->t_flags[8] &= CANC8MASK;
            tp->t_flags[9] &= CANC9MASK;
            tp->t_flags[10] &= CANCAMASK;
            tp->t_flags[11] &= CANCBMASK;
            tp->t_flags[12] &= CANCCMASK;
            tp->t_flags[13] &= CANCDMASK;
            tp->t_flags[14] &= CANCEMASK;
            tp->t_flags[15] &= CANCFMASK;

        when WS_MISSILE:
        {
            int dice;
            static struct object bolt =
            {
                MISSILE , {0, 0}, 0, "", "1d4 " , NULL, 0, WS_MISSILE, 50, 1
            };

            if (!obj)
                dice = zapper->t_stats.s_lvl;
            if (obj->o_type == RELIC)
                 dice = 15;
            else if (EQUAL(ws_type[which], "staff"))
                 dice = 10;
            else
                 dice = 6;
            sprintf(bolt.o_hurldmg, "%dd4", dice);
            do_motion(&bolt, direction->y, direction->x, zapper);
            if (!hit_monster(unc(bolt.o_pos), &bolt, zapper))
               msg("The missile vanishes with a puff of smoke");
        }
        when WS_HIT:
        {
            register unsigned char ch;
            struct object strike; /* don't want to change sticks attributes */

            direction->y += hero.y;
            direction->x += hero.x;
            ch = winat(direction->y, direction->x);
            if (isalpha(ch))
            {
                strike = *obj;
                strike.o_hplus  = 7;
                if (EQUAL(ws_type[which], "staff"))
                    strcpy(strike.o_damage,"3d8");
                else
                    strcpy(strike.o_damage,"2d8");
                fight(direction, &strike, FALSE);
            }
        }
        when WS_SLOW_M:
            if (is_player) {
                add_slow();
                break;
            }
            if (tp == NULL)
                break;
            if (cursed) {
                if (on(*tp, ISSLOW))
                    turn_off(*tp, ISSLOW);
                else
                    turn_on(*tp, ISHASTE);
                break;
            }
            if ((on(*tp,ISUNIQUE) && save(VS_MAGIC,tp,0)) || on(*tp,NOSLOW)) {
                msg(nothing);
                break;
            }
            else if (blessed) {
                turn_off(*tp, ISRUN);
                turn_on(*tp, ISHELD);
            }
            /*
             * always slow in case he breaks free of HOLD
             */
            if (on(*tp, ISHASTE))
                turn_off(*tp, ISHASTE);
            else
                turn_on(*tp, ISSLOW);

        when WS_CHARGE:
            if (ws_know[WS_CHARGE] != TRUE && obj)
                msg("This is a wand of charging.");
            nitem = get_item(pack, "charge", STICK, FALSE, FALSE);
            if (nitem != NULL) {
                nobj = OBJPTR(nitem);
                if ((++(nobj->o_charges) == 1) && (nobj->o_which == WS_HIT))
                    fix_stick(nobj);
                if (blessed) ++(nobj->o_charges);
                if (EQUAL(ws_type[nobj->o_which], "staff")) {
                    if (nobj->o_charges > 200) 
                        nobj->o_charges = 200;
                }
                else {
                    if (nobj->o_charges > 200)
                        nobj->o_charges = 200;
                }
            }
        when WS_ELECT:
            shoot_bolt( zapper, zapper->t_pos, *direction, TRUE, D_BOLT, 
                        "lightning bolt", roll(zapper->t_stats.s_lvl,6));

        when WS_FIRE:
            shoot_bolt( zapper, zapper->t_pos, *direction, TRUE, D_BOLT, 
                        "flame", roll(zapper->t_stats.s_lvl,6));

        when WS_COLD:
            shoot_bolt( zapper, zapper->t_pos, *direction, TRUE, D_BOLT, 
                        "ice", roll(zapper->t_stats.s_lvl,6));

        when WS_CONFMON:
            if (cursed || is_player) { 
                if (!save(VS_WAND, &player, 0)) {
                    dsrpt_player();
                    confus_player();
                }
                else {
                    if (zapper != &player) zapper->t_wand /= 2;
                    msg(nothing);
                }
            }
            else {
                if (tp == NULL)
                    break;
                if (save(VS_MAGIC, tp, 0) || on(*tp, ISCLEAR))
                     msg(nothing);
                else
                     turn_on (*tp, ISHUH);
            }
        when WS_PARALYZE:
            if (is_player || cursed) {
                if ((obj && obj->o_type==RELIC) || !save(VS_WAND, &player, 0)){
                    player.t_no_move += 2 * movement(&player) * FREEZETIME;
                    player.t_action = A_FREEZE;
                    msg("You can't move.");
                }
                else {
                    if (zapper != &player) zapper->t_wand /= 2;
                    msg(nothing);
                }
            }
            else {
                if (tp == NULL)
                    break;
                bonus = 0;
                if (blessed) bonus = -3;
                if (((obj && obj->o_type==RELIC) || !save(VS_WAND,tp,bonus)) &&
                    off(*tp, NOPARALYZE)) {
                    tp->t_no_move += 2 * movement(tp) * FREEZETIME;
                    tp->t_action = A_FREEZE;
                }
                else {
                    msg(nothing);
                }
            }
        when WS_FEAR:
            if (is_player) {
                if (!on(player, ISFLEE)         || 
                    ISWEARING(R_HEROISM)        || 
                    save(VS_WAND, &player, 0)) {
                        msg(nothing);
                        zapper->t_wand /= 2;
                }
                else {
                    turn_on(player, ISFLEE);
                    player.t_dest = &zapper->t_pos;
                    msg("The sight of %s terrifies you.", prname(mname, FALSE));
                }
                break;
            }
            if (tp == NULL)
                break;
            bonus = 0;
            if (blessed) bonus = -3;
            if(save(VS_WAND, tp,bonus) || on(*tp,ISUNDEAD) || on(*tp,NOFEAR)){
                    msg(nothing);
                    break;
            }
            turn_on(*tp, ISFLEE);
            turn_on(*tp, WASTURNED);

            /* Stop it from attacking us */
            dsrpt_monster(tp, TRUE, cansee(tp->t_pos.y, tp->t_pos.x));

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

        when WS_MDEG:
            if (is_player) {
                if (save(VS_WAND, &player, 0)) {
                    msg (nothing);
                    zapper->t_wand /= 2;
                    break;
                }
                pstats.s_hpt /= 2;
                if (pstats.s_hpt <= 0) {
            pstats.s_hpt = -1;
                    msg("Your life has been sucked out from you!  --More--");
                    wait_for(' ');
                    death(zapper);
                }
                else
                    msg("You feel a great drain on your system.");
            }
            if (tp == NULL)
                break;
            if (cursed) {
                 tp->t_stats.s_hpt *= 2;
                 msg("%s appears to be stronger now!", prname(mname, TRUE));
            }
            else if (on(*tp, ISUNIQUE) && save(VS_WAND, tp, 0))
                 msg (nothing);
            else {
                 tp->t_stats.s_hpt /= 2;
                 msg("%s appears to be weaker now", prname(mname, TRUE));
            }
            if (tp->t_stats.s_hpt < 1)
                 killed(item, TRUE, TRUE, TRUE);
        when WS_DISINTEGRATE:
            if (tp == NULL)
                break;
            if (cursed) {
                register int m1, m2;
                coord mp;
                struct linked_list *titem;
                char ch;
                struct thing *th;

                if (on(*tp, ISUNIQUE) || on(*tp, CANSELL)) {
                    msg (nothing);
                    break;
                }
                for (m1=tp->t_pos.x-1 ; m1 <= tp->t_pos.x+1 ; m1++) {
                    for(m2=tp->t_pos.y-1 ; m2<=tp->t_pos.y+1 ; m2++) {
                        if (m1 == hero.x && m2 == hero.y)
                            continue;
                        ch = winat(m2,m1);
                        if (shoot_ok(ch)) {
                            mp.x = m1;  /* create it */
                            mp.y = m2;
                            titem = new_item(sizeof(struct thing));
                            new_monster(titem,(short)tp->t_index,&mp,FALSE);
                            th = THINGPTR(titem);
                            turn_on (*th, ISMEAN);
                            runto(th,&hero);
                            if (on(*th, HASFIRE)) {
                                register struct room *rp;

                                rp = roomin(&th->t_pos);
                                if (rp) {
                                    register struct linked_list *fire_item;

                                    fire_item = creat_item();
                                    ldata(fire_item) = (char *) th;
                                    attach(rp->r_fires, fire_item);
                                    rp->r_flags |= HASFIRE;
                                    if (cansee(th->t_pos.y, th->t_pos.x) &&
                                        next(rp->r_fires) == NULL)
                                        light(&hero);
                                }
                            }
                        }
                    }
                }
            }
            else { /* if its a UNIQUE it might still live */
                if (on(*tp, ISUNIQUE) && save(VS_MAGIC, tp, 0)) {
                    tp->t_stats.s_hpt /= 2;
                    if (tp->t_stats.s_hpt < 1) {
                         killed(item, FALSE, TRUE, TRUE);
                         msg("You have disintegrated %s", prname(mname, FALSE));
                    }
                    else {
                        msg("%s appears wounded", prname(mname, TRUE));
                    }
                }
                else {
                    msg("You have disintegrated %s", prname(mname, FALSE));
                    killed (item, FALSE, TRUE, TRUE);
                }
            }
        when WS_CURING:
            if (cursed) {
        bool sick = FALSE;
                if (!save(VS_POISON, &player, 0)) {
                    msg("You feel extremely sick. ");
            sick = TRUE;
                    pstats.s_hpt -= (pstats.s_hpt/3)+1;
                    if (pstats.s_hpt == 0)  {
            pstats.s_hpt = -1;
            msg("You die!  --More--");
            wait_for(' ');
            death (D_POISON);
            }
                }
                if (!save(VS_WAND, &player, 0) && !ISWEARING(R_HEALTH)) {
                    turn_on(player, HASDISEASE);
                    turn_on(player, HASINFEST);
                    turn_on(player, DOROT);
                    fuse(cure_disease, (VOID *)NULL, roll(HEALTIME,SICKTIME), AFTER);
                    infest_dam++;
                }
        else if (sick == FALSE) msg("You feel momentarily sick");
            }
            else {
                if (on(player, HASDISEASE) || on(player, HASINFEST)) {
                    extinguish(cure_disease);
                    turn_off(player, HASINFEST);
                    infest_dam = 0;
                    cure_disease(); /* this prints message */
                }
                if (on(player, DOROT)) {
                    msg("You feel your skin returning to normal.");
                    turn_off(player, DOROT);
                }
                pstats.s_hpt += roll(pstats.s_lvl, blessed ? 9 : 6);
                if (pstats.s_hpt > max_stats.s_hpt)
                    pstats.s_hpt = max_stats.s_hpt;
                msg("You begin to feel %sbetter.", blessed ? "much " : "");
                    
            }
        otherwise:
            msg("What a bizarre schtick!");
    }
}


/*
 * drain:
 *      Do drain hit points from player shtick
 */

drain(ymin, ymax, xmin, xmax)
int ymin, ymax, xmin, xmax;
{
    register int i, j, count;
    register struct thing *ick;
    register struct linked_list *item;

    /*
     * First count how many things we need to spread the hit points among
     */
    count = 0;
    for (i = ymin; i <= ymax; i++) {
        if (i < 1 || i > lines - 3)
            continue;
        for (j = xmin; j <= xmax; j++) {
            if (j < 0 || j > cols - 1)
                continue;
            if (isalpha(mvwinch(mw, i, j)))
                count++;
        }
    }
    if (count == 0)
    {
        msg("You have a tingling feeling.");
        return;
    }
    count = pstats.s_hpt / count;
    pstats.s_hpt /= 2;
    if (pstats.s_hpt <= 0) {
    pstats.s_hpt = -1;
    msg("Aarrgghhh!!  --More--");
    wait_for(' ');
    death(D_STRENGTH);
    }
    /*
     * Now zot all of the monsters
     */
    for (i = ymin; i <= ymax; i++) {
        if (i < 1 || i > lines - 3)
            continue;
        for (j = xmin; j <= xmax; j++) {
            if (j < 0 || j > cols - 1)
                continue;
            if (isalpha(mvwinch(mw, i, j)) &&
                ((item = find_mons(i, j)) != NULL)) {
                ick = THINGPTR(item);
                if (on(*ick, ISUNIQUE) && save(VS_MAGIC, ick, 0)) 
                    ick->t_stats.s_hpt -= count / 2;
                else
                    ick->t_stats.s_hpt -= count;
                if (ick->t_stats.s_hpt < 1)
                    killed(item, 
                           cansee(i,j)&&(!on(*ick,ISINVIS)||on(player,CANSEE)),
                           TRUE, TRUE);
                else {
                    runto(ick, &hero);

                    /* 
                     * The monster may not like being shot at.  Since the
                     * shot is not aimed directly at the monster, we will
                     * give him a poorer save.
                     */
                    if (on(*ick, ISCHARMED) && save(VS_MAGIC, ick, -2)) {
                        msg("The eyes of %s turn clear.",
                            prname(monster_name(ick), FALSE));
                        turn_off(*ick, ISCHARMED);
                    }
                    if (cansee(i,j) && (!on(*ick,ISINVIS)||on(player,CANSEE)))
                            msg("%s appears wounded", 
                                prname(monster_name(ick), TRUE));
                }
            }
        }
    }
}

/*
 * initialize a stick
 */

fix_stick(cur)
register struct object *cur;
{
    if (EQUAL(ws_type[cur->o_which], "staff")) {
        cur->o_weight = 100;
        cur->o_charges = 5 + rnd(11);
        strcpy(cur->o_damage,"3d4");
        cur->o_hplus = 1;
        cur->o_dplus = 0;
        switch (cur->o_which) {
            case WS_HIT:
                cur->o_hplus = 3;
                cur->o_dplus = 3;
                strcpy(cur->o_damage,"2d8");
            when WS_LIGHT:
                cur->o_charges = 15 + rnd(11);
            }
    }
    else {
        strcpy(cur->o_damage,"2d3");
        cur->o_weight = 75;
        cur->o_hplus = 1;
        cur->o_dplus = 0;
        cur->o_charges = 5 + rnd(11);
        switch (cur->o_which) {
            case WS_HIT:
                cur->o_hplus = 3;
                cur->o_dplus = 3;
                strcpy(cur->o_damage,"2d8");
            when WS_LIGHT:
                cur->o_charges = 15 + rnd(11);
            }
    }
    strcpy(cur->o_hurldmg,"3d3");

}

/*
 * Use the wand that our monster is wielding.
 */

m_use_wand(monster)
register struct thing *monster;
{
    register struct object *obj;

    /* Make sure we really have it */
    if (monster->t_using) 
        obj = OBJPTR(monster->t_using);
    else {
        debug("Stick not set!");
        monster->t_action = A_NIL;
        return;
    }

    if (obj->o_type != STICK) {
        debug("Stick not selected!");
        monster->t_action = A_NIL;
        return;
    }
    /*
     * shoot the stick!
     * assume all blessed sticks are normal for now. 
     * Note that we don't get here if the wand is cursed.
     */
    msg("%s points a %s at you!", prname(monster_name(monster), TRUE),
        ws_type[obj->o_which]);
    do_zap(monster, obj, &monster->t_newpos, obj->o_which, NULL);
    monster->t_wand /= 2; /* chance lowers with each use */
}

bool
need_dir(type, which)
int     type,           /* type of item, NULL means stick */
        which;          /* which item                     */
{
    if (type == STICK || type == 0) {
        switch (which) {
            case WS_LIGHT:
            case WS_DRAIN: 
            case WS_CHARGE:
            case WS_CURING:
                return(FALSE);
            default:
                return(TRUE);
        }
    }
    else if (type == RELIC) {
        switch (which) {
            case MING_STAFF:
            case ASMO_ROD:
            case EMORI_CLOAK:
                return(TRUE);
            default:
                return(FALSE);
        }
    }
return (FALSE); /* hope we don't get here */
}

/*
 * let the player zap a stick and see what happens
 */

player_zap(which, flag)
int which;
int flag;
{
    register struct linked_list *item;
    register struct object *obj;

    obj = NULL;
    if (which == 0) {
        /* This is a stick.  It takes 2 movement periods to zap it */
        if (player.t_action != C_ZAP) {
            if ((item = get_item(pack,"zap with",ZAPPABLE,FALSE,FALSE)) == NULL)
                return(FALSE);

            obj = OBJPTR(item);

            if (need_dir(obj->o_type, obj->o_which)) {
                if (!get_dir(&player.t_newpos))
                    return(FALSE);
            }
            player.t_using = item;      /* Remember what it is */
            player.t_action = C_ZAP;    /* We are quaffing */
            player.t_no_move = 2 * movement(&player);
            return(TRUE);
        }

        item = player.t_using;
        /* We've waited our time, let's shoot 'em up! */
        player.t_using = NULL;
        player.t_action = A_NIL;

        obj = OBJPTR(item);

        /* Handle relics specially here */
        if (obj->o_type == RELIC) {
            switch (obj->o_which) {
                case ORCUS_WAND:
            /* msg(nothing); */
            read_scroll(S_PETRIFY, NULL, FALSE);
            return(TRUE);
                when MING_STAFF:
                    which = WS_MISSILE;
                when EMORI_CLOAK:
                    which = WS_PARALYZE;
                    obj->o_charges = 0; /* one zap/day (whatever that is) */
                    fuse(cloak_charge, obj, CLOAK_TIME, AFTER);
                when ASMO_ROD:
                    switch (rnd(3)) {
                        case 0:         which = WS_ELECT;
                        when 1:         which = WS_COLD;
                        otherwise:      which = WS_FIRE;
                    }
            }
        }
        else {
            which = obj->o_which;
            ws_know[which] = TRUE;
            flag = obj->o_flags;
        }
    }
    do_zap(&player, obj, &player.t_newpos, which, flag);
    return(TRUE);
}

