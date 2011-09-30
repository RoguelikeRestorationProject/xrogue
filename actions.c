/*
    actions.c  -  functions for dealing with monster actions
   
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

#include <ctype.h>
#include <curses.h>
#include <limits.h>
#include "rogue.h"

int mf_count = 0;       /* move_free counter - see actions.c(m_act()) */
int mf_jmpcnt = 0;      /* move_free counter for # of jumps           */

/* 
 * Did we disrupt a spell? 
 */
dsrpt_monster(tp, always, see_him)
register struct thing *tp;
bool always, see_him;
{
    switch (tp->t_action) {
    case A_SUMMON:
    case A_MISSILE:
    case A_SLOW:
        tp->t_action = A_NIL;   /* Just make the old fellow start over again */
        tp->t_no_move = movement(tp);
        tp->t_using = NULL; /* Just to be on the safe side */
        turn_on(*tp, WASDISRUPTED);
        if (see_him)
            msg("%s's spell has been disrupted.",prname(monster_name(tp),TRUE));
        /*
         * maybe choose something else to do next time since player
         * is disrupting us
         */
        tp->t_summon *= 2;
        tp->t_cast /= 2;
        return;
    }

    /* We may want to disrupt other actions, too */
    if (always) {
        tp->t_action = A_NIL; /* Just make the old fellow start over again */
        tp->t_no_move = movement(tp);
        tp->t_using = NULL;/* Just to be on the safe side */
    }
}

dsrpt_player()
{
    int which, action;
    struct linked_list *item;
    struct object *obj;
    
    action = player.t_action;
    which = player.t_selection;

    switch (action) {
    case C_CAST: /* Did we disrupt a spell? */
    case C_PRAY:
    case C_CHANT:
    {
        msg("Your %s was disrupted!", action == C_CAST ? "spell" : "prayer");

        /* Charge him 1/4 anyway */
        if (action == C_CAST)
            spell_power += magic_spells[which].s_cost / 4;
        else if (action == C_PRAY)
            pray_time += cleric_spells[which].s_cost / 4;
        else if (action == C_CHANT)
            chant_time += druid_spells[which].s_cost / 4;
    }
    when C_COUNT: /* counting of gold? */
    {
        if (purse > 0) {
            msg("Your gold goes flying everywhere!");
            do {
                item = spec_item(GOLD, NULL, NULL, NULL);
                obj = OBJPTR(item);
                obj->o_count = min(purse, rnd(20)+1);
                purse -= obj->o_count;
                obj->o_pos = hero;
                fall(item, FALSE);
            } while (purse > 0 && rnd(25) != 1);
        }
    }
    when C_EAT:
        msg("Ack!  You gag on your food for a moment. ");
        del_pack(player.t_using);
        
    when A_PICKUP:
        msg("You drop what you are picking up! ");

    when C_SEARCH:      /* searching for traps and secret doors... */
        msg("Ouch!  You decide to stop searching. ");
        count = 0;      /* don't search again */

    when C_SETTRAP:
        msg("Ouch!  You can't set a trap right now. ");

    when A_NIL:
    default:
        return;
    }
    player.t_no_move = movement(&player); /* disoriented for a while */
    player.t_action = A_NIL;
    player.t_selection = 0;
}

/*
 * m_act:
 *      If the critter isn't doing anything, choose an action for it.
 *      Otherwise, let it perform its chosen action.
 */

m_act(tp)
register struct thing *tp;
{
    struct object *obj;
    bool flee;          /* Are we scared? */

    /* What are we planning to do? */
    switch (tp->t_action) {
        default:
            /* An unknown action! */
            msg("Unknown monster action (%d)", tp->t_action);

            /* Fall through */

        case A_NIL:
            /* If the monster is fairly intelligent and about to die, it
             * may turn tail and run.  But if we are a FRIENDLY creature
             * in the hero's service, don't run.
             */
            if (off(*tp, ISFLEE)                                        &&
                tp->t_stats.s_hpt < tp->maxstats.s_hpt                  &&
                tp->t_stats.s_hpt < max(10, tp->maxstats.s_hpt/6)       &&
                (off(*tp, ISFRIENDLY) || tp->t_dest != &hero)           &&
                rnd(25) < tp->t_stats.s_intel) {
                    turn_on(*tp, ISFLEE);

                    /* It is okay to turn tail */
                    tp->t_oldpos = tp->t_pos;
                }

            /* Should the monster run away? */
            flee = on(*tp, ISFLEE) ||
                ((tp->t_dest == &hero) && on(player, ISINWALL) &&
                 off(*tp, CANINWALL));

            m_select(tp, flee); /* Select an action */
            return;

        when A_ATTACK:
            /* 
             * We're trying to attack the player or monster at t_newpos 
             * if the prey moved, do nothing
             */
            obj = tp->t_using ? OBJPTR(tp->t_using) : NULL;
            if (ce(tp->t_newpos, hero)) {
                attack(tp, obj, FALSE);
            }
            else if (mvwinch(mw, tp->t_newpos.y, tp->t_newpos.x) &&
                     step_ok(tp->t_newpos.y, tp->t_newpos.x, FIGHTOK, tp)) {
                skirmish(tp, &tp->t_newpos, obj, FALSE);
            }

        when A_SELL:
                /* Is the quartermaster still next to us? */
            if (ce(tp->t_newpos, hero)) sell(tp);

                /* The quartermaster moved away */
            else if (off(player, ISBLIND) && cansee(unc(tp->t_pos)) &&
                (off(*tp, ISINVIS)     || on(player, CANSEE)) &&
                (off(*tp, ISSHADOW)    || on(player, CANSEE)) &&
                (off(*tp, CANSURPRISE) || ISWEARING(R_ALERT)) &&
        (rnd(12) < 4))
                msg("%s grunts with frustration",prname(monster_name(tp),TRUE));

        when A_MOVE:
            /* Let's try to move */
            do_chase(tp);

            /* If t_no_move > 0, we found that we have to fight! */
            if (tp->t_no_move > 0) return;

        when A_BREATHE:
            /* Breathe on the critter */
            m_breathe(tp);

        when A_SLOW:
            /* make him move slower */
            add_slow();
            turn_off(*tp, CANSLOW);

        when A_MISSILE:
            /* Start up a magic missile spell */
            m_spell(tp);

        when A_SONIC:
            /* Let out a sonic blast! */
            m_sonic(tp);

        when A_THROW:
            /* We're throwing something (like an arrow) */
            missile(tp->t_newpos.y, tp->t_newpos.x, tp->t_using, tp);

        when A_SUMMON:
            /* We're summoning help */
            m_summon(tp);

        when A_USERELIC:
            /* Use our relic */
            m_use_relic(tp);

        when A_USEWAND:
            /* use the wand we have */
            m_use_wand(tp);
    }

    /* Can we in fact move?  (we might have solidified in solid rock) */
    if (!step_ok(hero.y, hero.x, NOMONST, &player)) {

         if (move_free > 1) goto jump_over;   /* avoid messages */
         if (mf_count > 2)  goto jump_over;   /* limit messages */

         if (pstats.s_hpt < 1) {
         pstats.s_hpt = -1;
             msg("You have merged into the surroundings!  --More--");
             wait_for(' ');
             death(D_PETRIFY);
         }
         else {
             mf_count += 1;  /* count number of times we are here */
             pstats.s_hpt -= rnd(2)+1;
             if (pstats.s_hpt < 1) {
          pstats.s_hpt = -1;
                  msg("You have merged into the surroundings!  --More--");
                  wait_for(' ');
                  death(D_PETRIFY);
             }
         }
         switch (rnd(51)) {
             case 0: msg("Arrrggghhhhh!! ");
             when 5: msg("You can't move! "); 
             when 10: msg("You motion angrily! "); 
             when 15: msg("You feel so weird! ");
             when 20: msg("If only you could phase. ");
             when 25: msg("The rock maggots are closing in! ");
             when 30: msg("You wrench and wrench and wrench... ");
             when 35: msg("You wish you could teleport out of here! ");
             when 40: msg("Your feel your life force ebbing away... ");
             when 45: msg("You partially regain your senses. ");
             when 50: msg("The rock maggots have found you!!! "); 
             otherwise: pstats.s_hpt -= rnd(4)+1;
         }
         if (pstats.s_hpt < 1) {
          pstats.s_hpt = -1;
              msg("You lose the urge to live...  --More--");
              wait_for(' ');
              death(D_PETRIFY);
     }
        jump_over:
        mf_jmpcnt++;          /* count this jump */
        if (mf_jmpcnt > 9) {  /* take a few turns, then reset it */
            mf_jmpcnt = 0;
            mf_count  = 0;
        }
    }

    /* No action now */
    tp->t_action = A_NIL;
    tp->t_using = NULL;
}

/*
 * m_breathe:
 *      Breathe in the chosen direction.
 */

m_breathe(tp)
register struct thing *tp;
{
    register int damage;
    register char *breath = NULL;

    damage = tp->t_stats.s_hpt;
    turn_off(*tp, CANSURPRISE);

    /* Will it breathe at random */
    if (on(*tp, CANBRANDOM)) {
        /* Turn off random breath */
        turn_off(*tp, CANBRANDOM);

        /* Select type of breath */
        switch (rnd(10)) {
            case 0: breath = "acid";
                    turn_on(*tp, NOACID);
            when 1: breath = "flame";
                    turn_on(*tp, NOFIRE);
            when 2: breath = "lightning bolt";
                    turn_on(*tp, NOBOLT);
            when 3: breath = "chlorine gas";
                    turn_on(*tp, NOGAS);
            when 4: breath = "ice";
                    turn_on(*tp, NOCOLD);
            when 5: breath = "nerve gas";
                    turn_on(*tp, NOPARALYZE);
            when 6: breath = "sleeping gas";
                    turn_on(*tp, NOSLEEP);
            when 7: breath = "slow gas";
                    turn_on(*tp, NOSLOW);
            when 8: breath = "confusion gas";
                    turn_on(*tp, ISCLEAR);
            when 9: breath = "fear gas";
                    turn_on(*tp, NOFEAR);
        }
    }

    /* Or can it breathe acid? */
    else if (on(*tp, CANBACID)) {
        turn_off(*tp, CANBACID);
        breath = "acid";
    }

    /* Or can it breathe fire */
    else if (on(*tp, CANBFIRE)) {
        turn_off(*tp, CANBFIRE);
        breath = "flame";
    }

    /* Or can it breathe electricity? */
    else if (on(*tp, CANBBOLT)) {
        turn_off(*tp, CANBBOLT);
        breath = "lightning bolt";
    }

    /* Or can it breathe gas? */
    else if (on(*tp, CANBGAS)) {
        turn_off(*tp, CANBGAS);
        breath = "chlorine gas";
    }

    /* Or can it breathe ice? */
    else if (on(*tp, CANBICE)) {
        turn_off(*tp, CANBICE);
        breath = "ice";
    }

    else if (on(*tp, CANBPGAS)) {
        turn_off(*tp, CANBPGAS);
        breath = "nerve gas";
    }

    /* can it breathe sleeping gas */
    else if (on(*tp, CANBSGAS)) {
        turn_off(*tp, CANBSGAS);
        breath = "sleeping gas";
    }

    /* can it breathe slow gas */
    else if (on(*tp, CANBSLGAS)) {
        turn_off(*tp, CANBSLGAS);
        breath = "slow gas";
    }

    /* can it breathe confusion gas */
    else if (on(*tp, CANBCGAS)) {
        turn_off(*tp, CANBCGAS);
        breath = "confusion gas";
    }

    /* can it breathe fear gas */
    else {
        turn_off(*tp, CANBFGAS);
        breath = "fear gas";
    }

    /* Now breathe */
    shoot_bolt(tp, tp->t_pos, tp->t_newpos, FALSE, 
                    tp->t_index, breath, damage);

    running = FALSE;
    if (fight_flush) flushinp();
}

/*
 * m_select:
 *      Select an action for the monster.
 */

m_select(th, flee)
register struct thing *th;
register bool flee; /* True if running away or player is inaccessible in wall */
{
    register struct room *rer, *ree;    /* room of chaser, room of chasee */
    int dist = INT_MIN;
    int mindist = INT_MAX, maxdist = INT_MIN;
    bool rundoor;                       /* TRUE means run to a door */
    char sch;
    coord *last_door=0,                 /* Door we just came from */
           this;                        /* Temporary destination for chaser */

    rer = roomin(&th->t_pos);   /* Find room of chaser */
    ree = roomin(th->t_dest);   /* Find room of chasee */

    /* First see if we want to use an ability or weapon */
    if (m_use_it(th, flee, rer, ree)) return;

    /*
     * We don't count monsters on doors as inside rooms here because when
     * a monster is in a room and the player is not in that room, the
     * monster looks for the best door out.  If we counted doors as part
     * of the room, the monster would already be on the best door out;
     * so he would never move.
     */
    if ((sch = mvwinch(stdscr, th->t_pos.y, th->t_pos.x)) == DOOR ||
        sch == SECRETDOOR || sch == PASSAGE) {
        rer = NULL;
    }
    this = *th->t_dest;

    /*
     * If we are in a room heading for the player and the player is not
     * in the room with us, we run to the "best" door.
     * If we are in a room fleeing from the player, then we run to the
     * "best" door if he IS in the same room.
     *
     * Note:  We don't bother with doors in mazes or if we can walk
     * through walls.
     */
    if (rer != NULL && levtype != MAZELEV && off(*th, CANINWALL)) {
        if (flee) rundoor = (rer == ree);
        else rundoor = (rer != ree);
    }
    else rundoor = FALSE;

    if (rundoor) {
        register struct linked_list *exitptr;   /* For looping through exits */
        coord *exit,                            /* A particular door */
              *entrance;                        /* Place just inside doorway */
        int exity, exitx;                       /* Door's coordinates */
        char dch='\0';                          /* Door character */

        if ((th->t_doorgoal.x != -1) && (th->t_doorgoal.y != -1))
            dch = mvwinch(stdscr, th->t_doorgoal.y, th->t_doorgoal.x);
            
        /* Do we have a valid goal? */
        if ((dch == PASSAGE || dch == DOOR) &&  /* A real door */
            (!flee || !ce(th->t_doorgoal, *th->t_dest))) { /* Prey should not
                                                             * be at door if
                                                             * we are running
                                                             * away
                                                             */
            /* Make sure the player is not in the doorway, either */
            entrance = doorway(rer, &th->t_doorgoal);
            if (!flee || entrance == NULL || !ce(*entrance, *th->t_dest)) {
                this = th->t_doorgoal;
                dist = 0;       /* Indicate that we have our door */
            }
        }

        /* Go through all the doors */
        else for (exitptr = rer->r_exit; exitptr; exitptr = next(exitptr)) {
            exit = DOORPTR(exitptr);
            exity = exit->y;
            exitx = exit->x;

            /* Make sure it is a real door */
            dch = mvwinch(stdscr, exity, exitx);
            if (dch == PASSAGE || dch == DOOR) {
                /* Don't count a door if we are fleeing from someone and
                 * he is standing on it.  Also, don't count it if he is
                 * standing in the doorway.
                 */
                if (flee) {
                    if (ce(*exit, *th->t_dest)) continue;

                    entrance = doorway(rer, exit);
                    if (entrance != NULL && ce(*entrance, *th->t_dest))
                        continue;
                }
                
                /* Were we just on this door? */
                if (ce(*exit, th->t_oldpos)) last_door = exit;

                else {
                    dist = DISTANCE(th->t_dest->y, th->t_dest->x, exity, exitx);

                    /* If fleeing, we want to maximize distance from door to
                     * what we flee, and minimize distance from door to us.
                     */
                    if (flee)
                       dist -= DISTANCE(th->t_pos.y, th->t_pos.x, exity, exitx);

                    /* Maximize distance if fleeing, otherwise minimize it */
                    if ((flee && (dist > maxdist)) ||
                        (!flee && (dist < mindist))) {
                        th->t_doorgoal = *exit;  /* Use this door */
                        this = *exit;
                        mindist = maxdist = dist;
                    }
                }
            }
        }

        /* Could we not find a door? */
        if (dist == INT_MIN) {
            /* If we were on a door, go ahead and use it */
            if (last_door) {
                th->t_doorgoal = *last_door;
                this = th->t_oldpos;
                dist = 0;       /* Indicate that we found a door */
            }
            else th->t_doorgoal.x = th->t_doorgoal.y = -1; /* No more door goal */
        }

        /* Indicate that we do not want to flee from the door */
        if (dist != INT_MIN) flee = FALSE;
    }
    else th->t_doorgoal.x = th->t_doorgoal.y = -1;    /* Not going to any door */

    /* Now select someplace to go and start the action */
    chase(th, &this, rer, ree, flee);
}

/*
 * m_sonic:
 *      The monster is sounding a sonic blast.
 */

m_sonic(tp)
register struct thing *tp;
{
    register int damage;
    struct object blast =
    {
        MISSILE, {0, 0}, 0, "", "150" , NULL, 0, 0, 0, 0
    };

    turn_off(*tp, CANSONIC);
    turn_off(*tp, CANSURPRISE);
    do_motion(&blast, tp->t_newpos.y, tp->t_newpos.x, tp);
    damage = rnd(61)+40;
    if (save(VS_BREATH, &player, -3))
        damage /= 2;
    msg ("%s's ultra-sonic blast hits you", prname(monster_name(tp), TRUE));
    if ((pstats.s_hpt -= damage) <= 0) {
    pstats.s_hpt = -1;
        death(tp->t_index);
    }
    running = FALSE;
    if (fight_flush) flushinp();
    dsrpt_player();
}

/*
 * m_spell:
 *      The monster casts a spell.  Currently this is limited to
 *      magic missile.
 */
m_spell(tp)
register struct thing *tp;
{
    struct object missile =
    {
        MISSILE, {0, 0}, 0, "", "0d4 " , NULL, 0, WS_MISSILE, 100, 1
    };

    sprintf(missile.o_hurldmg, "%dd4", tp->t_stats.s_lvl);
    do_motion(&missile, tp->t_newpos.y, tp->t_newpos.x, tp);
    hit_monster(unc(missile.o_pos), &missile, tp);
    turn_off(*tp, CANMISSILE);
    turn_off(*tp, CANSURPRISE);

    running = FALSE;
    if (fight_flush) flushinp();
}

/*
 * m_summon:
 *      Summon aid.
 */

m_summon(tp)
register struct thing *tp;
{
    register char *helpname, *mname;
    int fail, numsum;
    register int which, i;

    /* Let's make sure our prey is still here */
    if (!cansee(unc(tp->t_pos)) || fallpos(&hero, FALSE, 2) == NULL) return;

    /*
     * Non-uniques can only summon once.  Uniques get fewer
     * creatures with each successive summoning. Also, the
     * probability of summoning goes down
     */
    if (off(*tp, ISUNIQUE))
            turn_off(*tp, CANSUMMON);

    turn_off(*tp, CANSURPRISE);
    mname = monster_name(tp);
    helpname = monsters[tp->t_index].m_typesum;
    which = findmindex(helpname);

    if ((off(*tp, ISINVIS)     || on(player, CANSEE)) &&
        (off(*tp, ISSHADOW)    || on(player, CANSEE)) &&
        (off(*tp, CANSURPRISE) || ISWEARING(R_ALERT))) {
        if (monsters[which].m_normal == FALSE) { /* genocided? */
            msg("%s appears dismayed", prname(mname, TRUE));
            monsters[tp->t_index].m_numsum = 0;
        }
        else {
            msg("%s summons %ss for help", prname(mname, TRUE), helpname);
        }
    }
    else {
        if (monsters[which].m_normal == FALSE) /* genocided? */
            monsters[tp->t_index].m_numsum = 0;
        else {
            msg("%ss seem to appear from nowhere!", helpname);
        }
    }
    numsum = monsters[tp->t_index].m_numsum;
    if (numsum && on(*tp, ISUNIQUE)) {   /* UNIQUEs summon less each time */
        monsters[tp->t_index].m_numsum--; 
        tp->t_summon *= 2; /* cut probability in half */
    }

    /*
     * try to make all the creatures around player but remember
     * if unsuccessful
     */
    for (i=0, fail=0; i<numsum; i++) {
         if (!creat_mons(&player, which, FALSE))
             fail++;    /* remember the failures */
    }

    /*
     * try once again to make the buggers
     */
    for (i=0; i<fail; i++)
         creat_mons(tp, which, FALSE);
    
    /* Now let the poor fellow see all the trouble */
    light(&hero);
    turn_on(*tp, HASSUMMONED);
}

/*
 * m_use_it:
 *      See if the monster (tp) has anything useful it can do
 *      (ie. an ability or a weapon) other than just move.
 */

bool
m_use_it(tp, flee, rer, ree)
register struct thing *tp;
bool flee;
register struct room *rer, *ree;
{
    int dist;
    register coord *ee = tp->t_dest, *er = &tp->t_pos; 
    coord *shoot_dir = NULL;
    coord straight_dir;
    int   straight_shot = FALSE;
    struct thing *prey;
    bool dest_player;   /* Are we after the player? */

    /*
     * If we are fleeing, there's a chance, depending on our
     * intelligence, that we'll just run in terror.
     */
    if (flee && rnd(25) >= tp->t_stats.s_intel) return(FALSE);

    /*
     * Make sure that we have a living destination, and record whether
     * it is the player.
     */
    if (ee != NULL) {
        if (ce(*ee, hero)) {
            dest_player = TRUE;
            prey = &player;
        }
        else {
            struct linked_list *item;

            dest_player = FALSE;

            /* What is the monster we're chasing? */
            item = find_mons(ee->y, ee->x);
            if (item != NULL) prey = THINGPTR(item);
            else return(FALSE);
        }
    }
    else return(FALSE);

    /*
     * If we are friendly to the hero, we don't do anything.
     */
    if (on(*tp, ISFRIENDLY) && dest_player) return(FALSE);

    /*
     * Also, for now, if our prey is in a wall, we won't do
     * anything.  The prey must be in the same room as we are OR
     * we must have a straight shot at him.  Note that
     * shoot_dir must get set before rer is checked so
     * that we get a valid value.
     */
 
    if (can_shoot(er, ee, &straight_dir) == 0)
        shoot_dir = &straight_dir;
    else
        shoot_dir = NULL;

    if (on(*prey, ISINWALL) ||
        ( (shoot_dir == NULL) && (rer == NULL || rer != ree)))
        return(FALSE);

    /*
     * If we can't see the prey then forget it
     */
    if (on(*prey, ISINVIS) && off(*tp, CANSEE))
        return(FALSE);

    /* How far are we from our prey? */
    dist = DISTANCE(er->y, er->x, ee->y, ee->x);

    /* 
     * Shall we summon aid so we don't have to get our hands dirty? 
     * For now, we will only summon aid against the player.
     * We'll wait until he's within 2 dots of a missile length.
     */
    if (on(*tp, CANSUMMON) && dest_player                       &&
        dist < (BOLT_LENGTH+2)*(BOLT_LENGTH+2)                  &&
        rnd(tp->t_summon) < tp->t_stats.s_lvl                   &&
        monsters[tp->t_index].m_numsum > 0                      &&
        fallpos(&hero, FALSE, 2) != NULL) {
        tp->t_action = A_SUMMON;        /* We're going to summon help */
        tp->t_no_move = movement(tp); /* It takes time! */
        return(TRUE);
    }

    /*
     * If the creature can cast a slow spell and if the prey is within
     * 2 dots of a missile fire, then see whether we will cast it.
     * if next to player, lessen chance because we don't like being
     * disrupted
     */
    if (on(*tp, CANSLOW) && dest_player                 && 
        dist < (BOLT_LENGTH+5)*(BOLT_LENGTH+5)          &&
        rnd(100) < (dist > 3 ? tp->t_cast : tp->t_cast/2)) {
            tp->t_action = A_SLOW;              /* We're going to slow him */
            tp->t_no_move = 3 * movement(tp);   /* Takes time! */
            debug("casting slow spell!");
            return(TRUE);
    }

    /*
     * If we have a special magic item, we might use it.  We will restrict
     * this options to uniques with relics and creatures with wands for now.  
     * Also check for the quartermaster. Don't want him shooting wands....
     */
    if ((on(*tp, ISUNIQUE) || on(*tp, CARRYSTICK)) && 
        off(*tp, CANSELL) && dest_player           &&
        m_use_pack(tp, ee, dist, shoot_dir)) {
            return(TRUE);
    }

    /* From now on, we must have a direct shot at the prey */
    if (!straight_shot) return(FALSE);

    /* We may use a sonic blast if we can, only on the player */
    if (on(*tp, CANSONIC)               && 
        dest_player                     &&
        (dist < BOLT_LENGTH*2)          &&
        (rnd(100) < tp->t_breathe)) {
        tp->t_newpos = *shoot_dir;      /* Save the direction */
        tp->t_action = A_SONIC; /* We're going to sonic blast */
        tp->t_no_move = 2 * movement(tp); /* Takes 2 movement periods */
    }

    /* If we can breathe, we may do so */
    else if (on(*tp, CANBREATHE)                &&
         (dist < BOLT_LENGTH*BOLT_LENGTH)       &&
         (rnd(100) < tp->t_breathe)) {
            tp->t_newpos = *shoot_dir;  /* Save the direction */
            tp->t_action = A_BREATHE;   /* We're going to breathe */
            tp->t_no_move = movement(tp); /* It takes 1 movement period */
    }

    /* 
     * We may shoot missiles if we can 
     * if next to player, lessen chance so we don't get disrupted as often
     */
    else if (on(*tp,CANMISSILE) && 
             rnd(100) < (dist > 3 ? tp->t_cast : tp->t_cast/2)){
            tp->t_newpos = *shoot_dir;  /* Save the direction */
            tp->t_action = A_MISSILE;   /* We're going to shoot MM's */
            tp->t_no_move = 3 * movement(tp); /* Takes time! */
    }

    /* 
     * If we can shoot or throw something, we might do so.
     * If next to player, then forget it
     */
    else if ((on(*tp,CANSHOOT)          || on(*tp,CARRYWEAPON) || 
              on(*tp,CARRYDAGGER)       || on(*tp, CARRYAXE))           &&
              dist > 3                                                  &&
              off(*tp, CANSELL)                                         &&
             (get_hurl(tp) != NULL)) {
            tp->t_newpos = *shoot_dir;  /* Save the direction */
            tp->t_action = A_THROW;     /* We're going to throw something */
            tp->t_using = get_hurl(tp);       /* Save our weapon */
            tp->t_no_move = 2 * movement(tp); /* Takes 2 movement periods */
    }
    
    /* We couldn't find anything to do */
    else return(FALSE);

    return(TRUE);

}

reap()
{
    _t_free_list(&rlist);
}

/*
 * runners:
 *      Make all the awake monsters try to do something.
 */

runners(segments)
int segments;    /* Number of segments since last called */
{
    register struct linked_list *item;
    register struct thing *tp = NULL;
    register int min_time = 20;     /* Minimum time until a monster can act */

    /*
     * loop thru the list of running (wandering) monsters and see what
     * each one will do this time. 
     *
     * Note: the special case that one of this buggers kills another.
     *       if this happens than we have to see if the monster killed
     *       himself or someone else. In case its himself we have to get next
     *       one immediately. If it wasn't we have to get next one at very
     *       end in case he killed the next one.
     */
    for (item = mlist; item != NULL; item = item->l_next)
    {
        tp = THINGPTR(item);
        turn_on(*tp, NEEDSTOACT);
    }

    for(;;)
    {
        for (item = mlist; item != NULL; item = item->l_next)
        {
            tp = THINGPTR(item);

            if (on(*tp, NEEDSTOACT))
                break;
        }

        if (item == NULL)
            break;

        turn_off(*tp, NEEDSTOACT);

        /* If we are not awake, just skip us */

        if (off(*tp, ISRUN) && off(*tp, ISHELD))
            continue;

        /* See if it's our turn */

        tp->t_no_move -= segments;

        if (tp->t_no_move > 0)
        {
            if (tp->t_no_move < min_time) min_time = tp->t_no_move;
                continue;
        }

        /* If we were frozen, we're moving now */

        if (tp->t_action == A_FREEZE)
            tp->t_action = A_NIL;

        if (on(*tp, ISHELD))
        {
            /* Make sure the action and using are nil */

            tp->t_action = A_NIL;
            tp->t_using = NULL;

            /* Can we break free? */

            if (rnd(tp->t_stats.s_lvl) > 11)
            {
                turn_off(*tp, ISHELD);

                runto(tp, &hero);

                if (cansee(tp->t_pos.y, tp->t_pos.x))
                    msg("%s breaks free from the hold spell", 
                        prname(monster_name(tp), TRUE));
            }
            else /* Too bad -- try again later */
                tp->t_no_move = movement(tp);
        }

        /* Heal the creature if it's not in the middle of some action */

        if (tp->t_action == A_NIL)
           doctor(tp);

        while (off(*tp, ISELSEWHERE) &&
               off(*tp, ISDEAD) &&
               tp->t_no_move <= 0 &&
               off(*tp, ISHELD) &&
               on(*tp, ISRUN)       )
        {
            /* Let's act (or choose an action if t_action = A_NIL) */

            m_act(tp);
        }

        if ( off(*tp,ISELSEWHERE) && off(*tp,ISDEAD) )
        {
            if (tp->t_no_move < min_time)
               min_time = tp->t_no_move;

            if (tp->t_quiet < 0)
               tp->t_quiet = 0;
        }
    }

    return(min_time);
}

/*
 * See if a monster has some magic it can use.  Return TRUE if so.
 * Only care about relics and wands for now.
 */
bool
m_use_pack(monster, defend_pos, dist, shoot_dir)
register struct thing *monster;
coord *defend_pos;
register int dist;
register coord *shoot_dir;
{
    register struct object *obj;
    register struct linked_list *pitem, *relic, *stick;
    register int units = -1;

    relic = stick = NULL;

    for (pitem=monster->t_pack; pitem; pitem=next(pitem)) {
        obj = OBJPTR(pitem);
        if (obj->o_flags & ISCURSED) continue;
        if (obj->o_type == RELIC) {
            switch (obj->o_which) {
                case MING_STAFF:
                    if (shoot_dir != NULL) {
                        units = 2;      /* Use 2 time units */
                        relic = pitem;
                    }

                when EMORI_CLOAK:
                    if (obj->o_charges != 0     && 
                        shoot_dir != NULL) {
                            units = 2;  /* Use 2 time units */
                            relic = pitem;
                    }

                when ASMO_ROD:
                    /* The bolt must be able to reach the defendant */
                    if (shoot_dir != NULL                       && 
                        dist < BOLT_LENGTH * BOLT_LENGTH) {
                        units = 2;      /* Use 2 time units */
                        relic = pitem;
                    }

                when BRIAN_MANDOLIN:
                    /* The defendant must be the player and within 4 spaces */
                    if (ce(*defend_pos, hero)           && 
                        dist < 25                       &&
                        player.t_action != A_FREEZE) {
                        units = 4;
                        relic = pitem;
                    }

                when GERYON_HORN:
                    /* The defendant must be the player and within 5 spaces */
                    if (ce(*defend_pos, hero)                                &&
                        dist < 25                                            &&
                        (off(player,ISFLEE)|| player.t_dest!=&monster->t_pos)) {
                        units = 3;
                        relic = pitem;
                    }
            }
        }
        if (obj->o_type == STICK) {
            if (obj->o_charges < 1) continue;
            switch(obj->o_which) {
                case WS_ELECT:
                case WS_FIRE:
                case WS_COLD:
                    /* The bolt must be able to reach the defendant */
                    if (shoot_dir != NULL                       && 
                        dist < BOLT_LENGTH * BOLT_LENGTH) {
                            units = 3;
                            stick = pitem;
                    }

                when WS_MISSILE:
                case WS_SLOW_M:
                case WS_CONFMON:
                case WS_PARALYZE:
                case WS_MDEG:
                case WS_FEAR:
                    if (shoot_dir != NULL) {
                        units = 3;
                        stick = pitem;
                    }
                
                otherwise:
                    break;
            }
        }
    }

    /* use relics in preference to all others */
    if (relic) debug("chance to use relic = %d%%", monster->t_artifact);
    if (stick) debug("chance to use stick = %d%%", monster->t_wand);
    if (relic && rnd(100) < monster->t_artifact)  {
        monster->t_action = A_USERELIC;
        pitem = relic;
    }
    else if (stick && rnd(100) < monster->t_wand) {
        /*
         * see if the monster will use the wand 
         */
        pitem = stick;
        monster->t_action = A_USEWAND;
    }
    else {
        return(FALSE);
    }

    monster->t_no_move = units * movement(monster);
    monster->t_using = pitem;
    monster->t_newpos = *shoot_dir;
    return(TRUE);
}

