/*
    util.c  -  all sorts of miscellaneous routines

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
 * this routine computes the players current AC without dex bonus's
 */

int 
ac_compute(ignoremetal)
bool ignoremetal;
{
    register int ac;

    ac = pstats.s_arm; /* base armor of "skin" */
    if (cur_armor) {
        if (!ignoremetal || 
             (cur_armor->o_which != LEATHER             && 
              cur_armor->o_which != STUDDED_LEATHER     && 
              cur_armor->o_which != PADDED_ARMOR))
                ac -= (10 - cur_armor->o_ac);
    }
    if (player.t_ctype == C_MONK)
        ac -= pstats.s_lvl * 3 / 5;
    ac -= ring_value(R_PROTECT);
    if (cur_misc[WEAR_BRACERS] != NULL)
        ac -= cur_misc[WEAR_BRACERS]->o_ac;
    if (cur_misc[WEAR_CLOAK] != NULL)
        ac -= cur_misc[WEAR_CLOAK]->o_ac;

    /* If player has the cloak, must be wearing it */
    if (cur_relic[EMORI_CLOAK]) ac -= 15;

    if (ac > 25)
        ac = 25;
    return(ac);
}

/*
 * aggravate:
 *      aggravate all the monsters on this level
 */

aggravate(do_uniques, do_good)
bool do_uniques, do_good;
{
    register struct linked_list *mi;
    register struct thing *thingptr;

    for (mi = mlist; mi != NULL; mi = next(mi)) {
        thingptr = THINGPTR(mi);
        if (do_good == FALSE && off(*thingptr, ISMEAN)) continue;
        if (do_uniques || off(*thingptr, ISUNIQUE)) runto(thingptr, &hero);
    }
}

/*
 * cansee:
 *      returns true if the hero can see a certain coordinate.
 */

cansee(y, x)
register int y, x;
{
    register struct room *rer;
    register int radius;
    coord tp;

    if (on(player, ISBLIND))
        return FALSE;

    tp.y = y;
    tp.x = x;
    rer = roomin(&tp);

    /* How far can we see? */
    if (levtype == OUTSIDE) {
        if (daytime) radius = 36;
        else if (lit_room(rer)) radius = 9;
        else radius = 3;
    }
    else radius = 3;

    /*
     * We can only see if the hero in the same room as
     * the coordinate and the room is lit or if it is close.
     */
    return ((rer != NULL && 
             levtype != OUTSIDE &&
             (levtype != MAZELEV ||     /* Maze level needs direct line */
              maze_view(tp.y, tp.x)) &&
             rer == roomin(&hero) &&
             lit_room(rer)) ||
            DISTANCE(y, x, hero.y, hero.x) < radius);
}

/*
 * check_level:
 *      Check to see if the guy has gone up a level.
 *
 *      Return points needed to obtain next level.
 *
 * These are certain beginning experience levels for all players.
 * All further experience levels are computed by muliplying by 2
 * up through MAXDOUBLE. Then the cap is added in to compute
 * further levels
 */

long
check_level()
{
    register int i, j, add = 0;
    register unsigned long exp;
    long retval;        /* Return value */
    int nsides;

    pstats.s_lvl -= pstats.s_lvladj; /* correct for level adjustment */
    /* See if we are past the doubling stage */
    exp = char_class[player.t_ctype].cap;
    if (pstats.s_exp >= exp) {
        i = pstats.s_exp/exp;   /* First get amount above doubling area */
        retval = exp + i * exp; /* Compute next higher boundary */
        i += MAXDOUBLE; /* Add in the previous doubled levels */
    }
    else {
        i = 0;
        exp = char_class[player.t_ctype].start_exp;
        while (exp <= pstats.s_exp) {
            i++;
            exp <<= 1;
        }
        retval = exp;
    }
    if (++i > pstats.s_lvl) {
        nsides = char_class[player.t_ctype].hit_pts;
        for (j=0; j<(i-pstats.s_lvl); j++) /* Take care of multi-level jumps */
            add += max(1, roll(1,nsides) + const_bonus());
        max_stats.s_hpt += add;
        if ((pstats.s_hpt += add) > max_stats.s_hpt)
            pstats.s_hpt = max_stats.s_hpt;
        msg("Welcome, %s, to level %d",
            cnames[player.t_ctype][min(i-1, NUM_CNAMES-1)], i);
    }
    pstats.s_lvl = i;
    pstats.s_lvl += pstats.s_lvladj; /* correct for level adjustment */
    return(retval);
}

/*
 * Used to modify the players strength
 * it keeps track of the highest it has been, just in case
 */

chg_str(amt)
register int amt;
{
    register int ring_str;              /* ring strengths */
    register struct stats *ptr;         /* for speed */

    ptr = &pstats;
    ring_str = ring_value(R_ADDSTR);
    ptr->s_str -= ring_str;
    ptr->s_str += amt;
    if (ptr->s_str > MAXATT) ptr->s_str = MAXATT;
    if (ptr->s_str > max_stats.s_str)
        max_stats.s_str = ptr->s_str;
    ptr->s_str += ring_str;
    if (ptr->s_str <= 0) {
    pstats.s_hpt = -1;
        death(D_STRENGTH);
    }
    updpack(TRUE, &player);
}

/*
 * let's confuse the player
 */

confus_player()
{
    if (off(player, ISCLEAR))
    {
        msg("Wait, what's going on here!  Huh? What? Who?");
        if (find_slot(unconfuse))
            lengthen(unconfuse, HUHDURATION);
        else
            fuse(unconfuse, (VOID *)NULL, HUHDURATION, AFTER);
        turn_on(player, ISHUH);
    }
    else msg("You feel dizzy for a moment, but it quickly passes.");
}

/*
 * this routine computes the players current dexterity
 */

dex_compute()
{
    if (cur_misc[WEAR_GAUNTLET] != NULL         &&
        cur_misc[WEAR_GAUNTLET]->o_which == MM_G_DEXTERITY) {
        if (cur_misc[WEAR_GAUNTLET]->o_flags & ISCURSED)
            return (3);
        else
            return (21);
    }
    else
            return (pstats.s_dext);
}

/*
 * diag_ok:
 *      Check to see if the move is legal if it is diagonal
 */

diag_ok(sp, ep, flgptr)
register coord *sp, *ep;
struct thing *flgptr;
{
    register int numpaths = 0;

    /* Horizontal and vertical moves are always ok */
    if (ep->x == sp->x || ep->y == sp->y)
        return TRUE;

    /* Diagonal moves are not allowed if there is a horizontal or
     * vertical path to the destination
     */
    if (step_ok(ep->y, sp->x, MONSTOK, flgptr)) numpaths++;
    if (step_ok(sp->y, ep->x, MONSTOK, flgptr)) numpaths++;
    return(numpaths != 1);
}

/*
 * pick a random position around the give (y, x) coordinates
 */

coord *
fallpos(pos, be_clear, range)
register coord *pos;
bool be_clear;
int range;
{
        register int tried, i, j;
        register char ch;
        static coord ret;
        static short masks[] = {
                0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x100 };

/*
 * Pick a spot at random centered on the position given by 'pos' and
 * up to 'range' squares away from 'pos'
 *
 * If 'be_clear' is TRUE, the spot must be either FLOOR or PASSAGE
 * inorder to be considered valid
 *
 * Generate a number from 0 to 8, representing the position to pick.
 * Note that this DOES include the positon 'pos' itself
 *
 * If this position is not valid, mark it as 'tried', and pick another.
 * Whenever a position is picked that has been tried before,
 * sequentially find the next untried position. This eliminates costly
 * random number generation
 */

        tried = 0;
        while( tried != 0x1ff ) {
                i = rnd(9);
                while( tried & masks[i] )
                        i = (i + 1) % 9;

                tried |= masks[i];

                for( j = 1; j <= range; j++ ) {
                        ret.x = pos->x + j*grid[i].x;
                        ret.y = pos->y + j*grid[i].y;

                        if (ret.x == hero.x && ret.y == hero.y)
                                continue; /* skip the hero */

                        if (ret.x < 0 || ret.x > cols - 1 ||
                            ret.y < 1 || ret.y > lines - 3)
                                continue; /* off the screen? */

                        ch = winat(ret.y, ret.x);

                        /*
                         * Check to make certain the spot is valid
                         */
                        switch( ch ) {
                        case FLOOR:
                        case PASSAGE:
                                return( &ret );
                        case GOLD:
                        case SCROLL:
                        case POTION:
                        case STICK:
                        case RING:
                        case WEAPON:
                        case ARMOR:
                        case MM:
                        case FOOD:
                                if(!be_clear && levtype != POSTLEV)
                                        return( &ret );
                        default:
                                break;
                        }
                }
        }
        return( NULL );
}

/*
 * findmindex:
 *      Find the index into the monster table of a monster given its name.
 */

findmindex(name)
char *name;
{
    int which;

    for (which=1; which<NUMMONST; which++) {
         if (strcmp(name, monsters[which].m_name) == 0)
             break;
    }
    if (which >= NUMMONST) {
         debug("couldn't find monster index");
         which = 1;
    }
    return(which);
}

/*
 * find_mons:
 *      Find the monster from his coordinates
 */

struct linked_list *
find_mons(y, x)
register int y;
register int x;
{
    register struct linked_list *item;
    register struct thing *th;

    for (item = mlist; item != NULL; item = next(item))
    {
        th = THINGPTR(item);
        if (th->t_pos.y == y && th->t_pos.x == x)
            return item;
    }
    return NULL;
}

/*
 * find_obj:
 *      find the unclaimed object at y, x
 */

struct linked_list *
find_obj(y, x)
register int y;
register int x;
{
    register struct linked_list *obj;
    register struct object *op;

    for (obj = lvl_obj; obj != NULL; obj = next(obj))
    {
        op = OBJPTR(obj);
        if (op->o_pos.y == y && op->o_pos.x == x)
                return obj;
    }
    return NULL;
}

/*
 * get coordinates from the player using the cursor keys (or mouse)
 */

coord 
get_coordinates()
{
    register int which;
    coord c;

    c = hero;
    wmove(cw, hero.y, hero.x);
    draw(cw);
    for (;;) {
        which = (wgetch(cw) & 0177);
        switch(which) {
            case ESC:
                c = hero;
                wmove(cw, c.y, c.x);
                draw(cw);
            case '\n':
            case '\r':
                return(c);
            when 'h':
            case 'H':
                c.x--;
            when 'j':
            case 'J':
                c.y++;
            when 'k':
            case 'K':
                c.y--;
            when 'l':
            case 'L':
                c.x++;
            when 'y':
            case 'Y':
                c.x--; c.y--;
            when 'u':
            case 'U':
                c.x++; c.y--;
            when 'b':
            case 'B':
                c.x--; c.y++;
            when 'n':
            case 'N':
                c.x++; c.y++;
            when '*':
               mpos = 0;
               msg("Use h,j,k,l,y,u,b,n to position cursor, then press enter.");
        }
        c.y = max(c.y, 1);
        c.y = min(c.y, lines - 3);
        c.x = max(c.x, 0);
        c.x = min(c.x, cols - 1);
        wmove(cw, c.y, c.x);
        draw(cw);
    } 
}

/*
 * set up the direction co_ordinate for use in various "prefix" commands
 */

bool
get_dir(direction)
coord *direction;
{
    register char *prompt;
    register bool gotit;
    int x,y;

    prompt = terse ? "Direction?" :  "Which direction? ";
    msg(prompt);
    do
    {
        gotit = TRUE;
        switch (wgetch(msgw))
        {
            case 'h': case'H': direction->y =  0; direction->x = -1;
            when 'j': case'J': direction->y =  1; direction->x =  0;
            when 'k': case'K': direction->y = -1; direction->x =  0;
            when 'l': case'L': direction->y =  0; direction->x =  1;
            when 'y': case'Y': direction->y = -1; direction->x = -1;
            when 'u': case'U': direction->y = -1; direction->x =  1;
            when 'b': case'B': direction->y =  1; direction->x = -1;
            when 'n': case'N': direction->y =  1; direction->x =  1;
            when ESC: return (FALSE);
            otherwise:
                mpos = 0;
                msg(prompt);
                gotit = FALSE;
        }
    } until (gotit);
    if ((on(player, ISHUH) || on(player, ISDANCE)) && rnd(100) > 20) {
        do
        {
            *direction = grid[rnd(9)];
        } while (direction->y == 0 && direction->x == 0);
    }
    else if (on(player, ISFLEE)) {
            y = hero.y;
            x = hero.x;
            while (shoot_ok(winat(y, x))) {
                y += direction->y;
                x += direction->x;
            }
            if (isalpha(mvwinch(mw, y, x))) {
                if (y == player.t_dest->y && x == player.t_dest->x) {
                    mpos = 0;
                    msg("You are too frightened to!");
                    return(FALSE);
            }
        }
    }
    mpos = 0;
    return TRUE;
}


/*
 * get_worth:
 *      Calculate an objects worth in gold
 */

long
get_worth(obj)
reg struct object *obj;
{
        reg long worth, wh;

        worth = 0;
        wh = obj->o_which;
        switch (obj->o_type) {
            case FOOD:
                worth = 2;
            when WEAPON:
                if (wh < MAXWEAPONS) {
                    worth = weaps[wh].w_worth;
                    worth += s_magic[S_ALLENCH].mi_worth * 
                                 (obj->o_hplus + obj->o_dplus);
                }
            when ARMOR:
                if (wh < MAXARMORS) {
                    worth = armors[wh].a_worth;
                    worth += s_magic[S_ALLENCH].mi_worth * 
                                (armors[wh].a_class - obj->o_ac);
                }
            when SCROLL:
                if (wh < MAXSCROLLS)
                    worth = s_magic[wh].mi_worth;
            when POTION:
                if (wh < MAXPOTIONS)
                    worth = p_magic[wh].mi_worth;
            when RING:
                if (wh < MAXRINGS) {
                    worth = r_magic[wh].mi_worth;
                    worth += obj->o_ac * 40;
                }
            when STICK:
                if (wh < MAXSTICKS) {
                    worth = ws_magic[wh].mi_worth;
                    worth += 20 * obj->o_charges;
                }
            when MM:
                if (wh < MAXMM) {
                    worth = m_magic[wh].mi_worth;
                    switch (wh) {
                        case MM_BRACERS:        worth += 40  * obj->o_ac;
                        when MM_PROTECT:        worth += 60  * obj->o_ac;
                        when MM_DISP:           /* ac already figured in price*/
                        otherwise:              worth += 20  * obj->o_ac;
                    }
                }
            when RELIC:
                if (wh < MAXRELIC) {
                    worth = rel_magic[wh].mi_worth;
                    if (wh == quest_item) worth *= 10;
                }
            otherwise:
                worth = 0;
        }
        if (obj->o_flags & ISPROT)      /* 300% more for protected */
            worth *= 3;
        if (obj->o_flags &  ISBLESSED)  /* 50% more for blessed */
            worth = worth * 3 / 2;
        if (obj->o_flags & ISCURSED)    /* half for cursed */
            worth /= 2;
        if (worth < 0)
            worth = 0;
        return worth;
}

/*
 *      invisible()
 */

bool
invisible(monst)
register struct thing *monst;
{
        register bool   ret_code;

        ret_code  = on(*monst, CANSURPRISE);
        ret_code &= !ISWEARING(R_ALERT);
        ret_code |= (on(*monst, ISINVIS) ||     
                        (on(*monst, ISSHADOW) && rnd(100) < 90)) &&
                        off(player, CANSEE);
        return( ret_code );
}

/* 
 * see if the object is one of the currently used items
 */

is_current(obj)
register struct object *obj;
{
    if (obj == NULL)
        return FALSE;
    if (obj == cur_armor         || obj == cur_weapon        || 
        obj == cur_ring[LEFT_1]  || obj == cur_ring[LEFT_2]  ||
        obj == cur_ring[LEFT_3]  || obj == cur_ring[LEFT_4]  ||
        obj == cur_ring[RIGHT_1] || obj == cur_ring[RIGHT_2] ||
    obj == cur_ring[RIGHT_3] || obj == cur_ring[RIGHT_4] ||
        obj == cur_misc[WEAR_BOOTS]    || obj == cur_misc[WEAR_JEWEL] ||
        obj == cur_misc[WEAR_BRACERS]  || obj == cur_misc[WEAR_CLOAK] ||
        obj == cur_misc[WEAR_GAUNTLET] || obj == cur_misc[WEAR_NECKLACE]) {

        return TRUE;
    }

    /* Is it a "current" relic? */
    if (obj->o_type == RELIC) {
        switch (obj->o_which) {
            case MUSTY_DAGGER:
            case EMORI_CLOAK:
            case HEIL_ANKH:
            case YENDOR_AMULET:
            case STONEBONES_AMULET:
            case HRUGGEK_MSTAR:
            case AXE_AKLAD:
            case YEENOGHU_FLAIL:
            case SURTUR_RING:
                if (cur_relic[obj->o_which]) return TRUE;
        }
    }

    return FALSE;
}


/*
 * Look:
 *      A quick glance all around the player
 */

look(wakeup, runend)
bool wakeup;    /* Should we wake up monsters */
bool runend;    /* At end of a run -- for mazes */
{
    register int x, y, radius;
    register unsigned char ch, och;
    register int oldx, oldy;
    register bool inpass, horiz, vert, do_light = FALSE, do_blank = FALSE;
    register int passcount = 0, curfloorcount = 0, nextfloorcount = 0;
    register struct room *rp;
    register int ey, ex;

    inpass = ((rp = roomin(&hero)) == NULL); /* Are we in a passage? */

    /* Are we moving vertically or horizontally? */
    if (runch == 'h' || runch == 'l') horiz = TRUE;
    else horiz = FALSE;
    if (runch == 'j' || runch == 'k') vert = TRUE;
    else vert = FALSE;

    /* How far around himself can the player see? */
    if (levtype == OUTSIDE) {
        if (daytime) radius = 9;
        else if (lit_room(rp)) radius = 3;
        else radius = 1;
    }
    else radius = 1;

    getyx(cw, oldy, oldx);      /* Save current position */

    /* Blank out the floor around our last position and check for
     * moving out of a corridor in a maze.
     */
    if (levtype == OUTSIDE) do_blank = !daytime;
    else if (oldrp != NULL && !lit_room(oldrp) && off(player, ISBLIND))
            do_blank = TRUE;

    /* Now move around the old position and blank things out */
    ey = player.t_oldpos.y + radius;
    ex = player.t_oldpos.x + radius;
    for (x = player.t_oldpos.x - radius; x <= ex; x++)
      if (x >= 0 && x < cols)
        for (y = player.t_oldpos.y - radius; y <= ey; y++) {
            struct linked_list *it;
            coord here;         /* Current <x,y> coordinate */
            unsigned char savech;        /* Saves character in monster window */
            bool in_room;       /* Are we in a room? */

            if (y < 1 || y > lines - 3) continue;

            /* See what's there -- ignore monsters, just see what they're on */
            savech = mvwinch(mw, y, x);
            waddch(mw, ' ');
            ch = show(y, x);
            mvwaddch(mw, y, x, savech); /* Restore monster */

            /*
             * If we have a monster that we can't see anymore, make sure
             * that we can note that fact.
             */
            if (isalpha(savech) &&
                (y < hero.y - radius || y > hero.y + radius ||
                 x < hero.x - radius || x > hero.x + radius)) {
                /* Find the monster */
                it = find_mons(y, x);
            }
            else it = NULL;

            /* Are we in a room? */
            here.y = y;
            here.x = x;
            in_room = (roomin(&here) != NULL);

            if ((do_blank || !in_room) && (y != hero.y || x != hero.x))
                switch (ch) {
                    case DOOR:
                    case SECRETDOOR:
                    case PASSAGE:
                    case STAIRS:
                    case TRAPDOOR:
                    case TELTRAP:
                    case BEARTRAP:
                    case SLEEPTRAP:
                    case ARROWTRAP:
                    case DARTTRAP:
                    case WORMHOLE:
                    case MAZETRAP:
                    case POOL:
                    case POST:
                    case VERTWALL:
                    case HORZWALL:
                    case WALL:
                        /* If there was a monster showing, make it disappear */
                        if (isalpha(savech)) {
                            mvwaddch(cw, y, x, ch);

                            /* 
                             * If we found it (we should!), set it to
                             * the right character!
                             */
                            if (it) (THINGPTR(it))->t_oldch = ch;
                        }
                        break;
                    when FLOOR:
                    case FOREST:
                    default:
                        mvwaddch(cw, y, x, in_room ? ' ' : PASSAGE);

                        /* If we found a monster, set it to darkness! */
                        if (it) (THINGPTR(it))->t_oldch = mvwinch(cw, y, x);
                }
                
            /* Moving out of a corridor? */
            if (levtype == MAZELEV && !ce(hero, player.t_oldpos) &&
                !running && !isrock(ch) &&  /* Not running and not a wall */
                ((vert && x != player.t_oldpos.x && y==player.t_oldpos.y) ||
                 (horiz && y != player.t_oldpos.y && x==player.t_oldpos.x)))
                    do_light = off(player, ISBLIND);
        }

    /* Take care of unlighting a corridor */
    if (do_light && lit_room(rp)) light(&player.t_oldpos);

    /* Are we coming or going between a wall and a corridor in a maze? */
    och = show(player.t_oldpos.y, player.t_oldpos.x);
    ch = show(hero.y, hero.x);
    if (levtype == MAZELEV &&
        ((isrock(och) && !isrock(ch)) || (isrock(ch) && !isrock(och)))) {
        do_light = off(player, ISBLIND); /* Light it up if not blind */

        /* Unlight what we just saw */
        if (do_light && lit_room(&rooms[0])) light(&player.t_oldpos);
    }

    /* Look around the player */
    ey = hero.y + radius;
    ex = hero.x + radius;
    for (x = hero.x - radius; x <= ex; x++)
        if (x >= 0 && x < cols) for (y = hero.y - radius; y <= ey; y++) {
            if (y < 1 || y >= lines - 2)
                continue;
            if (isalpha(mvwinch(mw, y, x)))
            {
                register struct linked_list *it;
                register struct thing *tp;

                if (wakeup)
                    it = wake_monster(y, x);
                else
                    it = find_mons(y, x);

                if (it) {
                    tp = THINGPTR(it);
                    tp->t_oldch = mvinch(y, x);
                    if (isatrap(tp->t_oldch)) {
                        register struct trap *trp = trap_at(y, x);

                        tp->t_oldch = (trp->tr_flags & ISFOUND) ? tp->t_oldch
                                                                : trp->tr_show;
                    }
                    if (tp->t_oldch == FLOOR && !lit_room(rp) &&
                        off(player, ISBLIND))
                            tp->t_oldch = ' ';
                }
            }

            /*
             * Secret doors show as walls
             */
            if ((ch = show(y, x)) == SECRETDOOR)
                ch = secretdoor(y, x);
            /*
             * Don't show room walls if he is in a passage and
             * check for maze turns
             */
            if (off(player, ISBLIND))
            {
                if (y == hero.y && x == hero.x
                    || (inpass && (ch == HORZWALL || ch == VERTWALL)))
                        continue;

                /* Did we come to a crossroads in a maze? */
                if (levtype == MAZELEV &&
                    (runend || !ce(hero, player.t_oldpos)) &&
                    !isrock(ch) &&      /* Not a wall */
                    ((vert && x != hero.x && y == hero.y) ||
                     (horiz && y != hero.y && x == hero.x)))
                        /* Just came to a turn */
                        do_light = off(player, ISBLIND);
            }
            else if (y != hero.y || x != hero.x)
                continue;

            wmove(cw, y, x);
            waddch(cw, ch);
            if (door_stop && !firstmove && running)
            {
                switch (runch)
                {
                    case 'h':
                        if (x == hero.x + 1)
                            continue;
                    when 'j':
                        if (y == hero.y - 1)
                            continue;
                    when 'k':
                        if (y == hero.y + 1)
                            continue;
                    when 'l':
                        if (x == hero.x - 1)
                            continue;
                    when 'y':
                        if ((x + y) - (hero.x + hero.y) >= 1)
                            continue;
                    when 'u':
                        if ((y - x) - (hero.y - hero.x) >= 1)
                            continue;
                    when 'n':
                        if ((x + y) - (hero.x + hero.y) <= -1)
                            continue;
                    when 'b':
                        if ((y - x) - (hero.y - hero.x) <= -1)
                            continue;
                }
                switch (ch)
                {
                    case DOOR:
                        if (x == hero.x || y == hero.y)
                            running = FALSE;
                        break;
                    case PASSAGE:
                        if (x == hero.x || y == hero.y)
                            passcount++;
                        break;
                    case FLOOR:
                        /* Stop by new passages in a maze (floor next to us) */
                        if ((levtype == MAZELEV) &&
                            !(hero.y == y && hero.x == x)) {
                            if (vert) { /* Moving vertically */
                                /* We have a passage on our row */
                                if (y == hero.y) curfloorcount++;

                                /* Some passage on the next row */
                                else if (y != player.t_oldpos.y)
                                    nextfloorcount++;
                            }
                            else {      /* Moving horizontally */
                                /* We have a passage on our column */
                                if (x == hero.x) curfloorcount++;

                                /* Some passage in the next column */
                                else if (x != player.t_oldpos.x)
                                    nextfloorcount++;
                            }
                        }
                    case VERTWALL:
                    case HORZWALL:
                    case ' ':
                        break;
                    default:
                        running = FALSE;
                        break;
                }
            }
        }

    /* Have we passed a side passage, with multiple choices? */
    if (curfloorcount > 0 && nextfloorcount > 0) running = FALSE;

    else if (door_stop && !firstmove && passcount > 1)
        running = FALSE;

    /* Do we have to light up the area (just stepped into a new corridor)? */
    if (do_light && !running && lit_room(rp)) light(&hero);

    mvwaddch(cw, hero.y, hero.x, PLAYER);
    wmove(cw, oldy, oldx);
    if (!ce(player.t_oldpos, hero)) {
        player.t_oldpos = hero; /* Don't change if we didn't move */
        oldrp = rp;
    }
}

/* 
 * Lower a level of experience 
 */

lower_level(who)
short who;
{
    int fewer, nsides;
	unsigned long exp;

    msg("You suddenly feel less skillful.");
    if (--pstats.s_lvl == 0) {
    pstats.s_hpt = -1;
        death(who);             /* All levels gone */
    }
    if (pstats.s_lvladj > 0) { /* lose artificial levels first */
        pstats.s_lvladj--;
        return;
    }
    exp = char_class[player.t_ctype].cap;
    if (pstats.s_exp >= exp*2)
        pstats.s_exp -= exp;
    else
        pstats.s_exp /= 2;

    nsides = char_class[player.t_ctype].hit_pts;
    fewer = max(1, roll(1,nsides) + const_bonus());
    pstats.s_hpt -= fewer;
    max_stats.s_hpt -= fewer;
    if (max_stats.s_hpt <= 0)
    max_stats.s_hpt = 0;
    if (pstats.s_hpt <= 0) {
        pstats.s_hpt = -1;
        death(who);
    }
}

/*
 * print out the name of a monster
 */

char *
monster_name(tp)
register struct thing *tp;
{
    prbuf[0] = '\0';
    if (on(*tp, ISFLEE) || on(*tp, WASTURNED))
        strcat(prbuf, "terrified ");
    if (on(*tp, ISHUH))
        strcat(prbuf, "confused ");
    if (on(*tp, ISCHARMED))
        strcat(prbuf, "charmed ");
    else if (on(*tp, ISFLY))
        strcat(prbuf, "flying ");

    /* If it is sleeping or stoned, write over any of the above attributes */
    if (off(*tp, ISRUN))
        strcpy(prbuf, "sleeping ");
    if (on(*tp, ISSTONE))
        strcpy(prbuf, "petrified ");

    if (tp->t_name) strcat(prbuf, tp->t_name);
    else strcat(prbuf, monsters[tp->t_index].m_name);

    return(prbuf);
}

/*
 * move_hero:
 *      Try to move the hero somplace besides next to where he is.  We ask him
 *      where.  There can be restrictions based on why he is moving.
 */

bool
move_hero(why)
int why;
{
    char *action = NULL;
    unsigned char which;
    coord c;

    switch (why) {
        case H_TELEPORT:
            action = "teleport";
    }

    msg("Where do you wish to %s to? (* for help) ", action);
    c = get_coordinates();
    mpos = 0;
    which = winat(c.y, c.x);
    switch (which) {
        default:
            if (!isrock(which) || off(player, CANINWALL)) break;

        case FLOOR:
        case PASSAGE:
        case DOOR:
        case STAIRS:
        case POST:
        case GOLD:
        case POTION:
        case SCROLL:
        case FOOD:
        case WEAPON:
        case ARMOR:
        case RING:
        case MM:
        case RELIC:
        case STICK:
            hero = c;
            return(TRUE);
    }
    return(FALSE);
}

/*
 * raise_level:
 *      The guy just magically went up a level.
 */

raise_level()
{
    unsigned long test;  /* Next level -- be sure it is not an overflow */

    test = check_level();       /* Get next boundary */

    /* Be sure it is higher than what we have no -- else overflow */
    if (test > pstats.s_exp) pstats.s_exp = test;
    check_level();

    /* Give him a bonus */
    switch (player.t_ctype) {
        case C_FIGHTER:
            (*add_abil[A_STRENGTH])(1);
        when C_RANGER:
        case C_PALADIN:
            (*add_abil[A_CHARISMA])(1);
        when C_MAGICIAN:
            (*add_abil[A_INTELLIGENCE])(1);
        when C_CLERIC:
        case C_DRUID:
            (*add_abil[A_WISDOM])(1);
        when C_THIEF:
        case C_ASSASSIN:
            (*add_abil[A_DEXTERITY])(1);
        when C_MONK:
            (*add_abil[A_CONSTITUTION])(1);
    }
}

/*
 * saving throw matrix for character saving throws
 * this table is indexed by char type and saving throw type
 */

static const char st_matrix[NUM_CHARTYPES][5] = {
/* Poison,      Petrify,        wand,           Breath,         Magic */
{ 13,           14,             15,             16,             17 },
{ 13,           14,             15,             16,             17 },
{ 13,           14,             15,             16,             17 },
{ 11,           12,             13,             14,             15 },
{ 11,           12,             13,             14,             15 },
{ 12,           13,             14,             15,             16 },
{ 12,           13,             14,             15,             16 },
{ 11,           12,             12,             14,             15 },
{ 12,           13,             14,             15,             16 },
{ 13,           14,             15,             16,             17 }
};

/*
 * save:
 *      See if a creature saves against something
 */

save(which, who, adj)
int which;              /* which type of save */
struct thing *who;      /* who is saving */
int adj;                /* saving throw adjustment */
{
    register int need, level, protect;

    protect = 0;
    level = who->t_stats.s_lvl;
    need = st_matrix[who->t_ctype][which];
    switch (who->t_ctype) {
    case C_FIGHTER:
    case C_RANGER:
    case C_PALADIN:
        need -= (2 * (level-1) / 5) - 1;        /* for level 61; -= 25 */
    when C_THIEF:
    case C_ASSASSIN:
    case C_MONK:
    case C_MONSTER:
        need -= (2 * (level-1) / 5) - 3;        /* for level 61; -= 27 */
    when C_MAGICIAN:
    case C_CLERIC:
    case C_DRUID:
        need -= (2 * (level-1) / 5) - 5;        /* for level 61; -= 29 */
    }
    /* 
     * add in pluses against poison for execeptional constitution 
     */
    if (which == VS_POISON && who->t_stats.s_const > 18)
        need -= (who->t_stats.s_const - 17) / 2;
    if (who == &player) {
        /*
         * does the player have a ring of protection on?
         */
        protect +=  ring_value(R_PROTECT);
        /*
         * does the player have a cloak of protection on?
         */
        if (cur_misc[WEAR_CLOAK])
            protect += cur_misc[WEAR_CLOAK]->o_ac;

        protect = min(protect, 10);/* limit protection to +10 */
        need -= protect;
    }
    need -= adj;
    /*
     * always miss or save on a 1 (except for UNIQUEs
     */
    if (who == &player || off(*who, ISUNIQUE))
        need = max(need, 2);
    need = min(20, need); /* always make our save on a 20 */
    debug("need a %d to save", need);
    return (roll(1, 20) >= need);
}

/*
 * secret_door:
 *      Figure out what a secret door looks like.
 */

secretdoor(y, x)
register int y, x;
{
    register int i;
    register struct room *rp;
    register coord *cpp;
    static coord cp;

    cp.y = y;
    cp.x = x;
    cpp = &cp;
    for (rp = rooms, i = 0; i < MAXROOMS; rp++, i++)
        if (inroom(rp, cpp))
            if (y == rp->r_pos.y || y == rp->r_pos.y + rp->r_max.y - 1)
                return(HORZWALL);
            else
                return(VERTWALL);

    return('p');
}

/*
 * this routine computes the players current strength
 */

str_compute()
{
    if (cur_misc[WEAR_GAUNTLET] != NULL         &&
        cur_misc[WEAR_GAUNTLET]->o_which == MM_G_OGRE) {
        if (cur_misc[WEAR_GAUNTLET]->o_flags & ISCURSED)
            return (3);
        else
            return (21);
    }
    else
            return (pstats.s_str);
}

/*
 * copy string using unctrl for things
 */

strucpy(s1, s2, len)
register char *s1, *s2;
register int len;
{
    register char *sp;
    while (len--)
    {
        strcpy(s1, (sp = unctrl(*s2)));
        s1 += strlen(sp);
        s2++;
    }
    *s1 = '\0';
}

/*
 * tr_name:
 *      print the name of a trap
 */

char *
tr_name(ch)
char ch;
{
    register char *s = NULL;

    switch (ch)
    {
        case TRAPDOOR:
            s = terse ? "A trapdoor." : "You found a trapdoor.";
        when BEARTRAP:
            s = terse ? "A beartrap." : "You found a beartrap.";
        when SLEEPTRAP:
            s = terse ? "A sleeping gas trap.":"You found a sleeping gas trap.";
        when ARROWTRAP:
            s = terse ? "An arrow trap." : "You found an arrow trap.";
        when TELTRAP:
            s = terse ? "A teleport trap." : "You found a teleport trap.";
        when DARTTRAP:
            s = terse ? "A dart trap." : "You found a poison dart trap.";
        when POOL:
            s = terse ? "A shimmering pool." : "You found a shimmering pool";
        when MAZETRAP:
            s = terse ? "A maze entrance." : "You found a maze entrance";
        when WORMHOLE:
            s = terse ? "A worm hole." : "You found a worm hole entrance";
    }
    return s;
}

/*
 * for printfs: if string starts with a vowel, return "n" for an "an"
 */

char *
vowelstr(str)
register char *str;
{
    switch (*str)
    {
        case 'a':
        case 'e':
        case 'i':
        case 'o':
        case 'u':
            return "n";
        default:
            return "";
    }
}

/*
 * wake up a room full (hopefully) of creatures
 */

wake_room(rp)
register struct room *rp;
{
        register struct linked_list *item;
        register struct thing *tp;

        for (item=mlist; item!=NULL; item=next(item)) {
            tp = THINGPTR(item);
            if (off(*tp,ISRUN) && on(*tp,ISMEAN) && roomin(&tp->t_pos) == rp)
                runto(tp, &hero);
        }
}
                

/*
 * waste_time:
 *      Do nothing but let other things happen
 */

waste_time()
{
    if (inwhgt)                 /* if from wghtchk then done */
        return;
    do_daemons(BEFORE);
    do_fuses(BEFORE);
    do_daemons(AFTER);
    do_fuses(AFTER);
}

