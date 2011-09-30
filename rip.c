/*
    rip.c - File for the fun ends Death or a total win
    
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

#define REALLIFE 1      /* Print out machine and logname */
#define EDITSCORE 2     /* Edit the current score file */
#define ADDSCORE 3      /* Add a new score */

#include <curses.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>
#include <sys/types.h>
#include <fcntl.h>
#include "mach_dep.h"
#include "network.h"
#include "rogue.h"

/* Network machines (for mutual score keeping) */
struct network Network[] = {
    { "", "" },
};

static char *rip[] = {
"                      ___________",
"                     /           \\",
"                    /             \\",
"                   /    R. I. P.   \\",
"                  /                 \\",
"                 /                   \\",
"                 |                   |",
"                 |                   |",
"                 |     killed by     |",
"                 |                   |",
"                 |                   |",
"                 |                   |",
"                *|      *  *  *      |*",
"       _________)|//\\\\///\\///\\//\\//\\/|(_________",
NULL
};

char    *killname();

/*UNUSED*/
void
byebye(sig)
int sig;
{
	NOOP(sig);
    exit_game(EXIT_ENDWIN);
}


/*
 * death:
 *      Do something really fun when he dies
 */

death(monst)
register short monst;
{
    register char **dp = rip, *killer;
    register struct tm *lt;
    time_t date;
    char buf[LINELEN];
    struct tm *localtime();

    time(&date);
    lt = localtime(&date);
    clear();
    move(8, 0);
    while (*dp)
        printw("%s\n", *dp++);
    mvaddstr(14, 28-((strlen(whoami)+1)/2), whoami);
    sprintf(buf, "%lu Points", pstats.s_exp );
    mvaddstr(15, 28-((strlen(buf)+1)/2), buf);
    killer = killname(monst);
    mvaddstr(17, 28-((strlen(killer)+1)/2), killer);
    mvaddstr(18, 25, (sprintf(prbuf, "%4d", 1900+lt->tm_year), prbuf));
    move(lines-1, 0);
    refresh();
    score(pstats.s_exp, KILLED, monst);
    exit_game(EXIT_ENDWIN);
}

char *
killname(monst)
register short monst;
{
    static char mons_name[LINELEN/2];
    int i;

    if (monst > NUMMONST) return("a strange monster");

    if (monst >= 0) {
        switch (monsters[monst].m_name[0]) {
            case 'a':
            case 'e':
            case 'i':
            case 'o':
            case 'u':
                sprintf(mons_name, "an %s", monsters[monst].m_name);
                break;
            default:
                sprintf(mons_name, "a %s", monsters[monst].m_name);
        }
        return(mons_name);
    }
    for (i = 0; i< DEATHNUM; i++) {
        if (deaths[i].reason == monst)
            break;
    }
    if (i >= DEATHNUM)
        return ("strange death");
    return (deaths[i].name);
}

/*
 * score -- figure score and post it.
 */

/* VARARGS2 */
score(amount, flags, monst)
unsigned long amount;
int flags;
short monst;
{
    struct sc_ent top_ten[NUMSCORE];
    register struct sc_ent *scp;
    register int i;
    register struct sc_ent *sc2;
    register FILE *outf;
    register char *killer;
    register int prflags = 0;
    short upquest=0, wintype=0, uplevel=0, uptype=0;    /* For network updating */
    char upsystem[SYSLEN], uplogin[LOGLEN];
    char *thissys;      /* Holds the name of this system */

#define REASONLEN 3
    static char *reason[] = {
        "killed",
        "quit",
        "A total winner",
        "somehow left",
    };
    char *packend;

    memset(top_ten,0,sizeof(top_ten));

    signal(SIGINT, byebye);
    if (level == 0 && max_level == 0) 
        amount = 0; /*don't count if quit early */
    if (flags != WINNER && flags != SCOREIT && flags != UPDATE) {
        if (flags == CHICKEN) {
            packend = "when you quit";
        amount = amount / 100;
        }
        else
            packend = "at your untimely demise";
        mvaddstr(lines - 1, 0, retstr);
        refresh();
        getstr(prbuf);
        showpack(packend);
    }
    purse = 0;  /* Steal all the gold */

    /*
     * Open file and read list
     */

    if ((outf = fopen(score_file, "rb+")) == NULL)
    {
        if ((outf = fopen(score_file, "wb+")) == NULL)
        {
            mvprintw(lines - 1, 0, "Unable to open or create score file: %s",score_file);
            refresh();
            return;
        }
    }

	thissys = md_gethostname();

    /*
     * If this is a SCOREIT optin (rogue -s), don't call byebye.  The
     * endwin() calls in byebye() will and this results in a core dump.
     */
    if (flags == SCOREIT) signal(SIGINT, SIG_DFL);
    else signal(SIGINT, byebye);

    if (flags != SCOREIT && flags != UPDATE)
    {
        mvaddstr(lines - 1, 0, retstr);
        refresh();
        fflush(stdout);
        getstr(prbuf);
    }

    /* Check for special options */
    if (strcmp(prbuf, "names") == 0)
        prflags = REALLIFE;
    else if (wizard) {
        if (strcmp(prbuf, "edit") == 0) prflags = EDITSCORE;
        else if (strcmp(prbuf, "add") == 0) {
            prflags = ADDSCORE;
            waswizard = FALSE;  /* We want the new score recorded */
        }
    }

    /* Read the score and convert it to a compatible format */

    fseek(outf, 0, SEEK_SET);
    rs_read_scorefile(outf, top_ten, NUMSCORE);

    /* Get some values if this is an update */
    if (flags == UPDATE) {
        int errcheck, errors = 0;

        upquest = (short) netread(&errcheck, sizeof(short), stdin);
        if (errcheck) errors++;

        if (fread(whoami, 1, NAMELEN, stdin) != NAMELEN) errors++;

        wintype = (short) netread(&errcheck, sizeof(short), stdin);
        if (errcheck) errors++;

        uplevel = (short) netread(&errcheck, sizeof(short), stdin);
        if (errcheck) errors++;

        uptype = (short) netread(&errcheck, sizeof(short), stdin);
        if (errcheck) errors++;

        if (fread(upsystem, 1, SYSLEN, stdin) != SYSLEN)
                errors++;
        if (fread(uplogin, 1, LOGLEN, stdin) != LOGLEN)
                errors++;
        
        if (errors) {
            fclose(outf);
            return;
        }
    }

    /*
     * Insert player in list if need be
     */
    if (!waswizard) {
        char *login= NULL;

        if (flags != UPDATE) {
			login = md_getusername();
            
            if ((login == NULL) || (*login == 0))
                login = "another rogue fiend";
        }

        if (flags == UPDATE)
            (void) update(top_ten, amount, upquest, whoami, wintype,
                   uplevel, monst, uptype, upsystem, uplogin);
        else {
            if (prflags == ADDSCORE) {  /* Overlay characteristic by new ones */
                char buffer[LINELEN];

                clear();
                mvaddstr(1, 0, "Score: ");
                mvaddstr(2, 0, "Quest (number): ");
                mvaddstr(3, 0, "Name: ");
                mvaddstr(4, 0, "System: ");
                mvaddstr(5, 0, "Login: ");
                mvaddstr(6, 0, "Level: ");
                mvaddstr(7, 0, "Char type: ");
                mvaddstr(8, 0, "Result: ");

                /* Get the score */
                move(1, 7);
                get_str(buffer, stdscr);
                amount = atol(buffer);

                /* Get the character's quest -- must be a number */
                move(2, 16);
                get_str(buffer, stdscr);
                quest_item = atoi(buffer);

                /* Get the character's name */
                move(3, 6);
                get_str(buffer, stdscr);
                strncpy(whoami, buffer, NAMELEN);

                /* Get the system */
                move(4, 8);
                get_str(buffer, stdscr);
                strncpy(thissys, buffer, SYSLEN);

                /* Get the login */
                move(5, 7);
                get_str(buffer, stdscr);
                strncpy(login, buffer, LOGLEN);

                /* Get the level */
                move(6, 7);
                get_str(buffer, stdscr);
                level = max_level = (short) atoi(buffer);

                /* Get the character type */
                move(7, 11);
                get_str(buffer, stdscr);
                for (i=0; i<NUM_CHARTYPES; i++) {
                    if (EQSTR(buffer, char_class[i].name, strlen(buffer)))
                        break;
                }
                player.t_ctype = i;

                /* Get the win type */
                move(8, 8);
                get_str(buffer, stdscr);
                switch (buffer[0]) {
                    case 'W':
                    case 'w':
                    case 'T':
                    case 't':
                        flags = WINNER;
                        break;

                    case 'Q':
                    case 'q':
                        flags = CHICKEN;
                        break;

                    case 'k':
                    case 'K':
                    default:
                        flags = KILLED;
                        break;
                }

                /* Get the monster if player was killed */
                if (flags == KILLED) {
                    mvaddstr(9, 0, "Death type: ");
                    get_str(buffer, stdscr);
                    if (buffer[0] == 'M' || buffer[0] == 'm')
                        do {
                            monst = makemonster(TRUE, "choose");
                        } while (monst < 0); /* Force a choice */
                    else monst = getdeath();
                }
            }

            if (update(top_ten, amount, (short) quest_item, whoami, flags,
                    (flags == WINNER) ? (short) max_level : (short) level,
                    monst, player.t_ctype, thissys, login)
                ) {
                /* Send this update to the other systems in the network */
                int i, j;
                char cmd[256];  /* Command for remote execution */
                FILE *rmf, *popen();    /* For input to remote command */

                for (i=0; Network[i].system[0] != 0; i++)
                    if (Network[i].system[0] != '!' &&
                        strcmp(Network[i].system, thissys)) {
                        sprintf(cmd, NETCOMMAND,
                                Network[i].system, Network[i].rogue);

                        /* Execute the command */
                        if ((rmf=popen(cmd, "w")) != NULL) {
                            unsigned long temp; /* Temporary value */

                            /* Write out the parameters */
                            (void) netwrite((unsigned long) amount,
                                          sizeof(unsigned long), rmf);

                            (void) netwrite((unsigned long) monst,
                                          sizeof(short), rmf);

                            (void) netwrite((unsigned long) quest_item,
                                        sizeof(short), rmf);

                            (void) fwrite(whoami, 1, strlen(whoami), rmf);
                            for (j=strlen(whoami); j<NAMELEN; j++)
                                putc('\0', rmf);

                            (void) netwrite((unsigned long) flags,
                                          sizeof(short), rmf);

                            temp = (unsigned long)
                                (flags==WINNER ? max_level : level);
                            (void) netwrite(temp, sizeof(short), rmf);

                            (void) netwrite((unsigned long) player.t_ctype,
                                          sizeof(short), rmf);

                            (void) fwrite(thissys, 1,
                                                strlen(thissys), rmf);
                            for (j=strlen(thissys); j<SYSLEN; j++)
                                putc('\0', rmf);

                            (void) fwrite(login, 1, strlen(login), rmf);
                            for (j=strlen(login); j<LOGLEN; j++)
                                putc('\0', rmf);

                            /* Close off the command */
                            (void) pclose(rmf);
                        }
                    }
            }
        }
    }

    /*
     * SCOREIT -- rogue -s option.  Never started curses if this option.
     * UPDATE -- network scoring update.  Never started curses if this option.
     * EDITSCORE -- want to delete or change a score.
     */
/*   if (flags != SCOREIT && flags != UPDATE && prflags != EDITSCORE)
        endwin();       */

    if (flags != UPDATE) {
        if (flags != SCOREIT) {
            clear();
            refresh();
            endwin();
        }
        /*
        * Print the list
        */
        printf("\nTop %d Adventurers:\nRank     Score\tName\n",
                NUMSCORE);
        for (scp = top_ten; scp < &top_ten[NUMSCORE]; scp++) {
            char *class;

            if (scp->sc_score != 0) {
                class = char_class[scp->sc_ctype].name;

                /* Make sure we have an in-bound reason */
                if (scp->sc_flags > REASONLEN) scp->sc_flags = REASONLEN;

                printf("%3d %10lu\t%s (%s)", scp - top_ten + 1,
                    scp->sc_score, scp->sc_name, class);
                    
                if (prflags == REALLIFE) printf(" [in real life %.*s!%.*s]",
                                SYSLEN, scp->sc_system, LOGLEN, scp->sc_login);

                printf(":\n\t\t%s on level %d", reason[scp->sc_flags],
                            scp->sc_level);

                switch (scp->sc_flags) {
                    case KILLED:
                        printf(" by");
                        killer = killname(scp->sc_monster);
                        printf(" %s", killer);
                        break;

                    case WINNER:
                        printf(" with the %s",
                                rel_magic[scp->sc_quest].mi_name);
                        break;
                }

                if (prflags == EDITSCORE)
                {
                    fflush(stdout);
                    getstr(prbuf);
                    printf("\n");
                    if (prbuf[0] == 'd') {
                        for (sc2 = scp; sc2 < &top_ten[NUMSCORE-1]; sc2++)
                            *sc2 = *(sc2 + 1);
                        top_ten[NUMSCORE-1].sc_score = 0;
                        for (i = 0; i < NAMELEN; i++)
                            top_ten[NUMSCORE-1].sc_name[i] = rnd(255);
                        top_ten[NUMSCORE-1].sc_flags = RN;
                            top_ten[NUMSCORE-1].sc_level = RN;
                            top_ten[NUMSCORE-1].sc_monster = RN;
                            scp--;
                    }
                    else if (prbuf[0] == 'e') {
                        printf("Death type: ");
                        getstr(prbuf);
                        if (prbuf[0] == 'M' || prbuf[0] == 'm')
                            do {
                                scp->sc_monster =
                                    makemonster(TRUE, "choose");
                            } while (scp->sc_monster < 0); /* Force a choice */
                        else scp->sc_monster = getdeath();
                        clear();
                        refresh();
                    }
                }
                else printf("\n");
            }
        }
/*      if (prflags == EDITSCORE) endwin();*/     /* End editing windowing */
    }
    fseek(outf, 0L, SEEK_SET);

    if (flags != SCOREIT)
        rs_write_scorefile(outf,top_ten,NUMSCORE);
    fclose(outf);
}

/*
 * showpack:
 *      Display the contents of the hero's pack
 */

showpack(howso)
char *howso;
{
        reg char *iname;
        reg int cnt, packnum;
        reg struct linked_list *item;
        reg struct object *obj;

        idenpack();
        cnt = 1;
        clear();
        mvprintw(0, 0, "Contents of your pack %s:\n",howso);
        packnum = 'a';
        for (item = pack; item != NULL; item = next(item)) {
                obj = OBJPTR(item);
                iname = inv_name(obj, FALSE);
                mvprintw(cnt, 0, "%c) %s\n",packnum++,iname);
                if (++cnt >= lines - 2 && 
                    next(item) != NULL) {
                        cnt = 1;
                        mvaddstr(lines - 1, 0, morestr);
                        refresh();
                        wait_for(' ');
                        clear();
                }
        }
        mvprintw(cnt + 1,0,"--- %ld  Gold Pieces ---",purse);
        refresh();
}

total_winner()
{
    register struct linked_list *item;
    register struct object *obj;
    register long worth;
    register char c;
    register long oldpurse;

    clear();
    standout();
    addstr("                                                               \n");
    addstr("  @   @               @   @           @          @@@  @     @  \n");
    addstr("  @   @               @@ @@           @           @   @     @  \n");
    addstr("  @   @  @@@  @   @   @ @ @  @@@   @@@@  @@@      @  @@@    @  \n");
    addstr("   @@@@ @   @ @   @   @   @     @ @   @ @   @     @   @     @  \n");
    addstr("      @ @   @ @   @   @   @  @@@@ @   @ @@@@@     @   @     @  \n");
    addstr("  @   @ @   @ @  @@   @   @ @   @ @   @ @         @   @  @     \n");
    addstr("   @@@   @@@   @@ @   @   @  @@@@  @@@@  @@@     @@@   @@   @  \n");
    addstr("                                                               \n");
    addstr("     Congratulations, you have made it to the light of day!    \n");
    standend();
    addstr("\nYou have joined the elite ranks of those who have escaped the\n");
    addstr("Dungeons of Doom alive.  You journey home and sell all your loot at\n");
    addstr("a great profit and are appointed ");
    switch (player.t_ctype) {
        case C_FIGHTER: addstr("Leader of the Fighter's Guild.\n");
        when C_RANGER:  addstr("King of the Northern Land.\n");
        when C_PALADIN: addstr("King of the Southern Land.\n");
        when C_MAGICIAN:addstr("High Wizard of the Sorcerer's Guild.\n");
        when C_CLERIC:  addstr("Bishop of the Monastery.\n");
        when C_THIEF:   addstr("Leader of the Thief's Guild.\n");
        when C_MONK:    addstr("Master of the Temple.\n");
        when C_ASSASSIN: addstr("Leader of the Assassin's Guild.\n");
        when C_DRUID:   addstr("High Priest of the Monastery.\n");
        otherwise:      addstr("Town Drunk in the Tavern.\n");
    }
    mvaddstr(lines - 1, 0, spacemsg);
    refresh();
    wait_for(' ');
    clear();
    mvaddstr(0, 0, "   Worth  Item");
    oldpurse = purse;
    for (c = 'a', item = pack; item != NULL; c++, item = next(item))
    {
        obj = OBJPTR(item);
        worth = get_worth(obj);
        if (obj->o_group == 0)
            worth *= obj->o_count;
        whatis(item);
        mvprintw(c-'a'+1, 0, "%c) %6ld  %s", c, worth, inv_name(obj, FALSE));
        purse += worth;
    }
    mvprintw(c - 'a' + 1, 0,"   %5ld  Gold Pieces          ", oldpurse);
    refresh();
    score(pstats.s_exp + (long) purse, WINNER, '\0');
    exit_game(EXIT_ENDWIN);
}


void
delete_score(top_ten, idx)
struct sc_ent top_ten[NUMSCORE];
int idx;
{
    for(;idx < NUMSCORE-1;idx++)
        top_ten[idx] = top_ten[idx+1];

    top_ten[NUMSCORE-1].sc_score = 0L;
}

int
insert_score(top_ten, sc)
struct sc_ent top_ten[NUMSCORE];
struct sc_ent *sc;
{
    int i,j;

    if (top_ten[NUMSCORE-1].sc_score > 0)
        return(-1); /* no room */
  
    for(i = 0; i < NUMSCORE; i++) {
        if (sc->sc_score > top_ten[i].sc_score) {
            for(j = NUMSCORE-1; j > i; j--)
                top_ten[j] = top_ten[j-1];
            top_ten[i] = *sc;
            return(i);
        }
    }

    return(-1);
}

/* PCS = player-class-system (used to determines uniqueness of player) */

int
is_pcs_match(sc1,sc2)
struct sc_ent *sc1;
struct sc_ent *sc2;
{
    return( (strcmp(sc1->sc_name,sc2->sc_name) == 0) &&
         (sc1->sc_ctype == sc2->sc_ctype) &&
         (strcmp(sc1->sc_system, sc2->sc_system)==0) );
}

int
count_pcs_matches(top_ten,sc,lowest)
struct sc_ent top_ten[NUMSCORE];
struct sc_ent *sc;
int *lowest;
{
    int i, matches = 0;

    *lowest = -1;

    for(i = 0; i < NUMSCORE; i++) {
        if (is_pcs_match(sc,&top_ten[i])) {
            matches++;
            *lowest = i;
        }
    }
    return(matches);
}

int
find_most_pcs_matches(top_ten,sc,num,idx)
struct sc_ent top_ten[NUMSCORE];
struct sc_ent *sc;
int *num, *idx;
{
    int i, matches, max_match=0, max_match_idx=-1, lowest;
    
    for(i = NUMSCORE-1; i > 0; i--) {
        matches = count_pcs_matches(top_ten,&top_ten[i],&lowest);

        if (matches > max_match) {
            max_match     = matches;
            max_match_idx = lowest;
        }
    }        

    matches = count_pcs_matches(top_ten,sc,&lowest) + 1;

    if (matches > max_match) {
        *num = matches;
        *idx = lowest;
    }
    else {
        *num = max_match;
        *idx = max_match_idx;
    }

    return(0);
}


int
add_score(top_ten,sc)
struct sc_ent top_ten[NUMSCORE];
struct sc_ent *sc;
{
    int idx, count;
      
    if (insert_score(top_ten,sc) == -1) { 
    /* Simple insert if space available in table */

        find_most_pcs_matches(top_ten,sc,&count,&idx);

        /* EVERY ENTRY UNIQUE,                  */
        /* INSERT IF SCORE > LOWEST MATCH SCORE */
        if (count == 1) {                               
            if (sc->sc_score > top_ten[idx].sc_score) {
                delete_score(top_ten,idx);
                insert_score(top_ten,sc);
            }
        }
        /* CURRENT PCS HAS HIGHEST DUPE COUNT   */
        /* INSERT IF SCORE > LOWEST MATCH SCORE */
        else if (is_pcs_match(sc,&top_ten[idx])) {
            if (sc->sc_score > top_ten[idx].sc_score) {
                delete_score(top_ten,idx);
                insert_score(top_ten,sc);
            }
        }
        /* UNRELATED PCS HAS HIGHEST DUPE COUNT         */
        /* DELETE LOWEST DUPE TO MAKE ROOM AND INSERT   */
        else {                                          
            delete_score(top_ten,idx); 
            insert_score(top_ten,sc);
        }
    }    
}

update(top_ten, amount, quest, whoami, flags, level, monst, ctype, system, login)
struct sc_ent top_ten[];
unsigned long amount;
short quest, flags, level, monst, ctype;
char *whoami, *system, *login;
{
    struct sc_ent sc;

    sc.sc_score = amount;
    sc.sc_quest = quest;
    strncpy(sc.sc_name, whoami, NAMELEN);
    sc.sc_name[NAMELEN-1] = 0;
    sc.sc_flags = flags;
    sc.sc_level = level;
    sc.sc_monster = monst;
    sc.sc_ctype = ctype;
    strncpy(sc.sc_system, system, SYSLEN);
    sc.sc_system[SYSLEN - 1] = 0;
    strncpy(sc.sc_login, login, LOGLEN);
    sc.sc_login[LOGLEN-1] = 0;

    add_score(top_ten, &sc);
    return(1);
}

