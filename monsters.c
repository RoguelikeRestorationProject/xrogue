/*
    monsters.c - File with various monster functions in it
    
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
#include <string.h>
#include "rogue.h"

/*
 * Check_residue takes care of any effect of the monster 
 */

check_residue(tp)
register struct thing *tp;
{
    /*
     * Take care of special abilities
     */
    if (on(*tp, DIDHOLD) && (--hold_count == 0)) {
        turn_off(player, ISHELD);
        turn_off(*tp, DIDHOLD);
    }

    /* If frightened of this monster, stop */
    if (on(player, ISFLEE) &&
        player.t_dest == &tp->t_pos) turn_off(player, ISFLEE);

    /* If monster was suffocating player, stop it */
    if (on(*tp, DIDSUFFOCATE)) {
        extinguish(suffocate);
        turn_off(*tp, DIDSUFFOCATE);
    }

    /* If something with fire, may darken */
    if (on(*tp, HASFIRE)) {
        register struct room *rp=roomin(&tp->t_pos);
        register struct linked_list *fire_item;

        if (rp) {
            for (fire_item = rp->r_fires; fire_item != NULL;
                 fire_item = next(fire_item)) {
                if (THINGPTR(fire_item) == tp) {
                    detach(rp->r_fires, fire_item);
                    destroy_item(fire_item);
                    if (rp->r_fires == NULL) {
                        rp->r_flags &= ~HASFIRE;
                        if (cansee(tp->t_pos.y, tp->t_pos.x)) light(&hero);
                    }
                    break;
                }
            }
        }
    }
}

/*
 * Creat_mons creates the specified monster -- any if 0 
 */

bool
creat_mons(person, monster, report)
struct thing *person;   /* Where to create next to */
short monster;
bool report;
{
    struct linked_list *nitem;
    register struct thing *tp;
    struct room *rp;
    coord *mp;

    if (levtype == POSTLEV)
        return(FALSE);
    if ((mp = fallpos(&(person->t_pos), FALSE, 2)) != NULL) {
        nitem = new_item(sizeof (struct thing));
        new_monster(nitem,
                    monster == 0 ? randmonster(FALSE, FALSE)
                                 : monster,
                    mp,
                    TRUE);
        tp = THINGPTR(nitem);
        runto(tp, &hero);
        carry_obj(tp, monsters[tp->t_index].m_carry/2); /* only half chance */

        /* since it just got here, it is disoriented */
        tp->t_no_move = 2 * movement(tp);

        if (on(*tp, HASFIRE)) {
            rp = roomin(&tp->t_pos);
            if (rp) {
                register struct linked_list *fire_item;

                /* Put the new fellow in the room list */
                fire_item = creat_item();
                ldata(fire_item) = (char *) tp;
                attach(rp->r_fires, fire_item);

                rp->r_flags |= HASFIRE;
            }
        }

        /* 
         * If we can see this monster, set oldch to ' ' to make light()
         * think the creature used to be invisible (ie. not seen here)
         */
        if (cansee(tp->t_pos.y, tp->t_pos.x)) tp->t_oldch = ' ';
        return(TRUE);
    }
    if (report) msg("You hear a faint cry of anguish in the distance.. ");
    return(FALSE);
}

/*
 * Genmonsters:
 *      Generate at least 'least' monsters for this single room level.
 *      'Treas' indicates whether this is a "treasure" level.
 */

void
genmonsters(least, treas)
register int least;
bool treas;
{
    reg int i;
    reg struct room *rp = &rooms[0];
    reg struct linked_list *item;
    reg struct thing *mp;
    coord tp;

    for (i = 0; i < (max(50, level) + least); i++) {
            if (!treas && rnd(100) < 65)        /* put in some little buggers */
                continue;

            /*
             * Put the monster in
             */
            item = new_item(sizeof *mp);
            mp = THINGPTR(item);
            do {
                    rnd_pos(rp, &tp);
            } until(mvwinch(stdscr, tp.y, tp.x) == FLOOR);

            new_monster(item, randmonster(FALSE, FALSE), &tp, FALSE);
            /*
             * See if we want to give it a treasure to carry around.
             */
            carry_obj(mp, monsters[mp->t_index].m_carry);

            /* Calculate a movement rate */
            mp->t_no_move = movement(mp);

            /* Is it going to give us some light? */
            if (on(*mp, HASFIRE)) {
                register struct linked_list *fire_item;

                fire_item = creat_item();
                ldata(fire_item) = (char *) mp;
                attach(rp->r_fires, fire_item);
                rp->r_flags |= HASFIRE;
            }
    }
}

/*
 * id_monst returns the index of the monster given its letter
 */

short
id_monst(monster)
register char monster;
{
    register short result;

    if (levtype == OUTSIDE) {
        result = NLEVMONS*vlevel + (NUMMONST-NUMDINOS-1);
        if (result > NUMMONST) result = NUMMONST;
    }
    else {  
        result = NLEVMONS*vlevel;
        if (result > NUMMONST-NUMDINOS) result = NUMMONST-NUMDINOS;
    }

    if (levtype == OUTSIDE) {
        for(; result>(NUMMONST-NUMDINOS-1); result--)
            if (monsters[result].m_appear == monster) return(result);
        for (result=(NLEVMONS*vlevel)+1; result <= NUMMONST-NUMDINOS; result++)
            if (monsters[result].m_appear == monster) return(result);
    }
    else {
        for(; result>0; result--)
            if (monsters[result].m_appear == monster) return(result);
        for (result=(NLEVMONS*vlevel)+1; result <= NUMMONST; result++)
            if (monsters[result].m_appear == monster) return(result);
    }
    return(0);
}


/*
 * new_monster:
 *      Pick a new monster and add it to the list
 */

new_monster(item, type, cp, max_monster)
struct linked_list *item;
short type;
coord *cp;
bool max_monster;
{
    register struct thing *tp;
    register struct monster *mp;
    register char *ip, *hitp;
    register int i, min_intel, max_intel;
    register int num_dice, num_sides=8, num_extra=0;

    attach(mlist, item);
    tp = THINGPTR(item);
    tp->t_pack = NULL;
    tp->t_index = type;
    tp->t_wasshot = FALSE;
    tp->t_type = monsters[type].m_appear;
    tp->t_ctype = C_MONSTER;
    tp->t_action = A_NIL;
    tp->t_doorgoal.x = tp->t_doorgoal.y = -1;
    tp->t_quiet = 0;
    tp->t_dest = NULL;
    tp->t_name = NULL;
    tp->t_pos = tp->t_oldpos = *cp;
    tp->t_oldch = mvwinch(cw, cp->y, cp->x);
    mvwaddch(mw, cp->y, cp->x, tp->t_type);
    mp = &monsters[tp->t_index];

    /* Figure out monster's hit points */
    hitp = mp->m_stats.ms_hpt;
    num_dice = atoi(hitp);
    if ((hitp = strchr(hitp, 'd')) != NULL) {
        num_sides = atoi(++hitp);
        if ((hitp = strchr(hitp, '+')) != NULL)
            num_extra = atoi(++hitp);
    }

    tp->t_stats.s_lvladj = 0;
    tp->t_stats.s_lvl = mp->m_stats.ms_lvl;
    tp->t_stats.s_arm = mp->m_stats.ms_arm;
    strcpy(tp->t_stats.s_dmg,mp->m_stats.ms_dmg);
    tp->t_stats.s_str = mp->m_stats.ms_str;
    tp->t_stats.s_dext = mp->m_stats.ms_dex;
    tp->t_movement = mp->m_stats.ms_move;
    if (vlevel > HARDER) { /* the deeper, the meaner we get */
         tp->t_stats.s_lvl += (vlevel - HARDER);
         num_dice += (vlevel - HARDER)/2;
         tp->t_stats.s_arm -= (vlevel - HARDER) / 4;
    }
    if (max_monster)
        tp->t_stats.s_hpt = num_dice * num_sides + num_extra;
    else
        tp->t_stats.s_hpt = roll(num_dice, num_sides) + num_extra;
    tp->t_stats.s_exp = mp->m_stats.ms_exp + mp->m_add_exp*tp->t_stats.s_hpt;

    /*
     * just initailize others values to something reasonable for now
     * maybe someday will *really* put these in monster table
     */
    tp->t_stats.s_wisdom = 8 + rnd(7);
    tp->t_stats.s_const = 8 + rnd(7);
    tp->t_stats.s_charisma = 8 + rnd(7);

    /* Set the initial flags */
    for (i=0; i<16; i++) tp->t_flags[i] = 0;
    for (i=0; i<MAXFLAGS; i++)
        turn_on(*tp, mp->m_flags[i]);

    /*
     * these are the base chances that a creatures will do something
     * assuming it can. These are(or can be) modified at runtime
     * based on what the creature experiences
     */
    tp->t_breathe = 70;         /* base chance of breathing */
    tp->t_artifact = 90;        /* base chance of using artifact */
    tp->t_summon = 50;          /* base chance of summoning */
    tp->t_cast = 70;            /* base chance of casting a spell */
    tp->t_wand = on(*tp, ISUNIQUE) ? 35 : 50;   /* base chance of using wands */

    /* suprising monsters don't always surprise you */
    if (!max_monster            && on(*tp, CANSURPRISE) && 
        off(*tp, ISUNIQUE)      && rnd(100) < 25)
            turn_off(*tp, CANSURPRISE);

    /* If this monster is unique, gen it */
    if (on(*tp, ISUNIQUE)) mp->m_normal = FALSE;

    /* 
     * If it is the quartermaster, then compute his level and exp pts
     * based on the level. This will make it fair when thieves try to
     * steal and give them reasonable experience if they succeed.
     * Then fill his pack with his wares.
     */
    if (on(*tp, CANSELL)) {     
        tp->t_stats.s_exp = vlevel * 100;
        tp->t_stats.s_lvl = vlevel/2 + 1;
        make_sell_pack(tp);
    }

    /* Normally scared monsters have a chance to not be scared */
    if (on(*tp, ISFLEE) && (rnd(4) == 0)) turn_off(*tp, ISFLEE);

    /* Figure intelligence */
    min_intel = atoi(mp->m_intel);
    if ((ip = (char *) strchr(mp->m_intel, '-')) == NULL)
        tp->t_stats.s_intel = min_intel;
    else {
        max_intel = atoi(++ip);
        if (max_monster)
            tp->t_stats.s_intel = max_intel;
        else
            tp->t_stats.s_intel = min_intel + rnd(max_intel - min_intel);
    }
    if (vlevel > HARDER) 
         tp->t_stats.s_intel += ((vlevel - HARDER)/2);
    tp->maxstats = tp->t_stats;

    /* If the monster can shoot, it may have a weapon */
    if (on(*tp, CANSHOOT) && ((rnd(100) < (20 + vlevel)) || max_monster)) {
        struct linked_list *item1;
        register struct object *cur, *cur1;

        item = new_item(sizeof *cur);
        item1 = new_item(sizeof *cur1);
        cur = OBJPTR(item);
        cur1 = OBJPTR(item1);
        cur->o_hplus = (rnd(4) < 3) ? 0
                                    : (rnd(3) + 1) * ((rnd(3) < 2) ? 1 : -1);
        cur->o_dplus = (rnd(4) < 3) ? 0
                                    : (rnd(3) + 1) * ((rnd(3) < 2) ? 1 : -1);
        cur1->o_hplus = (rnd(4) < 3) ? 0
                                    : (rnd(3) + 1) * ((rnd(3) < 2) ? 1 : -1);
        cur1->o_dplus = (rnd(4) < 3) ? 0
                                    : (rnd(3) + 1) * ((rnd(3) < 2) ? 1 : -1);
        strcpy(cur->o_damage,"0d0");
        strcpy(cur->o_hurldmg,"0d0");
        strcpy(cur1->o_damage,"0d0");
        strcpy(cur1->o_hurldmg,"0d0");
        cur->o_ac = cur1->o_ac = 11;
        cur->o_count = cur1->o_count = 1;
        cur->o_group = cur1->o_group = 0;
        cur->contents = cur1->contents = NULL;
        if ((cur->o_hplus <= 0) && (cur->o_dplus <= 0)) cur->o_flags = ISCURSED;
        if ((cur1->o_hplus <= 0) && (cur1->o_dplus <= 0))
            cur1->o_flags = ISCURSED;
        cur->o_flags = cur1->o_flags = 0;
        cur->o_type = cur1->o_type = WEAPON;
        cur->o_mark[0] = cur1->o_mark[0] = '\0';

        /* The monster may use a crossbow, sling, or an arrow */
        i = rnd(100);
        if (i < 35) {
            cur->o_which = CROSSBOW;
            cur1->o_which = BOLT;
            init_weapon(cur, CROSSBOW);
            init_weapon(cur1, BOLT);
        }
        else if (i < 70) {
            cur->o_which = BOW;
            cur1->o_which = ARROW;
            init_weapon(cur, BOW);
            init_weapon(cur1, ARROW);
        }
        else {
            cur->o_which = SLING;
            cur1->o_which = ROCK;
            init_weapon(cur, SLING);
            init_weapon(cur1, ROCK);
        }

        attach(tp->t_pack, item);
        attach(tp->t_pack, item1);
    }


    /* Calculate the initial movement rate */
    updpack(TRUE, tp);
    tp->t_no_move = movement(tp);

    if (ISWEARING(R_AGGR))
        runto(tp, &hero);

    if (on(*tp, ISDISGUISE))
    {
        char mch = 0;

        if (tp->t_pack != NULL)
            mch = (OBJPTR(tp->t_pack))->o_type;
        else
            switch (rnd(10)) {
                case 0: mch = GOLD;
                when 1: mch = POTION;
                when 2: mch = SCROLL;
                when 3: mch = FOOD;
                when 4: mch = WEAPON;
                when 5: mch = ARMOR;
                when 6: mch = RING;
                when 7: mch = STICK;
                when 8: mch = monsters[randmonster(FALSE, FALSE)].m_appear;
                when 9: mch = MM;
            }
        tp->t_disguise = mch;
    }
}

/*
 * randmonster:
 *      Pick a monster to show up.  The lower the level,
 *      the meaner the monster.
 */

short
randmonster(wander, no_unique)
register bool wander, no_unique;
{
    register int d, cur_level, range, i; 

    /* 
     * Do we want a merchant? Merchant is always in place 'NUMMONST' 
     */
    if (wander && monsters[NUMMONST].m_wander && rnd(100) < pstats.s_charisma/3)
        return NUMMONST;

    cur_level = vlevel;
    range = (4*NLEVMONS)+1;  /* range is 0 thru 12 */
    i = 0;
    do
    {
        if (i++ > NUMMONST-1) {    /* in case all have be genocided */
            i = 0;
            if (--cur_level <= 0)
                fatal("rogue: Could not find a monster to make! ");
        }
        if (levtype == OUTSIDE) {                 /* create DINOSUARS */
            d = (cur_level - rnd(range/2)) + (NUMMONST-NUMDINOS-1);
            if (d < NUMMONST-NUMDINOS)
                d = (NUMMONST-NUMDINOS) + rnd(range/2);
            if (d > NUMMONST-1)
                d = (NUMMONST-NUMDINOS) + rnd(NUMDINOS);
        }
        else {           /* Create NORMALs and UNIQs here */
            d = (NLEVMONS*(cur_level-1) + rnd(range) - (range-NLEVMONS-1));
            if (d < 1) d = rnd(6)+1;

            if (d > NUMMONST-NUMDINOS-1) {    /* Entire range NORMs + UNIQs */
        if (no_unique)   /* Choose from last 12 NORMAL monsters */
                    d = (NUMMONST-NUMDINOS-NUMUNIQUE-1) - rnd(NUMUNIQUE/5);
        else             /* Choose from entire UNIQ monsters + range */
            d = (NUMMONST-NUMDINOS-1) - rnd(NUMUNIQUE+range);
            }
                     /* Half-way into the UNIQs now */
            else if (d > (NUMMONST-NUMDINOS-(NUMUNIQUE/2)-1)) {
        if (no_unique)   /* Choose from last 15 NORMAL monsters */
                    d = (NUMMONST-NUMDINOS-NUMUNIQUE-1) - rnd(NUMUNIQUE/4);
        else             /* Choose from entire UNIQ monsters + range */
                    d = (NUMMONST-NUMDINOS-1) - rnd(NUMUNIQUE+range);
        }
                     /* End NORMALs and begin relic bearing UNIQs */
            else if (d > (NUMMONST-NUMDINOS-NUMUNIQUE-1)) {
        if (no_unique)   /* Choose from last 20 NORMAL monsters */
                    d = (NUMMONST-NUMDINOS-NUMUNIQUE-1) - rnd(NUMUNIQUE/3);
        else             /* Choose from first 20 UNIQ monsters */
                    d = (NUMMONST-NUMDINOS-NUMUNIQUE-1) + rnd(NUMUNIQUE/3);
        }
        }
    }
    while  (wander ? !monsters[d].m_wander || !monsters[d].m_normal 
                   : !monsters[d].m_normal);
    return d;
}

/* Sell displays a menu of goods from which the player may choose
 * to purchase something.
 */

sell(tp)
register struct thing *tp;
{
    register struct linked_list *item, *seller;
    register struct linked_list *sellpack;
    register struct object *obj;
    register long worth, min_worth;
    char buffer[LINELEN];

    /*
     * Get a linked_list pointer to the seller.  We need this in case
     * he disappears so we can set him ISDEAD.
     */
    seller = find_mons(tp->t_pos.y, tp->t_pos.x);

    sellpack = tp->t_pack;
    if (sellpack == NULL) {
        msg("%s looks puzzled and departs.", prname(monster_name(tp), TRUE));

        /* Get rid of the monster */
        killed(seller, FALSE, FALSE, FALSE);
        return;
    }

    /* See how much the minimum pack item is worth */
    min_worth = 100000;
    for (item = sellpack; item != NULL; item = next(item)) {
        obj = OBJPTR(item);
        obj->o_flags |= ISPOST; /* Force a long description of the item */
        worth = get_worth(obj);
        if (worth < min_worth) min_worth = worth;
    }

    /* See if player can afford an item */
    if (min_worth > purse) {
        msg("%s eyes your small purse and departs.", 
            prname(monster_name(tp), TRUE));

        /* Get rid of the monster */
        killed(seller, FALSE, FALSE, FALSE);
        return;
    }

    /* Announce our intentions */
    msg("%s opens his pack.  --More--", prname(monster_name(tp), TRUE));
    wait_for(' ');

    /* Try to sell something */
    sprintf(buffer, "You got %ld gold pieces.  Buy", purse);
    item = get_item(sellpack, buffer, ALL, TRUE, TRUE);

    /* Get rid of the monster */
    if (item != NULL) detach(tp->t_pack, item); /* Take it out of the pack */
    killed(seller, FALSE, FALSE, FALSE);

    if (item == NULL) return;

    /* Can he afford the selected item? */
    obj = OBJPTR(item);

    worth = get_worth(obj);
    if (worth > purse) {
        msg("You cannot afford it.");
        o_discard(item);
        return;
    }

    /* Charge him through the nose */
    purse -= worth;

    /* If a stick or ring, let player know the type */
    switch (obj->o_type) {
        case RING:   r_know[obj->o_which]  = TRUE;
        when POTION: p_know[obj->o_which]  = TRUE;
        when SCROLL: s_know[obj->o_which]  = TRUE;
        when STICK:  ws_know[obj->o_which] = TRUE;
        when MM:     m_know[obj->o_which]  = TRUE;

    }

    /* identify it */
    whatis (item);

    /* Remove the POST flag that we used for get_item() */
    obj->o_flags &= ~ISPOST;

    if (add_pack(item, FALSE) == FALSE) {
        obj->o_pos = hero;
        fall(item, TRUE);
    }
}

/*
 * what to do when the hero steps next to a monster
 */

struct linked_list *
wake_monster(y, x)
int y, x;
{
    register struct thing *tp;
    register struct linked_list *it;
    register struct room *trp;
    register char *mname;
    bool nasty; /* Will the monster "attack"? */

    if ((it = find_mons(y, x)) == NULL) {
        msg("Wake:  can't find monster in show (%d, %d)", y, x);
        return (NULL);
    }
    tp = THINGPTR(it);
    if (on(*tp, ISSTONE)) /* if stoned, don't do anything */
        return it;

    /*
     * For now, if we are a friendly monster, we won't do any of
     * our special effects.
     */
    if (on(*tp, ISFRIENDLY)) return it;

    trp = roomin(&tp->t_pos); /* Current room for monster */

    /*
     * Let greedy ones in a room guard gold
     * (except in a maze where lots of creatures would all go for the 
     * same piece of gold)
     */
    if (on(*tp, ISGREED) && off(*tp, ISRUN) && levtype != MAZELEV &&
    trp != NULL && lvl_obj != NULL) {
            register struct linked_list *item;
            register struct object *cur;

            for (item = lvl_obj; item != NULL; item = next(item)) {
                cur = OBJPTR(item);
                if ((cur->o_type == GOLD) && (roomin(&cur->o_pos) == trp)) {
                    /* Run to the gold */
                    runto(tp, &cur->o_pos);

                    /* Make it worth protecting */
                    cur->o_count += GOLDCALC + GOLDCALC;
                    break;
                }
            }
    }

    /*
     * Every time he sees mean monster, it might start chasing him
     */
    if (on(*tp, ISMEAN)  && 
        off(*tp, ISHELD) && 
        off(*tp, ISRUN)  && 
        rnd(100) > 35    && 
        (!is_stealth(&player) || (on(*tp, ISUNIQUE) && rnd(100) > 35)) &&
        (off(player, ISINVIS) || on(*tp, CANSEE)) ||
        (trp != NULL && (trp->r_flags & ISTREAS))) {
        runto(tp, &hero);
    }

    /*
     * Get the name; we don't want to do it until here because we need to
     * know whether the monster is still sleeping or not.
     */
    mname = monster_name(tp);

    /* See if the monster will bother the player */
    nasty = (on(*tp, ISRUN) && cansee(tp->t_pos.y, tp->t_pos.x));

    /*
     * if the creature is awake and can see the player and the
     * player has the dreaded "eye of vecna" then see if the
     * creature is turned to stone
     */
    if (cur_relic[EYE_VECNA] && nasty && off(*tp, NOSTONE) &&
        (off(player, ISINVIS) || on(*tp, CANSEE))) {
        turn_on(*tp, NOSTONE);  /* only have to save once */
        if (!save(VS_PETRIFICATION, tp, -2)) {
                turn_on(*tp, ISSTONE);
                turn_off(*tp, ISRUN);
                turn_off(*tp, ISINVIS);
                turn_off(*tp, CANSURPRISE);
                turn_off(*tp, ISDISGUISE);
                msg("%s is turned to stone!", prname(mname, TRUE));
                return it;
        }
    }

    /* 
     * Handle monsters that can gaze and do things while running
     * Player must be able to see the monster and the monster must 
     * not be asleep 
     */
    if (nasty && !invisible(tp)) {
        /*
         * Confusion
         */
        if (on(*tp, CANHUH)                              &&
           (off(*tp, ISINVIS)     || on(player, CANSEE)) &&
           (off(*tp, CANSURPRISE) || ISWEARING(R_ALERT))) {
            if (!save(VS_MAGIC, &player, 0)) {
                if (off(player, ISCLEAR)) {
                    if (find_slot(unconfuse))
                        lengthen(unconfuse, HUHDURATION);
                    else {
                        fuse(unconfuse, (VOID *)NULL, HUHDURATION, AFTER);
                        msg("%s's gaze has confused you.",prname(mname, TRUE));
                        turn_on(player, ISHUH);
                    }
                }
                else msg("You feel dizzy for a moment, but it quickly passes.");
            }
            else if (rnd(100) < 67)
                turn_off(*tp, CANHUH); /* Once you save, maybe that's it */
        }

        /* Sleep */
        if(on(*tp, CANSNORE) &&  
           player.t_action != A_FREEZE && 
           !save(VS_PARALYZATION, &player, 0)) {
            if (ISWEARING(R_ALERT))
                msg("You feel drowsy for a moment.. ");
            else {
                msg("%s's gaze puts you to sleep! ", prname(mname, TRUE));
                player.t_no_move += movement(&player) * SLEEPTIME;
                player.t_action = A_FREEZE;
                if (rnd(100) < 50) turn_off(*tp, CANSNORE);
            }
        }

        /* Fear */
        if (on(*tp, CANFRIGHTEN) && !on(player, ISFLEE)) {
            turn_off(*tp, CANFRIGHTEN);
            if (!ISWEARING(R_HEROISM) && 
                !save(VS_WAND, &player, -(tp->t_stats.s_lvl/10))) {
                    turn_on(player, ISFLEE);
                    player.t_dest = &tp->t_pos;
                    msg("The sight of %s terrifies you!", prname(mname, FALSE));
            }
        }

        /* blinding creatures */
        if(on(*tp, CANBLIND) && !find_slot(sight)) {
            turn_off(*tp, CANBLIND);
            if (!save(VS_WAND, &player, 0)) {
                msg("The gaze of %s blinds you! ", prname(mname, FALSE));
                turn_on(player, ISBLIND);
                fuse(sight, (VOID *)NULL, rnd(30)+20, AFTER);
                light(&hero);
            }
        }

        /* the sight of the ghost can age you! */
        if (on(*tp, CANAGE)) { 
            turn_off (*tp, CANAGE);
            if (!save(VS_MAGIC, &player, 0)) {
                msg ("The sight of %s ages you!", prname(mname, FALSE));
                pstats.s_const--;
                /* max_stats.s_const--; */
                if (pstats.s_const < 1) {
            pstats.s_hpt = -1;
                    death (D_CONSTITUTION);
        }
            }
        }

        /* Turning to stone */
        if (on(*tp, LOOKSTONE)) {
            turn_off(*tp, LOOKSTONE);

            if (on(player, CANINWALL))
                msg("The gaze of %s has no effect.", prname(mname, FALSE));
            else {
                if (!save(VS_PETRIFICATION, &player, 0) && rnd(100) < 5) {
                    pstats.s_hpt = -1;
                    msg("The gaze of %s petrifies you!", prname(mname, FALSE));
                    msg("You are turned to stone!!!  --More--");
                    wait_for(' ');
                    death(D_PETRIFY);
                }
                else {
                    msg("The gaze of %s stiffens your limbs.", 
                        prname(mname, FALSE));
                    player.t_no_move += movement(&player) * STONETIME;
                    player.t_action = A_FREEZE;
                }
            }
        }
    }

    return it;
}
/*
 * wanderer:
 *      A wandering monster has awakened and is headed for the player
 */

wanderer()
{
    register int i;
    register struct room *hr = roomin(&hero);
    register struct linked_list *item;
    register struct thing *tp;
    register long *attr;        /* Points to monsters' attributes */
    int carry;  /* Chance of wanderer carrying anything */
    short rmonst;       /* Our random wanderer */
    bool canteleport = FALSE,   /* Can the monster teleport? */
         seehim;        /* Is monster within sight? */
    coord cp;

    rmonst = randmonster(TRUE, FALSE);  /* Choose a random wanderer */
    attr = &monsters[rmonst].m_flags[0]; /* Start of attributes */
    for (i=0; i<MAXFLAGS; i++)
        if (*attr++ == CANTELEPORT) {
            canteleport = TRUE;
            break;
        }

    /* Find a place for it -- avoid the player's room if can't teleport */
    do {
        do {
            i = rnd_room();
        } until (canteleport || hr != &rooms[i] || levtype == MAZELEV ||
                 levtype == OUTSIDE);

        /* Make sure the monster does not teleport on top of the player */
        do {
            rnd_pos(&rooms[i], &cp);
        } while (hr == &rooms[i] && ce(cp, hero));
    } until (step_ok(cp.y, cp.x, NOMONST, (struct thing *)NULL));

    /* Create a new wandering monster */
    item = new_item(sizeof *tp);
    new_monster(item, rmonst, &cp, FALSE);
    tp = THINGPTR(item);
    runto(tp, &hero);
    tp->t_pos = cp;     /* Assign the position to the monster */
    seehim = cansee(tp->t_pos.y, tp->t_pos.x);
    if (on(*tp, HASFIRE)) {
        register struct room *rp;

        rp = roomin(&tp->t_pos);
        if (rp) {
            register struct linked_list *fire_item;

            fire_item = creat_item();
            ldata(fire_item) = (char *) tp;
            attach(rp->r_fires, fire_item);

            rp->r_flags |= HASFIRE;
            if (seehim && next(rp->r_fires) == NULL)
                light(&hero);
        }
    }

    /* See if we give the monster anything */
    carry = monsters[tp->t_index].m_carry;
    if (off(*tp, ISUNIQUE)) carry /= 2; /* Non-unique has only a half chance */
    carry_obj(tp, carry);

    /* Calculate its movement rate */
    tp->t_no_move = movement(tp);

    /* Alert the player if a monster just teleported in */
    if (hr == &rooms[i] && canteleport && seehim && !invisible(tp)) {
        msg("A %s just teleported in", monster_name(tp));
        light(&hero);
        running = FALSE;
    }

    if (wizard)
        msg("Started a wandering %s", monster_name(tp));
}

