/*
    bolt.c  -  functions shooting an object across the room
        
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
 * shoot_bolt fires a bolt from the given starting point in the
 *            given direction
 */

shoot_bolt(shooter, start, dir, get_points, reason, name, damage)
struct thing *shooter;
coord start, dir;
bool get_points;
short reason;
char *name;
int damage;
{
    unsigned char dirch = 0, ch;
    bool used, change, see_him;
    short y, x, bounces;
    coord pos;
    struct linked_list *target=NULL;
    struct {
        coord place;
        char oldch;
    } spotpos[BOLT_LENGTH];

    switch (dir.y + dir.x) {
        case 0: dirch = '/';
        when 1: case -1: dirch = (dir.y == 0 ? '-' : '|');
        when 2: case -2: dirch = '\\';
    }
    pos.y = start.y + dir.y;
    pos.x = start.x + dir.x;
    used = FALSE;
    change = FALSE;

    bounces = 0;        /* No bounces yet */
    nofont(cw);
    for (y = 0; y < BOLT_LENGTH && !used; y++) {
        ch = winat(pos.y, pos.x);
        spotpos[y].place = pos;
        spotpos[y].oldch = mvwinch(cw, pos.y, pos.x);

        /* Are we at hero? */
        if (ce(pos, hero)) goto at_hero;

        switch (ch) {
            case SECRETDOOR:
            case VERTWALL:
            case HORZWALL:
            case ' ':
                if (dirch == '-' || dirch == '|') {
                    dir.y = -dir.y;
                    dir.x = -dir.x;
                }
                else {
                    unsigned char chx = mvinch(pos.y-dir.y, pos.x),
                         chy = mvinch(pos.y, pos.x-dir.x);
                    bool anychange = FALSE; /* Did we change anthing */

                    if (chy == WALL || chy == SECRETDOOR ||
                        chy == HORZWALL || chy == VERTWALL) {
                        dir.y = -dir.y;
                        change ^= TRUE; /* Change at least one direction */
                        anychange = TRUE;
                    }
                    if (chx == WALL || chx == SECRETDOOR ||
                        chx == HORZWALL || chx == VERTWALL) {
                        dir.x = -dir.x;
                        change ^= TRUE; /* Change at least one direction */
                        anychange = TRUE;
                    }

                    /* If we didn't make any change, make both changes */
                    if (!anychange) {
                        dir.x = -dir.x;
                        dir.y = -dir.y;
                    }
                }

                /* Do we change how the bolt looks? */
                if (change) {
                    change = FALSE;
                    if (dirch == '\\') dirch = '/';
                    else if (dirch == '/') dirch = '\\';
                }

                y--;    /* The bounce doesn't count as using up the bolt */

                /* Make sure we aren't in an infinite bounce */
                if (++bounces > BOLT_LENGTH) used = TRUE;
                msg("The %s bounces", name);
                break;
            default:
                if (isalpha(ch)) {
                    register struct linked_list *item;
                    struct thing *tp;
                    register char *mname;
                    bool see_monster = cansee(pos.y, pos.x);

                    item = find_mons(unc(pos));
                    assert(item != NULL);
                    tp = THINGPTR(item);
                    mname = monster_name(tp);

                    /*
                     * If our prey shot this, let's record the fact that
                     * he can shoot, regardless of whether he hits us.
                     */
                    if (tp->t_dest != NULL && ce(*tp->t_dest, shooter->t_pos)) 
                        tp->t_wasshot = TRUE;

                    if (!save(VS_BREATH, tp, -(shooter->t_stats.s_lvl/10))) {
                        if (see_monster) {
                            if (on(*tp, ISDISGUISE) &&
                                (tp->t_type != tp->t_disguise)) {
                                msg("Wait! That's a %s!", mname);
                                turn_off(*tp, ISDISGUISE);
                            }

                            turn_off(*tp, CANSURPRISE);
                            msg("The %s hits %s", name, prname(mname, FALSE));
                        }

                        /* Should we start to chase the shooter? */
                        if (shooter != &player                  &&
                            shooter != tp                       &&
                            shooter->t_index != tp->t_index     &&
                            (tp->t_dest == NULL || rnd(100) < 25)) {
                            /*
                             * If we're intelligent enough to realize that this
                             * is a friendly monster, we will attack the hero
                             * instead.
                             */
                            if (on(*shooter, ISFRIENDLY) &&
                                 roll(3,6) < tp->t_stats.s_intel)
                                 runto(tp, &hero);

                            /* Otherwise, let's chase the monster */
                            else runto(tp, &shooter->t_pos);
                        }
                        else if (shooter == &player) {
                            runto(tp, &hero);

                            /*
                             * If the player shot a charmed monster, it may
                             * not like being shot at.
                             */
                            if (on(*tp, ISCHARMED) && save(VS_MAGIC, tp, 0)) {
                                msg("The eyes of %s turn clear.", 
                                    prname(mname, FALSE));
                                turn_off(*tp, ISCHARMED);
                                mname = monster_name(tp);
                            }
                        }

                        /*
                         * Let the defender know that the attacker has
                         * missiles!
                         */
                        if (ce(*tp->t_dest, shooter->t_pos))
                            tp->t_wasshot = TRUE;

                        used = TRUE;

                        /* Hit the monster -- does it do anything? */
                        if ((EQUAL(name,"ice")           && on(*tp, NOCOLD))  ||
                            (EQUAL(name,"flame")         && on(*tp, NOFIRE))  ||
                            (EQUAL(name,"acid")          && on(*tp, NOACID))  ||
                            (EQUAL(name,"lightning bolt")&& on(*tp,NOBOLT))   ||
                            (EQUAL(name,"nerve gas")     &&on(*tp,NOPARALYZE))||
                            (EQUAL(name,"sleeping gas")  &&
                             (on(*tp, NOSLEEP) || on(*tp, ISUNDEAD)))         ||
                            (EQUAL(name,"slow gas")      && on(*tp,NOSLOW))   ||
                            (EQUAL(name,"fear gas")      && on(*tp,NOFEAR))   ||
                            (EQUAL(name,"confusion gas") && on(*tp,ISCLEAR))  ||
                            (EQUAL(name,"chlorine gas")  && on(*tp,NOGAS))) {
                            if (see_monster)
                                msg("The %s has no effect on %s.",
                                        name, prname(mname, FALSE));
                        }

                        else {
                            see_him = !invisible(tp);
                           
                            /* Did a spell get disrupted? */
                            dsrpt_monster(tp, FALSE, see_him);

                            /* 
                             * Check for gas with special effects 
                             */
                            if (EQUAL(name, "nerve gas")) {
                                tp->t_no_move = movement(tp) * FREEZETIME;
                                tp->t_action = A_FREEZE;
                            }
                            else if (EQUAL(name, "sleeping gas")) {
                                tp->t_no_move = movement(tp) * SLEEPTIME;
                                tp->t_action = A_FREEZE;
                            }
                            else if (EQUAL(name, "slow gas")) {
                                if (on(*tp, ISHASTE))
                                    turn_off(*tp, ISHASTE);
                                else
                                    turn_on(*tp, ISSLOW);
                            }
                            else if (EQUAL(name, "fear gas")) {
                                turn_on(*tp, ISFLEE);
                                tp->t_dest = &hero;

                                /* It is okay to turn tail */
                                tp->t_oldpos = tp->t_pos;
                            }
                            else if (EQUAL(name, "confusion gas")) {
                                turn_on(*tp, ISHUH);
                                tp->t_dest = &hero;
                            }
                            else if ((EQUAL(name, "lightning bolt")) &&
                                     on(*tp, BOLTDIVIDE)) {
                                    if (creat_mons(tp, tp->t_index, FALSE)) {
                                      if (see_monster)
                                       msg("The %s divides %s.",
                                           name,prname(mname, FALSE));
                                      light(&hero);
                                    }
                                    else if (see_monster)
                                        msg("The %s has no effect on %s.",
                                            name, prname(mname, FALSE));
                            }
                            else {
                                if (save(VS_BREATH, tp,
                                         -(shooter->t_stats.s_lvl/10)))
                                    damage /= 2;

                                /* The poor fellow got killed! */
                                if ((tp->t_stats.s_hpt -= damage) <= 0) {
                                    if (see_monster)
                                        msg("The %s kills %s", 
                                            name, prname(mname, FALSE));
                                    else
                                     msg("You hear a faint groan in the distance");
                                    /*
                                     * Instead of calling killed() here, we
                                     * will record that the monster was killed
                                     * and call it at the end of the routine,
                                     * after we restore what was under the bolt.
                                     * We have to do this because in the case
                                     * of a bolt that first misses the monster
                                     * and then gets it on the bounce.  If we
                                     * call killed here, the 'missed' space in
                                     * spotpos puts the monster back on the
                                     * screen
                                     */
                                    target = item;
                                }
                                else {  /* Not dead, so just scream */
                                     if (!see_monster)
                                       msg("You hear a scream in the distance");
                                }
                            }
                        }
                    }
                    else if (isalpha(show(pos.y, pos.x))) {
                        if (see_monster) {
                            if (terse)
                                msg("%s misses", name);
                            else
                                msg("The %s whizzes past %s",
                                            name, prname(mname, FALSE));
                        }
                        if (get_points) runto(tp, &hero);
                    }
                }
                else if (pos.y == hero.y && pos.x == hero.x) {
at_hero:            if (!save(VS_BREATH, &player,
                                -(shooter->t_stats.s_lvl/10))){
                        if (terse)
                            msg("The %s hits you", name);
                        else
                            msg("You are hit by the %s", name);
                        used = TRUE;

                        /* 
                         * The Amulet of Yendor protects against all "breath" 
                         *
                         * The following two if statements could be combined 
                         * into one, but it makes the compiler barf, so split 
                         * it up
                         */
                        if (cur_relic[YENDOR_AMULET]                        ||
                            (EQUAL(name,"chlorine gas")&&on(player, NOGAS)) ||
                            (EQUAL(name,"acid")&&on(player, NOACID))        ||
                            (EQUAL(name,"sleeping gas")&&ISWEARING(R_ALERT))){
                             msg("The %s has no effect", name);
                        }
                        else if((EQUAL(name, "flame") && on(player, NOFIRE)) ||
                                (EQUAL(name, "ice")   && on(player, NOCOLD)) ||
                                (EQUAL(name,"lightning bolt")&& 
                                                         on(player,NOBOLT))  ||
                                (EQUAL(name,"fear gas")&&ISWEARING(R_HEROISM))){
                             msg("The %s has no effect", name);
                        }

                        else {
                            dsrpt_player();

                            /* 
                             * Check for gas with special effects 
                             */
                            if (EQUAL(name, "nerve gas")) {
                                msg("The nerve gas paralyzes you.");
                                player.t_no_move +=
                                        movement(&player) * FREEZETIME;
                                player.t_action = A_FREEZE;
                            }
                            else if (EQUAL(name, "sleeping gas")) {
                                msg("The sleeping gas puts you to sleep.");
                                player.t_no_move +=
                                        movement(&player) * SLEEPTIME;
                                player.t_action = A_FREEZE;
                            }
                            else if (EQUAL(name, "confusion gas")) {
                                if (off(player, ISCLEAR)) {
                                    if (on(player, ISHUH))
                                        lengthen(unconfuse,
                                                 rnd(20)+HUHDURATION);
                                    else {
                                        turn_on(player, ISHUH);
                                        fuse(unconfuse, (VOID *)NULL,
                                             rnd(20)+HUHDURATION, AFTER);
                                        msg("The confusion gas has confused you.");
                                    }
                                }
                                else msg("You feel dizzy for a moment, but it quickly passes.");
                            }
                            else if (EQUAL(name, "slow gas")) {
                                add_slow();
                            }
                            else if (EQUAL(name, "fear gas")) {
                                turn_on(player, ISFLEE);
                                player.t_dest = &shooter->t_pos;
                                msg("The fear gas terrifies you.");
                            }
                            else {
                                if (EQUAL(name, "acid")                 &&
                                    cur_armor != NULL                   &&
                                    !(cur_armor->o_flags & ISPROT)      &&
                                    !save(VS_BREATH, &player, -2)       &&
                                    cur_armor->o_ac < pstats.s_arm+1) {
                                       msg("Your armor corrodes from the acid");
                                       cur_armor->o_ac++;
                                }
                                if (save(VS_BREATH, &player,
                                         -(shooter->t_stats.s_lvl/10))  &&
                                         off(player, NOACID))
                                    damage /= 2;
                                if ((pstats.s_hpt -= damage) <= 0) 
                                    death(reason);
                            }
                        }
                    }
                    else
                        msg("The %s whizzes by you", name);
                }

                mvwaddch(cw, pos.y, pos.x, dirch);
                draw(cw);
        }

        pos.y += dir.y;
        pos.x += dir.x;
    }

    /* Restore what was under the bolt */
    newfont(cw);
    for (x = y - 1; x >= 0; x--)
        mvwaddch(cw, spotpos[x].place.y, spotpos[x].place.x, spotpos[x].oldch);

    /* If we killed something, do so now.  This will also blank the monster. */
    if (target) killed(target, FALSE, get_points, TRUE);
    return;
}

