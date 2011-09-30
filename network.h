/*
    network.h  -  networking setup
    
    XRogue: Expeditions into the Dungeons of Doom
    Copyright (C) 1991 Robert Pietkivitch
    All rights reserved.
    
    Based on "Advanced Rogue"
    Copyright (C) 1984, 1985 Michael Morgan, Ken Dalka and AT&T
    All rights reserved.

    See the file LICENSE.TXT for full copyright and licensing information.
*/

/*
 * Note that networking is set up for machines that can communicate
 * via some system such as uucp.  The mechanism listed here uses uux
 * and assumes that the target machine allows access to the game via
 * the uux command.  NETCOMMAND must be defined if networking is desired.
 */

/* #undef  NETCOMMAND "uux - -n '%s!%s -u' >/dev/null 2>&1" */
/* #define NETCOMMAND "usend -s -d%s -uNoLogin -!'%s -u' - 2>/dev/null" */
#define NETCOMMAND ""

/* Networking information -- should not vary among networking machines */

struct network {
    char *system;
    char *rogue;
};
extern struct network Network[];
extern unsigned long netread();
extern unsigned long netwrite();

