/*
    passages.c - Draw the connecting passages
    
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
 * do_passages:
 *      Draw all the passages on a level.
 */

do_passages()
{
    register struct rdes *r1, *r2 = NULL;
    register int i, j;
    register int roomcount;
    static struct rdes
    {
        bool    conn[MAXROOMS];         /* possible to connect to room i? */
        bool    isconn[MAXROOMS];       /* connection been made to room i? */
        bool    ingraph;                /* this room in graph already? */
    } rdes[MAXROOMS] = {
        { { 0, 1, 0, 1, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
        { { 1, 0, 1, 0, 1, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
        { { 0, 1, 0, 0, 0, 1, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
        { { 1, 0, 0, 0, 1, 0, 1, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
        { { 0, 1, 0, 1, 0, 1, 0, 1, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
        { { 0, 0, 1, 0, 1, 0, 0, 0, 1 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
        { { 0, 0, 0, 1, 0, 0, 0, 1, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
        { { 0, 0, 0, 0, 1, 0, 1, 0, 1 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
        { { 0, 0, 0, 0, 0, 1, 0, 1, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 0 },
    };

    /*
     * reinitialize room graph description
     */
    for (i = 0; i < MAXROOMS; i++)
    {
        r1 = &rdes[i];
        for (j = 0; j < MAXROOMS; j++)
            r1->isconn[j] = FALSE;
        r1->ingraph = FALSE;
    }

    /*
     * starting with one room, connect it to a random adjacent room and
     * then pick a new room to start with.
     */
    roomcount = 1;
    r1 = &rdes[rnd(MAXROOMS)];
    r1->ingraph = TRUE;
    do
    {
        /*
         * find a room to connect with
         */
        j = 0;
        for (i = 0; i < MAXROOMS; i++)
            if (r1->conn[i] && !rdes[i].ingraph && rnd(++j) == 0)
                r2 = &rdes[i];
        /*
         * if no adjacent rooms are outside the graph, pick a new room
         * to look from
         */
        if (j == 0)
        {
            do
                r1 = &rdes[rnd(MAXROOMS)];
            until (r1->ingraph);
        }
        /*
         * otherwise, connect new room to the graph, and draw a tunnel
         * to it
         */
        else
        {
            r2->ingraph = TRUE;
            i = r1 - rdes;
            j = r2 - rdes;
            conn(i, j);
            r1->isconn[j] = TRUE;
            r2->isconn[i] = TRUE;
            roomcount++;
        }
    } while (roomcount < MAXROOMS);

    /*
     * attempt to add passages to the graph a random number of times so
     * that there isn't just one unique passage through it.
     */
    for (roomcount = rnd(5); roomcount > 0; roomcount--)
    {
        r1 = &rdes[rnd(MAXROOMS)];      /* a random room to look from */
        /*
         * find an adjacent room not already connected
         */
        j = 0;
        for (i = 0; i < MAXROOMS; i++)
            if (r1->conn[i] && !r1->isconn[i] && rnd(++j) == 0)
                r2 = &rdes[i];
        /*
         * if there is one, connect it and look for the next added
         * passage
         */
        if (j != 0)
        {
            i = r1 - rdes;
            j = r2 - rdes;
            conn(i, j);
            r1->isconn[j] = TRUE;
            r2->isconn[i] = TRUE;
        }
    }
}

/*
 * conn:
 *      Draw a corridor from a room in a certain direction.
 */

conn(r1, r2)
int r1, r2;
{
    register struct room *rpf, *rpt = NULL;
    register char rmt;
    register int distance = 0, max_diag, offset = 0, i;
    register int rm;
    int turns[3], turn_dist[3];
    register char direc;
	coord delta = {0, 0}, curr, turn_delta = {0,0}, spos = {0,0}, epos = {0,0};

    if (r1 < r2)
    {
        rm = r1;
        if (r1 + 1 == r2)
            direc = 'r';
        else
            direc = 'd';
    }
    else
    {
        rm = r2;
        if (r2 + 1 == r1)
            direc = 'r';
        else
            direc = 'd';
    }
    rpf = &rooms[rm];
    /*
     * Set up the movement variables, in two cases:
     * first drawing one down.
     */
    if (direc == 'd')
    {
        rmt = rm + 3;                           /* room # of dest */
        rpt = &rooms[rmt];                      /* room pointer of dest */
        delta.x = 0;                            /* direction of move */
        delta.y = 1;
        spos.x = rpf->r_pos.x;                  /* start of move */
        spos.y = rpf->r_pos.y;
        epos.x = rpt->r_pos.x;                  /* end of move */
        epos.y = rpt->r_pos.y;
        if (!(rpf->r_flags & ISGONE))           /* if not gone pick door pos */
        {
            spos.x += rnd(rpf->r_max.x-2)+1;
            spos.y += rpf->r_max.y-1;
        }
        if (!(rpt->r_flags & ISGONE))
            epos.x += rnd(rpt->r_max.x-2)+1;
        distance = abs(spos.y - epos.y) - 1;    /* distance to move */
        turn_delta.y = 0;                       /* direction to turn */
        turn_delta.x = (spos.x < epos.x ? 1 : -1);
        offset = abs(spos.x - epos.x);  /* how far to turn */
    }
    else if (direc == 'r')                      /* setup for moving right */
    {
        rmt = rm + 1;
        rpt = &rooms[rmt];
        delta.x = 1;
        delta.y = 0;
        spos.x = rpf->r_pos.x;
        spos.y = rpf->r_pos.y;
        epos.x = rpt->r_pos.x;
        epos.y = rpt->r_pos.y;
        if (!(rpf->r_flags & ISGONE))
        {
            spos.x += rpf->r_max.x-1;
            spos.y += rnd(rpf->r_max.y-2)+1;
        }
        if (!(rpt->r_flags & ISGONE))
            epos.y += rnd(rpt->r_max.y-2)+1;
        distance = abs(spos.x - epos.x) - 1;
        turn_delta.y = (spos.y < epos.y ? 1 : -1);
        turn_delta.x = 0;
        offset = abs(spos.y - epos.y);
    }
    else
        debug("error in connection tables");

    /*
     * Draw in the doors on either side of the passage or just put #'s
     * if the rooms are gone.
     */
    if (!(rpf->r_flags & ISGONE)) door(rpf, &spos);
    else
    {
        cmov(spos);
        addch('#');
    }
    if (!(rpt->r_flags & ISGONE)) door(rpt, &epos);
    else
    {
        cmov(epos);
        addch('#');
    }

    /* How far can we move diagonally? */
    max_diag = min(distance, offset);

    /*
     * Decide how many turns we will have.
     */
    for (i=0; i<3; i++) turn_dist[i] = 0;       /* Init distances */
    if (max_diag > 0) {
        int nturns;

        for (i=0, nturns=0; i<3; i++) {
            if (rnd(3 - i + nturns) == 0) {
                nturns++;
                turns[i] = 0;
            }
            else turns[i] = -1;
        }
    }
    else {
        /* Just use a straight line (middle turn) */
        turns[0] = turns[2] = -1;
        turns[1] = 0;
    }

    /*
     * Now decide how long each turn will be (for those selected above).
     */
    while (max_diag > 0) {
        for (i=0; i<3; i++) {
            if (turns[i] >= 0 && max_diag > 0 && rnd(2) == 0) {
                turn_dist[i]++;
                max_diag--;
            }
        }
    }
    
    /*
     * If we have extra offset space, add it to the straight turn.
     */
    if (offset > distance) turn_dist[1] += offset - distance;

    /*
     * Decide where we want to make our turns.
     * First calculate the offsets, then use those offsets to calculate
     * the exact position relative to "distance."
     */
    turns[0] = rnd(distance - turn_dist[0] - turn_dist[2]);
    turns[2] = rnd(distance - turn_dist[0] - turn_dist[2] - turns[0]);
    turns[1] = rnd(distance - turn_dist[0] - turn_dist[2] -
                   turns[0] - turns[2]);

    turns[0] = distance - turns[0];
    turns[1] = turns[0] - turn_dist[0] - turns[1];
    turns[2] = turns[1] - turns[2];

    /*
     * Get ready to move...
     */
    curr.x = spos.x;
    curr.y = spos.y;
    while (distance > 0) {
        /*
         * Move to next row/column
         */
        curr.x += delta.x;
        curr.y += delta.y;

        /*
         * Check if we are at a turn place; if so make a turn
         */
        for (i=0; i<3; i++) {
            if (distance == turns[i] && turn_dist[i] > 0) {
                /*
                 * If this is the start of a straight path,
                 * we might put in a right-angle turn (33% chance).
                 */
                if (i == 1 && rnd(3) == 0) {
                    cmov(curr);
                    addch(PASSAGE);
                }

                /* Now dig the turn */
                while (turn_dist[i]--) {
                    curr.x += turn_delta.x;
                    curr.y += turn_delta.y;
                    cmov(curr);
                    addch(PASSAGE);
                    if (i != 1) {       /* A diagonal */
                        if (--distance > 0) {
                            curr.x += delta.x;
                            curr.y += delta.y;
                        }
                    }
                }
            }
        }

        if (distance > 0) {
            /*
             * Dig the passage.
             */
            cmov(curr);
            addch(PASSAGE);
            distance--;
        }
    }
    curr.x += delta.x;
    curr.y += delta.y;
    if (!ce(curr, epos))
        msg("Warning, connectivity problem (%d, %d) to (%d, %d).",
            curr.y, curr.x, epos.y, epos.x);
}

/*
 * Add a door or possibly a secret door
 * also enters the door in the exits array of the room.
 */

door(rm, cp)
register struct room *rm;
register coord *cp;
{
    struct linked_list *newroom;
    coord *exit;

    cmov(*cp);

	if (rnd(10) < (level - 1) && rnd(100) < 20)
		addch(SECRETDOOR);
	else
		addch(DOOR);

    /* Insert the new room into the linked list of rooms */
    newroom = new_item(sizeof(coord));
    exit = DOORPTR(newroom);
    *exit = *cp;
    attach(rm->r_exit, newroom);
}

