/*
    outside.c  -  functions for dealing with the "outside" level

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

extern char rnd_terrain(), get_terrain();

/*
 * init_terrain:
 *      Get the single "outside room" set up correctly
 */

void
init_terrain()
{
    register struct room *rp;

    for (rp = rooms; rp < &rooms[MAXROOMS]; rp++) {
            rp->r_flags = ISGONE;       /* kill all rooms */
            rp->r_fires = NULL;         /* no fires */
    }
    rp = &rooms[0];                     /* point to only room */
    rp->r_flags = ISDARK;               /* outside is always dark */
    rp->r_pos.x = 0;                    /* room fills whole screen */
    rp->r_pos.y = 1;
    rp->r_max.x = cols;
    rp->r_max.y = lines - 3;
}

void
do_terrain(basey, basex, deltay, deltax, fresh)
int basey, basex, deltay, deltax;
bool fresh;
{
    register int cury, curx;        /* Current y and x positions */

    /* Lay out the boundary */
    for (cury=1; cury<lines-2; cury++) {        /* Vertical "walls" */
        mvaddch(cury, 0, VERTWALL);
        mvaddch(cury, cols-1, VERTWALL);
    }
    for (curx=0; curx<cols; curx++) {           /* Horizontal "walls" */
        mvaddch(1, curx, HORZWALL);
        mvaddch(lines-3, curx, HORZWALL);
    }

    /* If we are not continuing, let's start out with a line of terrain */
    if (fresh) {
        char ch;        /* Next char to add */

        /* Move to the starting point (should be (1, 0)) */
        move(basey, basex);
        curx = basex;

        /* Start with some random terrain */
        if (basex == 0) {
            ch = rnd_terrain();
            addch(ch);
        }
        else ch = mvinch(basey, basex);

        curx += deltax;

        /* Fill in the rest of the line */
        while (curx > 0 && curx < cols-1) {
            /* Put in the next piece */
            ch = get_terrain(ch, '\0', '\0', '\0');
            mvaddch(basey, curx, ch);
            curx += deltax;
        }

        basey++;        /* Advance to next line */
    }

    /* Fill in the rest of the lines */
    cury = basey;
    while (cury > 1 && cury < lines - 3) {
        curx = basex;
        while (curx > 0 && curx < cols-1) {
            register char left, top_left, top, top_right;
            register int left_pos, top_pos;

            /* Get the surrounding terrain */
            left_pos = curx - deltax;
            top_pos = cury - deltay;

            left = mvinch(cury, left_pos);
            top_left = mvinch(top_pos, left_pos);
            top = mvinch(top_pos, curx);
            top_right = mvinch(top_pos, curx + deltax);

            /* Put the piece of terrain on the map */
            mvaddch(cury, curx, get_terrain(left, top_left, top, top_right));

            /* Get the next x coordinate */
            curx += deltax;
        }

        /* Get the next y coordinate */
        cury += deltay;
    }
        /* The deeper we go.. */
        if (level > 40)         genmonsters(20, (bool) 0);
        else if (level > 10)    genmonsters(15, (bool) 0);
        else                    genmonsters(10, (bool) 0);

        /* sometimes they're real angry */
        if (rnd(100) < 65) {
            /* protect good guys */
            if (player.t_ctype == C_PALADIN ||
                player.t_ctype == C_RANGER  || player.t_ctype == C_MONK) {
                    aggravate(TRUE, FALSE);
            }
            else {
                aggravate(TRUE, TRUE);
            }
        }
}

/*
 * do_paths:
 *      draw at least a single path-way through the terrain
 */

/*
 * rnd_terrain:
 *      return a weighted, random type of outside terrain
 */

char
rnd_terrain()
{
    int chance = rnd(100);

    /* Meadow is most likely */
    if (chance < 40) return(FLOOR);

    /* Next comes forest */
    if (chance < 65) return(FOREST);

    /* Then comes lakes */
    if (chance < 85) return(POOL);

    /* Finally, mountains */
    return(WALL);
}


/*
 * get_terrain:
 *      return a terrain weighted by what is surrounding
 */

char
get_terrain(one, two, three, four)
char one, two, three, four;
{
    register int i;
    int forest = 0, mountain = 0, lake = 0, meadow = 0, total = 0;
    char surrounding[4];

    surrounding[0] = one;
    surrounding[1] = two;
    surrounding[2] = three;
    surrounding[3] = four;

    for (i=0; i<4; i++) 
        switch (surrounding[i]) {
            case FOREST:
                forest++;
                total++;
            
            when WALL:
                mountain++;
                total++;

            when POOL:
                lake++;
                total++;

            when FLOOR:
                meadow++;
                total++;
        }

    /* Should we continue mountain? */
    if (rnd(total+1) < mountain) return(WALL);

    /* Should we continue lakes? */
    if (rnd(total+1) < lake) return(POOL);

    /* Should we continue meadow? */
    if (rnd(total+1) < meadow) return(FLOOR);

    /* Should we continue forest? */
    if (rnd(total+2) < forest) return(FOREST);

    /* Return something random */
    return(rnd_terrain());
}

/*
 * lake_check:
 *      Determine if the player would drown
 */

/*UNUSED*/
/* void
 * lake_check(place)
 * register coord *place;
 * {
 * }
 */

