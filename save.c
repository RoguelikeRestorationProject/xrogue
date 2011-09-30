/*
    save.c - save and restore routines

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
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include "rogue.h"
#include "mach_dep.h"

extern char version[];
extern unsigned char encstr[];
extern int big_endian;

bool
save_game()
{
    register FILE *savef;
    register int c;
    char buf[LINELEN];

    /*
     * get file name
     */
    mpos = 0;
    if (file_name[0] != '\0')
    {
        msg("Save file (%s)? ", file_name);
        do
        {
            c = wgetch(cw);
            if (c == ESC) return(0);
        } while (c != 'n' && c != 'N' && c != 'y' && c != 'Y');
        mpos = 0;
        if (c == 'y' || c == 'Y')
        {
            msg("File name: %s", file_name);
            goto gotfile;
        }
    }
    else
        goto gotfile; /* must save to file restored from */

    do
    {
        msg("File name: ");
        mpos = 0;
        buf[0] = '\0';
        if (get_str(buf, msgw) == QUIT)
        {
            msg("");
            return FALSE;
        }
        strcpy(file_name, buf);
gotfile:

        if ((savef = fopen(file_name, "wb")) == NULL)
             msg(strerror(errno));
    } while (savef == NULL);

    msg("");
    /*
     * write out encrpyted file
     */
    if (save_file(savef) == FALSE) {
        fclose(savef);
        msg("Cannot create save file.");
        unlink(file_name);
        return(FALSE);
    }
    fclose(savef);
    return(TRUE);
}

/*
 * automatically save a file.  This is used if a HUP signal is recieved
 */

void
auto_save(int sig)
{
    register FILE *savef = NULL;
    register int i;
	NOOP(sig);
    for (i = 0; i < NSIG; i++)
        signal(i, SIG_IGN);
    if (file_name[0] != '\0'    && 
        pstats.s_hpt > 0        &&
        ((savef = fopen(file_name, "wb")) != NULL))
        save_file(savef);
    fclose(savef);
    exit_game(EXIT_ENDWIN);
}

/*
 * write the saved game on the file
 */

bool
save_file(savef)
register FILE *savef;
{
    int slines = LINES;
    int scols = COLS;
    int ret = FALSE;
    int endian = 0x01020304;
    big_endian = ( *((char *)&endian) == 0x01 );

    wmove(cw, LINES-1, 0);
    draw(cw);

    encwrite(version,strlen(version)+1,savef);
    rs_write_int(savef,slines);
    rs_write_int(savef,scols);

    ret = rs_save_file(savef);

    return(ret);
}

restore(file, envp)
register char *file;
char **envp;
{
    register int inf;
    extern char **environ;
    char buf[LINELEN];
    int endian = 0x01020304;
    big_endian = ( *((char *)&endian) == 0x01 );

    if (strcmp(file, "-r") == 0)
        file = file_name;

    if ((inf = open(file, O_RDONLY)) < 0)
    {
        perror(file);
        return(-1);
    }

    fflush(stdout);

    encread(buf, strlen(version) + 1, inf);

    if (strcmp(buf, version) != 0)
    {
        printf("Sorry, saved game is out of date.\n");
        return FALSE;
    }

    fflush(stdout);

    rs_read_int(inf,&lines);
    rs_read_int(inf,&cols);

    initscr();

    if (lines > LINES)
    {
        endwin();
        printf("Sorry, original game was played on a screen with %d lines.\n",lines);
        printf("Current screen only has %d lines. Unable to restore game\n",LINES);
        return(FALSE);
    }
    if (cols > COLS)
    {
        endwin();
        printf("Sorry, original game was played on a screen with %d columns.\n",cols);
        printf("Current screen only has %d columns. Unable to restore game\n",COLS);
        return(FALSE);
    }

    typeahead(-1);
    setup();
    cw = newwin(LINES, COLS, 0, 0);
    mw = newwin(LINES, COLS, 0, 0);
    hw = newwin(LINES, COLS, 0, 0);
    msgw = newwin(4, cols, 0, 0);

    keypad(cw, TRUE);
    keypad(hw, TRUE);

    if (rs_restore_file(inf) == FALSE)
    {
        endwin();
        printf("Cannot restore file\n");
        close(inf);
        return(FALSE);
    }

    close(inf);

    if (!wizard)
        unlink(file);
 
    mpos = 0;
    environ = envp;
    strcpy(file_name, file);
    clearok(curscr, TRUE);
    touchwin(cw);
    wrefresh(cw);
    msg("Welcome back!  --More-- ");
    wait_for(' ');
    msg("");
    playit();

    /*NOTREACHED*/
	return(1);
}

#define ENCWBSIZ        1024

/*
 * perform an encrypted write
 */

long
encwrite(start, size, outf)
register char *start;
register unsigned long size;
register FILE *outf;
{
    register unsigned char *ep;
    register int i = 0;
    unsigned long num_written = 0;
    char buf[ENCWBSIZ];
    int ret;

    ep = encstr;

    while (size--)
    {
        buf[i++] = *start++ ^ *ep++;
        if (*ep == '\0')
           ep = encstr;

        if (i == ENCWBSIZ || size == 0)
        {
            ret = (unsigned int) fwrite(buf, 1, i, outf);
            if (ret > 0)
               num_written += ret;

            if (ret < i)
                 return(num_written);

            i = 0;
        }
    }
    return(num_written);
}

#define ENCRBSIZ        32768

/*
 * perform an encrypted read
 */

long
encread(start, size, inf)
register char *start;
register unsigned long size;
int inf;
{
    register unsigned char *ep;
    register int rd_siz;
    register unsigned long total_read;

    total_read = 0;
    while (total_read < size) {
        rd_siz = ENCRBSIZ;
        rd_siz = ((size-total_read) > ENCRBSIZ) ? ENCRBSIZ : (size-total_read);
        rd_siz = read(inf,&start[total_read],rd_siz);
        if(rd_siz==-1 || rd_siz==0)
                break;
        total_read += rd_siz;
    }
    ep = encstr;

    size = total_read;
    while (size--)
    {
        *start++ ^= *ep++;
        if (*ep == '\0')
            ep = encstr;
    }
    return total_read;
}

