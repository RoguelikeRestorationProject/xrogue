/*
    main.c  -  setup code
    
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
#include <signal.h>
#include <time.h>

#include "mach_dep.h"
#include "network.h"
#include "rogue.h"

main(argc, argv, envp)
char **argv;
char **envp;
{
    register char *env;
    time_t now;

    md_init();

    /*
     * get home and options from environment
     */

    strncpy(home, md_gethomedir(), LINELEN);

    /* Get default save file */
    strcpy(file_name, home);
    strcat(file_name, "xrogue.sav");

    /* Get default score file */
    strcpy(score_file, md_getroguedir());

    if (*score_file)
        strcat(score_file,"/");

    strcat(score_file, "xrogue.scr");

    if ((env = getenv("ROGUEOPTS")) != NULL)
        parse_opts(env);
    
    if (whoami[0] == '\0')
        strucpy(whoami, md_getusername(), strlen(md_getusername()));

    /*
     * check for print-score option
     */
    if (argc == 2 && strcmp(argv[1], "-s") == 0)
    {
        waswizard = TRUE;
        score((long)0, SCOREIT, (short)0);
        exit_game(0);
    }

    /*
     * Check for a network update
     */
    if (argc == 2 && strcmp(argv[1], "-u") == 0) {
        int errcheck, errors = 0;
        unsigned long amount;
        short monster;

        /* Read in the amount and monster values to pass to score */
        amount = netread(&errcheck, sizeof(unsigned long), stdin);
        if (errcheck) errors++;

        monster = (short) netread(&errcheck, sizeof(short), stdin);
        if (errcheck) errors++;

        /* Now do the update if there were no errors */
        if (errors) exit_game(0);
        else {
            score((long)amount, UPDATE, (short)monster);
            exit_game(0);
        }
    }

    /*
     * Check to see if he is a wizard
     */
    if (argc >= 2 && argv[1][0] == '\0')
        if (strcmp(PASSWD, xcrypt(md_getpass("Wizard's password: "), "mT")) == 0)
        {
            wizard = TRUE;
            argv++;
            argc--;
        }

    if (betaover())
    {
        printf("Sorry, %s, but the test period of this prerelease version\n",whoami);
        printf("of xrogue is over. Please acquire a new version. Sorry.\n");
        exit_game(0);
    }

    if (!wizard && !author() && !playtime()) {
        printf("Sorry, %s, but you can't play during working hours.\n", whoami);
        printf("Try again later.\n");
        exit_game(0);
    }
    if (!wizard && !author() && too_much()) {
        printf("Sorry, %s, but the system is too loaded now.\n", whoami);
        printf("Try again later.\n");
        exit_game(0);
    }

    if (argc == 2)
        if (!restore(argv[1], envp)) /* Note: restore will never return */
            exit_game(0);

    if (wizard && getenv("SEED") != NULL) {
        seed = atoi(getenv("SEED")); 
    }
    else {
        seed = (int) time(&now) + getpid();
    }
    if (wizard)
        printf("Hello %s, welcome to dungeon #%d", whoami, seed);
    else
        printf("Hello %s, just a moment while I dig the dungeon...", whoami);
    fflush(stdout);
   
    md_srand(seed);

    init_things();                      /* Set up probabilities of things */
    init_colors();                      /* Set up colors of potions */
    init_stones();                      /* Set up stone settings of rings */
    init_materials();                   /* Set up materials of wands */
    init_names();                       /* Set up names of scrolls */
    init_misc();                        /* Set up miscellaneous magic */
    init_foods();                       /* set up the food table */

    initscr();                          /* Start up cursor package */

    typeahead(-1);          /* turn off 3.2/4.0 curses feature */

    if (COLS < MINCOLS)
    {
        printf("\n\nSorry, %s, but your terminal window has too few columns.\n", whoami);
        printf("Your terminal has %d columns, needs 70.\n",COLS);
        byebye(0);
    }
    if (LINES < MINLINES)
    {
        printf("\n\nSorry, %s, but your terminal window has too few lines.\n", whoami);
        printf("Your terminal has %d lines, needs 22.\n",LINES);
        byebye(0);
    }
    
    cols  = COLS;
    lines = LINES;

    if ( cols % 2 != 0)  cols -=1;          /* must be even for maze code */
    if (lines % 2 != 0) lines -=1;          /* must be even for maze code */

    /*
     * Now that we have cols and lines, we can update our window
     * structure for non-hardware windows.
     */
    setup();
    /*
     * Set up windows
     */
    cw = newwin(lines, cols, 0, 0);
    mw = newwin(lines, cols, 0, 0);
    hw = newwin(lines, cols, 0, 0);
    msgw = newwin(4, cols, 0, 0);
    if (cw == NULL || hw == NULL || mw == NULL || msgw == NULL) {
        exit_game(EXIT_CLS | EXIT_ENDWIN);
    }

    keypad(cw, TRUE);
    keypad(hw, TRUE);

    init_player();                      /* Roll up the rogue */
    waswizard = wizard;

    draw(cw);
    /* A super wizard doesn't have to get equipped */
    /* Check if "" option is TRUE and get environment flag */
    if (wizard && strcmp(getenv("SUPER"),"YES") == 0 ||
    def_attr == TRUE) {
        level = 1;
        new_level(NORMLEV);
    }
    else 
        new_level(STARTLEV);            /* Draw current level */

    /*
     * Start up daemons and fuses
     */
    daemon(doctor, &player, AFTER);
    fuse(swander, (VOID *)NULL, WANDERTIME, AFTER);
    /* Give characters their innate abilities */
    if (player.t_ctype == C_MAGICIAN || player.t_ctype == C_RANGER)
            fuse(spell_recovery, (VOID *)NULL, SPELLTIME, AFTER);
    if (player.t_ctype == C_DRUID    || player.t_ctype == C_MONK)
            fuse(chant_recovery, (VOID *)NULL, SPELLTIME, AFTER);
    if (player.t_ctype == C_CLERIC   || player.t_ctype == C_PALADIN)
            fuse(prayer_recovery, (VOID *)NULL, SPELLTIME, AFTER);
    daemon(stomach, (VOID *)NULL, AFTER);
    if (player.t_ctype == C_THIEF    ||
        player.t_ctype == C_ASSASSIN ||
        player.t_ctype == C_MONK)
            daemon(trap_look, (VOID *)NULL, AFTER);

    /* Does this character have any special knowledge? */
    switch (player.t_ctype) {
        case C_ASSASSIN:
            /* Assassins automatically recognize poison */
            p_know[P_POISON] = TRUE;
        when C_FIGHTER:
            /* Fighters automatically recognize skill */
            p_know[P_SKILL] = TRUE;
    }

    /* Choose an initial quest item */
    if (!wizard) {
    if (def_attr == FALSE)
        quest_item = rnd(MAXRELIC);
    }
    mpos = 0;
    draw(cw);
    msg("You have been quested to retrieve the %s....",
         rel_magic[quest_item].mi_name);
    mpos = 0;
    playit();
}

/*
 * endit:
 *      Exit the program abnormally.
 */

/*UNUSED*/
void
endit(sig)
int sig;
{
	NOOP(sig);
    fatal("Ok, if you want to exit that badly, I'll have to allow it\n");
}

/*
 * fatal:
 *      Exit the program, printing a message.
 */

fatal(s)
char *s;
{
    clear();
    move(lines-2, 0);
    printw("%s", s);
    draw(stdscr);
    printf("\n");       /* So the curser doesn't stop at the end of the line */
    exit_game(EXIT_ENDWIN);
}

/*
 * rnd:
 *      Pick a very random number.
 */

rnd(range)
register int range;
{
    return( md_rand(range) );
}

/*
 * roll:
 *      roll a number of dice
 */

roll(number, sides)
register int number, sides;
{
    register int dtotal = 0;

    while(number--)
        dtotal += rnd(sides)+1;
    return dtotal;
}

setup()
{
	md_setup();
}

/*
 * playit:
 *      The main loop of the program.  Loop until the game is over,
 * refreshing things and looking at the proper times.
 */

playit()
{
    register char *opts;

    /*
     * parse environment declaration of options
     */
    if ((opts = getenv("ROGUEOPTS")) != NULL)
        parse_opts(opts);

    player.t_oldpos = hero;
    oldrp = roomin(&hero);
    after = TRUE;
    command();                  /* Command execution */
    endit(-1);
}

/*
 * see if the system is being used too much for this game
 */

too_much()
{
    /* we no longer do load checking or user counts */
    return(FALSE);
}

/*
 * author:
 *      See if a user is an author of the program
 */

author()
{
        switch (md_getuid()) {
                case 0: /* always OK for root to play */
                        return TRUE;
                default:
                        return FALSE;
        }
}

/*
 * playtime:
 *      Returns TRUE when it is a good time to play rogue
 */

playtime()
{
        /* we no longer do playtime checking */

        return TRUE;
}

/*
 * betaover:
 *      Returns TRUE if the test period of this version of the game is over
 */

betaover()
{
     return(FALSE);
}


exit_game(flag)
int flag;
{
    int i;

    if (flag & EXIT_CLS)  /* Clear Screen    */
    {
        wclear(cw);
        draw(cw);
    }

    if (flag & EXIT_ENDWIN)  /* Shutdown Curses */
    {
        keypad(cw,FALSE);
        keypad(hw,FALSE);
        delwin(cw);
        delwin(mw);
        delwin(hw);
        delwin(msgw);
        if (!isendwin())
            endwin();
    }
    o_free_list(player.t_pack);
    t_free_list(mlist);
    t_free_list(rlist);
    t_free_list(tlist);
    o_free_list(lvl_obj);               /* Free up previous objects (if any) */
    for (i = 0; i < MAXROOMS; i++)
    {
        r_free_list(rooms[i].r_exit);        /* Free up the exit lists */ 
        _r_free_fire_list(&rooms[i].r_fires);
    }

    for(i=0; i<MAXSCROLLS; i++)
    {
        if (s_names[i] != NULL)
            free( s_names[i] );
        if (s_guess[i] != NULL)
            free( s_guess[i] );
    }

    for(i=0; i<MAXPOTIONS; i++)
    {
        if (p_guess[i] != NULL)
            free( p_guess[i] );
    }

    for(i=0; i<MAXRINGS; i++)
    {
        if (r_guess[i] != NULL)
            free( r_guess[i] );
    }

    for(i=0; i<MAXSTICKS; i++)
    {
        if (ws_guess[i] != NULL)
            free( ws_guess[i] );
    }

    exit(0);
}

