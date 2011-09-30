/*
    maze.c  -  functions for dealing with mazes
    
    XRogue: Expeditions into the Dungeons of Doom
    Copyright (C) 1991 Robert Pietkivitch
    All rights reserved.
    
    Based on "Advanced Rogue"
    Copyright (C) 1984, 1985 Michael Morgan, Ken Dalka and AT&T
    All rights reserved.

    See the file LICENSE.TXT for full copyright and licensing information.
*/

#include <stdlib.h>
#include <curses.h>
#include "rogue.h"

struct cell {
        char y_pos;
        char x_pos;
};
struct b_cellscells {
        char num_pos;           /* number of frontier cells next to you */
        struct cell conn[4];    /* the y,x position of above cell */
} b_cells;

static char     *maze_frontier, *maze_bits;
static int      maze_lines, maze_cols;
static char     *moffset(), *foffset();
static int      rmwall(),findcells(),crankout(),draw_maze();

/*
 * crankout:
 *      Does actual drawing of maze to window
 */

static
crankout()
{
    reg int x, y;

    for (y = 0; y < lines - 3; y++) {
        move(y + 1, 0);
        for (x = 0; x < cols - 1; x++) {
            if (*moffset(y, x)) {           /* here is a wall */
                if(y==0 || y==lines-4) /* top or bottom line */
                    addch(HORZWALL);
                else if(x==0 || x==cols-2) /* left | right side */
                    addch(VERTWALL);
                else if (y % 2 == 0 && x % 2 == 0) {
                    if(*moffset(y, x-1) || *moffset(y, x+1))
                        addch(HORZWALL);
                    else
                        addch(VERTWALL);
                }
                else if (y % 2 == 0)
                    addch(HORZWALL);
                else
                    addch(VERTWALL);
            }
            else
                addch(FLOOR);
        }
    }
}

/*
 * domaze:
 *      Draw the maze on this level.
 */

do_maze()
{
        reg int least;
        reg struct room *rp;
        reg struct linked_list *item;
        reg struct object *obj;
        int cnt;
        bool treas;
        coord tp;

        for (rp = rooms; rp < &rooms[MAXROOMS]; rp++) {
                rp->r_flags = ISGONE;           /* kill all rooms */
                rp->r_fires = NULL;             /* no fires */
        }
        rp = &rooms[0];                         /* point to only room */
        rp->r_flags = ISDARK;                   /* mazes always dark */
        rp->r_pos.x = 0;                        /* room fills whole screen */
        rp->r_pos.y = 1;
        rp->r_max.x = cols - 1;
        rp->r_max.y = lines - 3;
        draw_maze();                            /* put maze into window */
        /*
         * add some gold to make it worth looking for 
         */
        item = spec_item(GOLD, NULL, NULL, NULL);
        obj = OBJPTR(item);
        obj->o_count *= (rnd(50) + 50);         /* add in one large hunk */
        attach(lvl_obj, item);
        cnt = 0;
        do {
            rnd_pos(rp, &tp);
        } until (mvinch(tp.y, tp.x) == FLOOR || cnt++ > 2500);
        mvaddch(tp.y, tp.x, GOLD);
        obj->o_pos = tp;
        /*
         * add in some food to make sure he has enough
         */
        item = spec_item(FOOD, NULL, NULL, NULL);
        obj = OBJPTR(item);
        attach(lvl_obj, item);
        do {
            rnd_pos(rp, &tp);
        } until (mvinch(tp.y, tp.x) == FLOOR || cnt++ > 2500);
        mvaddch(tp.y, tp.x, FOOD);
        obj->o_pos = tp;

        /* it doesn't mater if it's a treasure maze or a normal maze,
         * more than enough monsters will be genned. 
         */
        least = rnd(11)+5;
        if (least < 6) {
        least = 7;
        treas = FALSE;
    }
    else treas = TRUE;
        genmonsters(least, treas);

        /* sometimes they're real angry */
        if (rnd(100) < 65) {
            /* protect the good charactors */
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
 * draw_maze:
 *      Generate and draw the maze on the screen
 */

static
draw_maze()
{
        reg int i, j, more;
        reg char *ptr;

        maze_lines = (lines - 3) / 2;
        maze_cols = (cols - 1) / 2;
        maze_bits = ALLOC((lines - 3) * (cols - 1));
        maze_frontier = ALLOC(maze_lines * maze_cols);
        ptr = maze_frontier;
        while (ptr < (maze_frontier + (maze_lines * maze_cols)))
                *ptr++ = TRUE;
        for (i = 0; i < lines - 3; i++) {
                for (j = 0; j < cols - 1; j++) {
                        if (i % 2 == 1 && j % 2 == 1)
                                *moffset(i, j) = FALSE;         /* floor */
                        else
                                *moffset(i, j) = TRUE;          /* wall */
                }
        }
        for (i = 0; i < maze_lines; i++) {
                for (j = 0; j < maze_cols; j++) {
                        do
                                more = findcells(i,j);
                        while(more != 0);
                }
        }
        crankout();
        FREE(maze_frontier);
        FREE(maze_bits);
}

/*
 * findcells:
 *      Figure out cells to open up 
 */

static findcells(y,x)
reg int x, y;
{
        reg int rtpos, i;

        *foffset(y, x) = FALSE;
        b_cells.num_pos = 0;
        if (y < maze_lines - 1) {                               /* look below */
                if (*foffset(y + 1, x)) {
                        b_cells.conn[b_cells.num_pos].y_pos = y + 1;
                        b_cells.conn[b_cells.num_pos].x_pos = x;
                        b_cells.num_pos += 1;
                }
        }
        if (y > 0) {                                    /* look above */
                if (*foffset(y - 1, x)) {
                        b_cells.conn[b_cells.num_pos].y_pos = y - 1;
                        b_cells.conn[b_cells.num_pos].x_pos = x;
                        b_cells.num_pos += 1;

                }
        }
        if (x < maze_cols - 1) {                                /* look right */
                if (*foffset(y, x + 1)) {
                        b_cells.conn[b_cells.num_pos].y_pos = y;
                        b_cells.conn[b_cells.num_pos].x_pos = x + 1;
                        b_cells.num_pos += 1;
                }
        }
        if (x > 0) {                                    /* look left */
                if (*foffset(y, x - 1)) {
                        b_cells.conn[b_cells.num_pos].y_pos = y;
                        b_cells.conn[b_cells.num_pos].x_pos = x - 1;
                        b_cells.num_pos += 1;

                }
        }
        if (b_cells.num_pos == 0)               /* no neighbors available */
                return 0;
        else {
                i = rnd(b_cells.num_pos);
                rtpos = b_cells.num_pos - 1;
                rmwall(b_cells.conn[i].y_pos, b_cells.conn[i].x_pos, y, x);
                return rtpos;
        }
}

/*
 * foffset:
 *      Calculate memory address for frontier
 */

static char *
foffset(y, x)
int y, x;
{

        return (maze_frontier + (y * maze_cols) + x);
}


/*
 * Maze_view:
 *      Returns true if the player can see the specified location within
 *      the confines of a maze (within one column or row)
 */

bool
maze_view(y, x)
int y, x;
{
    register int start, goal, delta, ycheck = 0, xcheck = 0, absy, absx, see_radius;
    register bool row;

    /* Get the absolute value of y and x differences */
    absy = hero.y - y;
    absx = hero.x - x;
    if (absy < 0) absy = -absy;
    if (absx < 0) absx = -absx;

    /* If we are standing in a wall, we can see a bit more */
    switch (winat(hero.y, hero.x)) {
        case VERTWALL:
        case HORZWALL:
        case WALL:
        case SECRETDOOR:
        case DOOR:
            see_radius = 2;
        otherwise:
            see_radius = 1;
    }

    /* Must be within one or two rows or columns */
    if (absy > see_radius && absx > see_radius) return(FALSE);

    if (absx > see_radius) {            /* Go along row */
        start = hero.x;
        goal = x;
        ycheck = hero.y;
        row = TRUE;
    }
    else {                      /* Go along column */
        start = hero.y;
        goal = y;
        xcheck = hero.x;
        row = FALSE;
    }

    if (start <= goal) delta = 1;
    else delta = -1;

    /* Start one past where we are standing */
    if (start != goal) start += delta;

    /* If we are in a wall, we want to look in the area outside the wall */
    if (see_radius > 1) {
        if (row) {
            /* See if above us it okay first */
            switch (winat(ycheck, start)) {
                case VERTWALL:
                case HORZWALL:
                case WALL:
                case DOOR:
                case SECRETDOOR:
                    /* No good, try one up */
                    if (y > hero.y) ycheck++;
                    else ycheck--;
                otherwise:
                    see_radius = 1;     /* Just look straight over the row */
            }
        }
        else {
            /* See if above us it okay first */
            switch (winat(start, xcheck)) {
                case VERTWALL:
                case HORZWALL:
                case WALL:
                case DOOR:
                case SECRETDOOR:
                    /* No good, try one over */
                    if (x > hero.x) xcheck++;
                    else xcheck--;
                otherwise:
                    see_radius = 1;     /* Just look straight up the column */
            }
        }
    }

    /* Check boundary again */
    if (absy > see_radius && absx > see_radius) return(FALSE);

    while (start != goal) {
        if (row) xcheck = start;
        else     ycheck = start;

        if (xcheck < 0 || ycheck < 0)
            return FALSE;
        switch (winat(ycheck, xcheck)) {
            case VERTWALL:
            case HORZWALL:
            case WALL:
            case DOOR:
            case SECRETDOOR:
                return(FALSE);
        }
        start += delta;
    }
    return(TRUE);
}


/*
 * moffset:
 *      Calculate memory address for bits
 */

static char *
moffset(y, x)
int y, x;
{
    return (maze_bits + (y * (cols - 1)) + x);
}

/*
 * rmwall:
 *      Removes appropriate walls from the maze
 */
static
rmwall(newy, newx, oldy, oldx)
int newy, newx, oldy, oldx;
{
        reg int xdif,ydif;
        
        xdif = newx - oldx;
        ydif = newy - oldy;

        *moffset((oldy * 2) + ydif + 1, (oldx * 2) + xdif + 1) = FALSE;
        findcells(newy, newx);
}

