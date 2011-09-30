/*
    mach_dep.h  -  machine dependents

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

/*
 * define/undefine that the wizard commands exist
 */

#if defined(_WIN32)
#define fstat _fstat
#define stat _stat
#define open _open
#define popen _popen
#define pclose _pclose
#if !defined(__MINGW32__)
#define PATH_MAX _MAX_PATH
#endif
#endif

#define NOOP(x) (x += 0)

extern char *md_getusername();
extern char *md_gethomedir();
extern char *md_getroguedir();
extern void md_flushinp();
extern char *md_getshell();
extern char *md_gethostname();
extern void md_dobinaryio();
extern char *md_getpass();
extern void md_init();
extern char *xcrypt();

/*
 * define if you want to limit scores to one per class per userid
 */

/* #define LIMITSCORE 1*/
#undef LIMITSCORE

/* 
 * fudge factor allowed in time for saved game
 */

#define FUDGE_TIME      200
