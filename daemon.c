/*
    daemon.c - functions for dealing with things that happen in the future
 
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

#define EMPTY           0
#define DAEMON          -1

#define _X_ { EMPTY }

struct delayed_action {
        int d_type;
        int (*d_func)();
        VOID *d_arg;
        int d_time;
} ;
struct delayed_action d_list[MAXDAEMONS] = {
        _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_
};
struct delayed_action f_list[MAXFUSES] = {
        _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_,
        _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_
};

int demoncnt = 0;        /* number of active daemons */
int fusecnt = 0;


/*
 * d_slot:
 *      Find an empty slot in the daemon list
 */
struct delayed_action *
d_slot()
{
        reg int i;
        reg struct delayed_action *dev;

        for (i = 0, dev = d_list; i < MAXDAEMONS; i++, dev++)
                if (dev->d_type == EMPTY)
                        return dev;
        return NULL;
}

/*
 * f_slot:
 *      Find an empty slot in the fuses list
 */
struct delayed_action *
f_slot()
{
        reg int i;
        reg struct delayed_action *dev;

        for (i = 0, dev = f_list; i < MAXFUSES; i++, dev++)
                if (dev->d_type == EMPTY)
                        return dev;
        return NULL;
}

/*
 * find_slot:
 *      Find a particular slot in the table
 */

struct delayed_action *
find_slot(func)
reg int (*func)();
{
        reg int i;
        reg struct delayed_action *dev;

        for (i = 0, dev = f_list; i < MAXFUSES; i++, dev++)
                if (dev->d_type != EMPTY && func == dev->d_func)
                        return dev;
        return NULL;
}

/*
 * daemon:
 *      Start a daemon, takes a function.
 */

daemon(dfunc, arg, type)
reg VOID  *arg;
reg int type, (*dfunc)();
{
        reg struct delayed_action *dev;

        dev = d_slot();
        if (dev != NULL) {
                dev->d_type = type;
                dev->d_func = dfunc;
                dev->d_arg = arg;
                dev->d_time = DAEMON;
                demoncnt += 1;                  /* update count */
        }
}

/*
 * kill_daemon:
 *      Remove a daemon from the list
 */

kill_daemon(dfunc)
reg int (*dfunc)();
{
        reg struct delayed_action *dev;
        reg int i;

        for (i = 0, dev = d_list; i < MAXDAEMONS; i++, dev++) {
                if (dev->d_type != EMPTY && dfunc == dev->d_func)
                        break;
        }
        if (i >= MAXDAEMONS) return; /* if not found, forget it */
        /*
         * Take it out of the list
         */
        dev->d_type = EMPTY;
		dev->d_arg  = NULL;
		dev->d_func = NULL;
		dev->d_time = 0;

        demoncnt -= 1;                  /* update count */
}

/*
 * do_daemons:
 *      Run all the daemons that are active with the current flag,
 *      passing the argument to the function.
 */

do_daemons(flag)
reg int flag;
{
        struct delayed_action *dev;
        int i;

        /*
         * Loop through the devil list
         */
        for (i = 0; i < MAXDAEMONS; i++)
        {
            dev = &d_list[i];
        /*
         * Executing each one, giving it the proper arguments
         */
                if ((dev->d_type == flag) && (dev->d_time == DAEMON) && (dev->d_func != NULL))
                        (*dev->d_func)(dev->d_arg);
        }
}

/*
 * fuse:
 *      Start a fuse to go off in a certain number of turns
 */

fuse(dfunc, arg, time, type)
VOID *arg;
reg int (*dfunc)(), time, type;
{
        reg struct delayed_action *wire;

        wire = f_slot();
        if (wire != NULL) {
                wire->d_type = type;
                wire->d_func = dfunc;
                wire->d_arg = arg;
                wire->d_time = time;
                fusecnt += 1;                   /* update count */
        }
}

/*
 * lengthen:
 *      Increase the time until a fuse goes off
 */

lengthen(dfunc, xtime)
reg int (*dfunc)(), xtime;
{
        reg struct delayed_action *wire;

        if ((wire = find_slot(dfunc)) == NULL)
                return;
        wire->d_time += xtime;
}

/*
 * extinguish:
 *      Put out a fuse
 */

extinguish(dfunc)
reg int (*dfunc)();
{
        reg struct delayed_action *wire;

        if ((wire = find_slot(dfunc)) == NULL)
                return;
        wire->d_type = EMPTY;
		wire->d_func = NULL;
		wire->d_arg = NULL;
		wire->d_time = 0;
        fusecnt -= 1;
}

/*
 * do_fuses:
 *      Decrement counters and start needed fuses
 */

do_fuses(flag)
reg int flag;
{
        struct delayed_action *wire;
        int i;

        /*
         * Step though the list
         */
        for (i = 0; i < MAXFUSES; i++) {
            wire = &f_list[i];
        /*
         * Decrementing counters and starting things we want.  We also need
         * to remove the fuse from the list once it has gone off.
         */
            if(flag == wire->d_type && wire->d_time > 0 &&
              --wire->d_time == 0) {
                wire->d_type = EMPTY;
				if (wire->d_func != NULL)
					(*wire->d_func)(wire->d_arg);
                fusecnt -= 1;
            }
        }
}

/*
 * activity:
 *      Show wizard number of demaons and memory blocks used
 */

activity()
{
        msg("Daemons = %d : Fuses = %d : Memory Items = %d : Memory Used = %d",
            demoncnt,fusecnt,total,md_memused());
}

