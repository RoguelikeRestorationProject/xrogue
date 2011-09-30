/*
    rogue.h - Rogue definitions and variable declarations
    
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
 * some compiler don't handle void pointers well so
 */
#include <assert.h>
#define reg  register
#define VOID long
#undef lines
#define ENCREAD encread
#define ENCWRITE encwrite
#undef SCROLL   /* UNIX/370 defines SCROLL for some bizarre reason */
#define exfork fork     /* Standard fork with no paging available */

/*
 * Maximum number of different things
 */

#define MINCOLS         70
#define MINLINES        22
#define MAXROOMS        9
#define MAXTHINGS       9
#define MAXOBJ          9
#define MAXSTATS        74      /* max total of all stats at startup */
#define MAXPACK         27      /* max number of items in pack */
#define MAXDOUBLE       14      /* max number of times exppts is doubled */
#define MAXCONTENTS     20      /* max number of things beaker/book can hold */
#define MAXENCHANT      30      /* max number of enchantments on an item */
#define MAXTREAS        25      /* number monsters/treasure in treasure room */
#define MAXTRAPS        20      /* max number of traps that may be on level */
#define MAXTRPTRY       15      /* attempts/level allowed for setting traps */
#define MAXDOORS        4       /* maximum doors to a room */
#define MAXCHANTS       16      /* maximum number of chants for a druid */
#define MAXPRAYERS      16      /* maximum number of prayers for cleric */
#define MAXSPELLS       16      /* maximum number of spells for magician */
#define MAXQUILL        14      /* scrolls the Quill of Nagrom can write */
#define QUILLCHARGES    300     /* max num of charges in the Quill of Nagrom */
#define NUM_CNAMES      26      /* number of names per character level */
#define NUMMONST        211     /* current number of monsters */
#define NUMUNIQUE       60      /* number of UNIQUEs (minus jacaranda) */
#define NUMDINOS        30      /* number of dinosaurs (for OUTSIDE level) */
#define NLEVMONS        3       /* number of new monsters per level */
#define NUMSCORE        20      /* number of entries in score file */
#define HARDER          40      /* at this level start making things harder */
#define LINELEN         256     /* characters in a buffer */
#define JUG_EMPTY       -1      /* signifys that the alchemy jug is empty */
#define MAXPURCH        (pstats.s_charisma/3) /* num of purchases at t.post */
#define MAXATT          50      /* charactor's attribute maximum number */

/* Movement penalties */
#define BACKPENALTY 3
#define SHOTPENALTY 2           /* In line of sight of missile */
#define DOORPENALTY 1           /* Moving out of current room */

/*
 * these defines are used in calls to get_item() to signify what
 * it is we want
 */

#define ALL             -1
#define WEARABLE        -2
#define CALLABLE        -3
#define WIELDABLE       -4
#define USEABLE         -5
#define IDENTABLE       -6
#define REMOVABLE       -7
#define PROTECTABLE     -8
#define ZAPPABLE        -9
#define READABLE        -10
#define QUAFFABLE       -11

/*
 * stuff to do with encumberance
 */

#define NORMENCB        1400    /* normal encumberance */
#define F_SATIATED       0      /* player's stomach is very full */
#define F_OKAY           1      /* have plenty of food in stomach */
#define F_HUNGRY         2      /* player is hungry */
#define F_WEAK           3      /* weak from lack of food */
#define F_FAINT          4      /* fainting from lack of food */

/*
 * actions a player/monster will take
 */

#define A_MOVE          0200    /* normal movement */
#define A_FREEZE        0201    /* frozen in place */
#define A_ATTACK        0202    /* trying to hit */
#define A_SELL          0203    /* trying to sell goods */
#define A_NIL           0204    /* not doing anything */
#define A_BREATHE       0205    /* breathing */
#define A_MISSILE       0206    /* Firing magic missiles */
#define A_SONIC         0207    /* Sounding a sonic blast */
#define A_SUMMON        0210    /* Summoning help */
#define A_USERELIC      0211    /* Monster uses a relic */
#define A_SLOW          0212    /* monster slows the player */
#define A_ZAP           0213    /* monster shoots a wand */
#define A_PICKUP        0214    /* player is picking something up */
#define A_USEWAND       0215    /* monster is shooting a wand */
#define A_THROW         't'
#define C_CAST          'C'
#define C_COUNT         '*'
#define C_DIP           'D'
#define C_DROP          'd'
#define C_EAT           'e'
#define C_PRAY          'p'
#define C_CHANT         'c'
#define C_QUAFF         'q'
#define C_READ          'r'
#define C_SEARCH        's'
#define C_SETTRAP       '^'
#define C_TAKEOFF       'T'
#define C_USE           CTRL('U')
#define C_WEAR          'W'
#define C_WIELD         'w'
#define C_ZAP           'z'

/* Possible ways for the hero to move */

#define H_TELEPORT 0

/*
 * return values for get functions
 */

#define NORM    0       /* normal exit */
#define QUIT    1       /* quit option setting */
#define MINUS   2       /* back up one option */

/* 
 * The character types
 */

#define C_FIGHTER       0
#define C_RANGER        1
#define C_PALADIN       2
#define C_MAGICIAN      3
#define C_CLERIC        4
#define C_THIEF         5
#define C_ASSASSIN      6
#define C_DRUID         7
#define C_MONK          8
#define C_MONSTER       9
#define NUM_CHARTYPES   10

/*
 * define the ability types
 */

#define A_INTELLIGENCE  0
#define A_STRENGTH      1
#define A_WISDOM        2
#define A_DEXTERITY     3
#define A_CONSTITUTION  4
#define A_CHARISMA      5
#define NUMABILITIES    6

/*
 * values for games end
 */

#define UPDATE  -2
#define SCOREIT -1
#define KILLED   0
#define CHICKEN  1
#define WINNER   2

/*
 * definitions for function step_ok:
 *      MONSTOK indicates it is OK to step on a monster -- it
 *      is only OK when stepping diagonally AROUND a monster;
 *      it is also OK if the stepper is a friendly monster and
 *      is in a fighting mood.
 */

#define MONSTOK 1
#define NOMONST 2
#define FIGHTOK 3

/*
 * used for ring stuff
 */

#define LEFT_1          0
#define LEFT_2          1
#define LEFT_3          2
#define LEFT_4          3
#define RIGHT_1         4
#define RIGHT_2         5
#define RIGHT_3         6
#define RIGHT_4         7
#define NUM_FINGERS     8

/*
 * used for micellaneous magic (MM) stuff
 */

#define WEAR_BOOTS      0
#define WEAR_BRACERS    1
#define WEAR_CLOAK      2
#define WEAR_GAUNTLET   3
#define WEAR_JEWEL      4
#define WEAR_NECKLACE   5
#define NUM_MM          6

/*
    How to exit flags:
*/

#define EXIT_CLS        1    /* Clear screen first */
#define EXIT_ENDWIN     2    /* Shutdown Curses    */


/*
 * All the fun defines
 */

#define next(ptr) (*ptr).l_next
#define prev(ptr) (*ptr).l_prev
#define ldata(ptr) (*ptr).l_data
#define inroom(rp, cp) (\
    (cp)->x <= (rp)->r_pos.x + ((rp)->r_max.x - 1) && (rp)->r_pos.x <= (cp)->x \
 && (cp)->y <= (rp)->r_pos.y + ((rp)->r_max.y - 1) && (rp)->r_pos.y <= (cp)->y)
#define winat(y, x) (mvwinch(mw, y, x)==' '?mvwinch(stdscr, y, x):winch(mw))
#define debug if (wizard) msg
#define RN (((seed = seed*11109+13849) & 0x7fff) >> 1)
#define unc(cp) (cp).y, (cp).x
#define cmov(xy) move((xy).y, (xy).x)
#define DISTANCE(y1, x1, y2, x2) ((x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1))
#define OBJPTR(what)    (struct object *)((*what).l_data)
#define THINGPTR(what)  (struct thing *)((*what).l_data)
#define DOORPTR(what)   (coord *)((*what).l_data)
#define when break;case
#define otherwise break;default
#define until(expr) while(!(expr))
#define ce(a, b) ((a).x == (b).x && (a).y == (b).y)
#define draw(window) wrefresh(window)
#define newfont(window) if (funfont) wattron(window, A_ALTCHARSET);
#define nofont(window) if (funfont) wattroff(window, A_ALTCHARSET);
#define hero player.t_pos
#define pstats player.t_stats
#define max_stats player.maxstats
#define pack player.t_pack
#define attach(a, b) _attach(&a, b)
#define detach(a, b) _detach(&a, b)
#define o_free_list(a) _o_free_list(&a)
#define r_free_list(a) _r_free_list(&a)
#define t_free_list(a) _t_free_list(&a)
#undef min
#undef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define on(thing, flag) \
    (((thing).t_flags[(flag >> FLAGSHIFT) & FLAGINDEX] & flag) != 0)
#define off(thing, flag) \
    (((thing).t_flags[(flag >> FLAGSHIFT) & FLAGINDEX] & flag) == 0)
#define turn_on(thing, flag) \
    ((thing).t_flags[(flag >> FLAGSHIFT) & FLAGINDEX] |= (flag & ~FLAGMASK))
#define turn_off(thing, flag) \
    ((thing).t_flags[(flag >> FLAGSHIFT) & FLAGINDEX] &= ~flag)

/* define the control character */

#undef CTRL
#define CTRL(ch) (ch & 037)

#define ALLOC(x) malloc((unsigned int) x)
#define FREE(x) free((char *) x)
#define EQSTR(a, b, c)  (strncmp(a, b, c) == 0)
#define EQUAL(a, b)     (strcmp(a, b) == 0)
#define GOLDCALC (rnd(50 + 10 * level) + 2)
#define ISRING(h, r) (cur_ring[h] != NULL && cur_ring[h]->o_which == r)
#define ISWEARING(r)    (ISRING(LEFT_1, r)  || ISRING(LEFT_2, r)  ||\
                         ISRING(LEFT_3, r)  || ISRING(LEFT_4, r)  ||\
                         ISRING(RIGHT_1, r) || ISRING(RIGHT_2, r) ||\
             ISRING(RIGHT_3, r) || ISRING(RIGHT_4, r))
#define newgrp() ++group
#define o_charges o_ac
#define o_kind o_ac
#define ISMULT(type) (type == FOOD)
#define isrock(ch) ((ch == WALL) || (ch == HORZWALL) || (ch == VERTWALL) || (ch == SECRETDOOR))
#define is_stealth(tp) \
    (rnd(25) < (tp)->t_stats.s_dext || (tp == &player && ISWEARING(R_STEALTH)))

#define has_light(rp) (((rp)->r_flags & HASFIRE) || ISWEARING(R_LIGHT))

#define mi_wght mi_worth
#define mi_food mi_curse

/*
 * Ways to die
 */

#define D_PETRIFY       -1
#define D_ARROW         -2
#define D_DART          -3
#define D_POISON        -4
#define D_BOLT          -5
#define D_SUFFOCATION   -6
#define D_POTION        -7
#define D_INFESTATION   -8
#define D_DROWN         -9
#define D_ROT           -10
#define D_CONSTITUTION  -11
#define D_STRENGTH      -12
#define D_SIGNAL        -13
#define D_CHOKE         -14
#define D_STRANGLE      -15
#define D_FALL          -16
#define D_RELIC         -17
#define D_STARVATION    -18
#define D_FOOD_CHOKE    -19
#define D_SCROLL        -20
#define D_FRIGHT        -21
#define D_CRYSTAL       -22
#define D_CARD          -23
#define DEATHNUM         23      /* number of ways to die */

/*
 * Things that appear on the screens
 */

#define WALL            ' '
#define PASSAGE         '#'
#define DOOR            '+'
#define FLOOR           '.'
#define HORZWALL        '-'
#define VERTWALL        '|'
#define VPLAYER         '@'
#define IPLAYER         '_'
#define POST            '^'
#define TRAPDOOR        '>'
#define ARROWTRAP       '{'
#define SLEEPTRAP       '$'
#define BEARTRAP        '}'
#define TELTRAP         '~'
#define DARTTRAP        '`'
#define WORMHOLE        '<'
#define POOL            '"'
#define MAZETRAP        '\''
#define SECRETDOOR      '&'
#define STAIRS          '%'
#define GOLD            '*'
#define POTION          '!'
#define SCROLL          '?'
#define MAGIC           '$'
#define BMAGIC          '>'     /*      Blessed magic   */
#define CMAGIC          '<'     /*      Cursed  magic   */
#define FOOD            ':'
#define WEAPON          ')'
#define MISSILE         '*'     /*      Magic Missile   */
#define ARMOR           ']'
#define MM              ';'
#define RELIC           ','
#define RING            '='
#define STICK           '/'
#define FOREST          '\\'

/*
 * Various constants
 * Crypt() returns a different string on the PC for some silly reason
 */

#define PASSWD          "mT5uKwhm5WDRs"
#define FIGHTBASE       10
#define BEFORE          1
#define AFTER           2
#define ESC             27
#define BOLT_LENGTH     12
#define MARKLEN         20
#define SLEEPTIME       (roll(15, 2))
#define BEARTIME        (roll(15, 2))
#define FREEZETIME      30
#define HEALTIME        40
#define SICKTIME        40
#define MORETIME        80
#define STOMACHSIZE     2100
#define PAINTIME        (roll(15, 2))
#define CLOAK_TIME      (roll(15, 2))
#define CHILLTIME       (roll(15, 2))
#define STONETIME       (roll(15, 2))
#define SMELLTIME       (50+rnd(30))
#define DUSTTIME        (50+rnd(30))
#define STINKTIME       (50+rnd(30))
#define HASTETIME       (50+rnd(30))
#define HUHDURATION     (50+rnd(30))
#define GONETIME        (50+rnd(30)) 
#define SKILLDURATION   (50+rnd(30))
#define SEEDURATION     (150+rnd(50))
#define CLRDURATION     (150+rnd(50))
#define FLYTIME         (150+rnd(50))
#define PHASEDURATION   (150+rnd(50))
#define ALCHEMYTIME     (250+rnd(100))
#define FIRETIME        (180+roll(20, 2))
#define COLDTIME        (180+roll(20, 2))
#define BOLTTIME        (180+roll(20, 2))
#define DAYLENGTH       700
#define LEVEL           700  /* make depth of dungeon equal to DAYLENGTH */
#define WANDERTIME      (max(5, (HARDER+rnd(25))-rnd(vlevel*2)))
#define SPELLTIME       ((max(30-pstats.s_lvl,5)))
#define vlevel          (max(level, turns/LEVEL + 1))

/*
 * Save against things
 */

#define VS_POISON               00
#define VS_PARALYZATION         00
#define VS_DEATH                00
#define VS_PETRIFICATION        01
#define VS_WAND                 02
#define VS_BREATH               03
#define VS_MAGIC                04

/*
 * attributes for treasures in dungeon
 */

#define ISCURSED               01
#define ISKNOW                 02
#define ISPOST                 04       /* object is in a trading post */
#define ISMETAL               010
#define ISPROT                020       /* object is protected */
#define ISBLESSED             040
#define ISPOISON             0100
#define ISMISL             020000
#define ISMANY             040000

/*
 * Various flag bits
 */

#define ISDARK                 01
#define ISGONE                 02
#define ISTREAS                04
#define ISFOUND               010
#define ISTHIEFSET            020
#define FORCEDARK             040

/*
 * 1st set of creature flags (this might include player)
 */

#define ISBLIND         0x00000001
#define ISINWALL        0x00000002
#define ISRUN           0x00000004
#define ISFLEE          0x00000008
#define ISINVIS         0x00000010
#define ISMEAN          0x00000020
#define ISGREED         0x00000040
#define CANSHOOT        0x00000080
#define ISHELD          0x00000100
#define ISHUH           0x00000200
#define ISREGEN         0x00000400
#define CANHUH          0x00000800
#define CANSEE          0x00001000
#define HASFIRE         0x00002000
#define ISSLOW          0x00004000
#define ISHASTE         0x00008000
#define ISCLEAR         0x00010000
#define CANINWALL       0x00020000
#define ISDISGUISE      0x00040000
#define CANBLINK        0x00080000
#define CANSNORE        0x00100000
#define HALFDAMAGE      0x00200000
#define CANSUCK         0x00400000
#define CANRUST         0x00800000
#define CANPOISON       0x01000000
#define CANDRAIN        0x02000000
#define ISUNIQUE        0x04000000
#define STEALGOLD       0x08000000

/* 
 * Second set of flags 
 */

#define STEALMAGIC      0x10000001
#define CANDISEASE      0x10000002
#define HASDISEASE      0x10000004
#define CANSUFFOCATE    0x10000008
#define DIDSUFFOCATE    0x10000010
#define BOLTDIVIDE      0x10000020
#define BLOWDIVIDE      0x10000040
#define NOCOLD          0x10000080
#define TOUCHFEAR       0x10000100
#define BMAGICHIT       0x10000200
#define NOFIRE          0x10000400
#define NOBOLT          0x10000800
#define CARRYGOLD       0x10001000
#define CANITCH         0x10002000
#define HASITCH         0x10004000
#define DIDDRAIN        0x10008000
#define WASTURNED       0x10010000
#define CANSELL         0x10020000
#define CANBLIND        0x10040000
#define NOACID          0x10080000
#define NOSLOW          0x10100000
#define NOFEAR          0x10200000
#define NOSLEEP         0x10400000
#define NOPARALYZE      0x10800000
#define NOGAS           0x11000000
#define CANMISSILE      0x12000000
#define CMAGICHIT       0x14000000
#define CANPAIN         0x18000000

/* 
 * Third set of flags 
 */

#define CANSLOW         0x20000001
#define CANTUNNEL       0x20000002
#define TAKEWISDOM      0x20000004
#define NOMETAL         0x20000008
#define MAGICHIT        0x20000010
#define CANINFEST       0x20000020
#define HASINFEST       0x20000040
#define NOMOVE          0x20000080
#define CANSHRIEK       0x20000100
#define CANDRAW         0x20000200
#define CANSMELL        0x20000400
#define CANPARALYZE     0x20000800
#define CANROT          0x20001000
#define ISSCAVENGE      0x20002000
#define DOROT           0x20004000
#define CANSTINK        0x20008000
#define HASSTINK        0x20010000
#define ISSHADOW        0x20020000
#define CANCHILL        0x20040000
#define CANHUG          0x20080000
#define CANSURPRISE     0x20100000
#define CANFRIGHTEN     0x20200000
#define CANSUMMON       0x20400000
#define TOUCHSTONE      0x20800000
#define LOOKSTONE       0x21000000
#define CANHOLD         0x22000000
#define DIDHOLD         0x24000000
#define DOUBLEDRAIN     0x28000000

/* 
 * Fourth set of flags 
 */

#define CANBRANDOM      0x30000001      /* Types of breath */
#define CANBACID        0x30000002      /* acid */
#define CANBFIRE        0x30000004      /* Fire */
#define CANBCGAS        0x30000008      /* confusion gas */
#define CANBBOLT        0x30000010      /* lightning bolt */
#define CANBGAS         0x30000020      /* chlorine gas */
#define CANBICE         0x30000040      /* ice */
#define CANBFGAS        0x30000080      /* Fear gas */
#define CANBPGAS        0x30000100      /* Paralyze gas */
#define CANBSGAS        0x30000200      /* Sleeping gas */
#define CANBSLGAS       0x30000400      /* Slow gas */
#define CANBREATHE      0x300007ff      /* Can it breathe at all? */

/*
 * Fifth set of flags
 */

#define ISUNDEAD        0x40000001
#define CANSONIC        0x40000002
#define TURNABLE        0x40000004
#define TAKEINTEL       0x40000008
#define NOSTAB          0x40000010
#define CANDISSOLVE     0x40000020
#define ISFLY           0x40000040      /* creature can fly */
#define CANTELEPORT     0x40000080      /* creature can teleport */
#define CANEXPLODE      0x40000100      /* creature explodes when hit */
#define CANDANCE        0x40000200      /* creature can make hero "dance" */
#define ISDANCE         0x40000400      /* creature (hero) is dancing */
#define CARRYFOOD       0x40000800
#define CARRYSCROLL     0x40001000
#define CARRYPOTION     0x40002000
#define CARRYRING       0x40004000
#define CARRYSTICK      0x40008000
#define CARRYMISC       0x40010000
#define CARRYMDAGGER    0x40020000      /* Dagger of Musty */
#define CARRYCLOAK      0x40040000      /* Cloak of Emori */
#define CARRYANKH       0x40080000      /* Ankh of Heil */
#define CARRYSTAFF      0x40100000      /* Staff of Ming */
#define CARRYWAND       0x40200000      /* Wand of Orcus */
#define CARRYROD        0x40400000      /* Rod of Asmodeus */
#define CARRYYAMULET    0x40800000      /* Amulet of Yendor */
#define CARRYMANDOLIN   0x41000000      /* Mandolin of Brian */
#define MISSEDDISP      0x42000000      /* Missed Cloak of Displacement */
#define CANBSTAB        0x44000000      /* Can backstab */
#define ISGUARDIAN      0x48000000      /* Guardian of a treasure room */

/*
 * Sixth set of flags
 */

#define CARRYHORN       0x50000001      /* Horn of Geryon */
#define CARRYMSTAR      0x50000002      /* Morning Star of Hruggek */
#define CARRYFLAIL      0x50000004      /* Flail of Yeenoghu */
#define CARRYWEAPON     0x50000008      /* A generic weapon */
#define CANAGE          0x50000010      /* can age you */
#define CARRYDAGGER     0x50000020      /* carry's a dumb old dagger */
#define AREMANY         0x50000040      /* they come in droves */
#define CARRYEYE        0x50000080      /* has the eye of Vecna */
#define HASSUMMONED     0x50000100      /* has already summoned */
#define ISSTONE         0x50000200      /* has been turned to stone */
#define NODETECT        0x50000400      /* detect monster will not show him */
#define NOSTONE         0x50000800      /* creature made its save vrs stone */
#define CARRYQUILL      0x50001000      /* has the quill of Nagrom */
#define CARRYAXE        0x50002000      /* has the axe of Aklad */
#define TOUCHSLOW       0x50004000      /* touch will slow hero */
#define WASDISRUPTED    0x50008000      /* creature was disrupted by player */
#define CARRYARMOR      0x50010000      /* creature will pick up armor */
#define CARRYBAMULET    0x50020000      /* amulet of skoraus stonebones */
#define CARRYSURTURRING 0x50040000      /* ring of Surtur */
#define CARRYCARD       0x50080000      /* carry the card of Alteran */
#define ISCHARMED       0x50100000      /* is the monster charmed? */
#define ISFRIENDLY      0x50100000      /* monster friendly for any reason? */

#define NEEDSTOACT      0x60000001      /* monster ready to act this turn n */
#define ISDEAD          0x60000002      /* monster is dead                  */
#define ISELSEWHERE     0x60000004      /* monster has been whisked away    */

/* Masks for choosing the right flag */

#define FLAGMASK     0xf0000000
#define FLAGINDEX    0x0000000f
#define FLAGSHIFT    28
#define MAXFLAGS     25                 /* max initial flags per creature */

/* 
 * Mask for cancelling special abilities 
 * The flags listed here will be the ones left on after the
 * cancellation takes place
 */

#define CANC0MASK (     ISBLIND         | ISINWALL      | ISRUN         | \
                        ISFLEE          | ISMEAN        | ISGREED       | \
                        CANSHOOT        | ISHELD        | ISHUH         | \
                        ISSLOW          | ISHASTE       | ISCLEAR       | \
                        ISUNIQUE )
#define CANC1MASK (     HASDISEASE      | DIDSUFFOCATE  | CARRYGOLD     | \
                        HASITCH         | CANSELL       | DIDDRAIN      | \
                        WASTURNED )
#define CANC2MASK (     HASINFEST       | NOMOVE        | ISSCAVENGE    | \
                        DOROT           | HASSTINK      | DIDHOLD )
#define CANC3MASK (     CANBREATHE )
#define CANC4MASK (     ISUNDEAD        | CANSONIC      | NOSTAB        | \
                        ISFLY           | CARRYFOOD     | CANEXPLODE    | \
                        ISDANCE         | CARRYSCROLL   | CARRYPOTION   | \
                        CARRYRING       | CARRYSTICK    | CARRYMISC     | \
                        CARRYMDAGGER    | CARRYCLOAK    | CARRYANKH     | \
                        CARRYSTAFF      | CARRYWAND     | CARRYROD      | \
                        CARRYYAMULET    | CARRYMANDOLIN | ISGUARDIAN )
#define CANC5MASK (     CARRYHORN       | CARRYMSTAR    | CARRYFLAIL    | \
                        CARRYEYE        | CARRYDAGGER   | HASSUMMONED   | \
                        AREMANY         | CARRYWEAPON   | NOSTONE       | \
                        CARRYQUILL      | CARRYAXE      | WASDISRUPTED  | \
                        CARRYARMOR      | CARRYBAMULET  | CARRYSURTURRING )
#define CANC6MASK (     CARRYCARD )
#define CANC7MASK ( 0 )
#define CANC8MASK ( 0 )
#define CANC9MASK ( 0 )
#define CANCAMASK ( 0 )
#define CANCBMASK ( 0 )
#define CANCCMASK ( 0 )
#define CANCDMASK ( 0 )
#define CANCEMASK ( 0 )
#define CANCFMASK ( 0 )

/* types of things */

#define TYP_POTION      0
#define TYP_SCROLL      1
#define TYP_FOOD        2
#define TYP_WEAPON      3
#define TYP_ARMOR       4
#define TYP_RING        5
#define TYP_STICK       6
#define TYP_MM          7
#define TYP_RELIC       8
#define NUMTHINGS       9

/*
 * food types
 */

#define E_RATION        0
#define E_APPLE         1
#define E_BANANA        2
#define E_BLUEBERRY     3
#define E_CANDLEBERRY   4
#define E_CAPRIFIG      5
#define E_DEWBERRY      6
#define E_ELDERBERRY    7
#define E_GOOSEBERRY    8
#define E_GUANABANA     9
#define E_HAGBERRY      10
#define E_JABOTICABA    11
#define E_PEACH         12
#define E_PITANGA       13
#define E_PRICKLEY      14
#define E_RAMBUTAN      15
#define E_SAPODILLA     16
#define E_SOURSOP       17
#define E_STRAWBERRY    18
#define E_SWEETSOP      19
#define E_WHORTLEBERRY  20
#define E_SLIMEMOLD     21
#define MAXFOODS        22

/*
 * Potion types
 */

#define P_CLEAR         0
#define P_ABIL          1
#define P_SEEINVIS      2
#define P_HEALING       3
#define P_MFIND         4
#define P_TFIND         5
#define P_RAISE         6
#define P_HASTE         7
#define P_RESTORE       8
#define P_PHASE         9
#define P_INVIS         10
#define P_FLY           11
#define P_FFIND         12
#define P_SKILL         13
#define P_FIRE          14
#define P_COLD          15
#define P_LIGHTNING     16
#define P_POISON        17
#define MAXPOTIONS      18

/*
 * Scroll types
 */

#define S_CONFUSE       0
#define S_MAP           1
#define S_LIGHT         2
#define S_HOLD          3
#define S_SLEEP         4
#define S_ALLENCH       5
#define S_IDENT         6
#define S_SCARE         7
#define S_GFIND         8
#define S_TELEP         9
#define S_CREATE        10
#define S_REMOVE        11
#define S_PETRIFY       12
#define S_GENOCIDE      13
#define S_CURING        14
#define S_MAKEIT        15
#define S_PROTECT       16
#define S_FINDTRAPS     17
#define S_RUNES         18
#define S_CHARM         19
#define MAXSCROLLS      20

/*
 * Weapon types
 */

#define MACE            0               /* mace */
#define SWORD           1               /* long sword */
#define BOW             2               /* short bow */
#define ARROW           3               /* arrow */
#define DAGGER          4               /* dagger */
#define ROCK            5               /* rocks */
#define TWOSWORD        6               /* two-handed sword */
#define SLING           7               /* sling */
#define DART            8               /* darts */
#define CROSSBOW        9               /* crossbow */
#define BOLT            10              /* crossbow bolt */
#define SPEAR           11              /* spear */
#define TRIDENT         12              /* trident */
#define SPETUM          13              /* spetum */
#define BARDICHE        14              /* bardiche */
#define PIKE            15              /* pike */
#define BASWORD         16              /* bastard sword */
#define HALBERD         17              /* halberd */
#define BATTLEAXE       18              /* battle axe */
#define MAXWEAPONS      19              /* types of weapons */
#define NONE            100             /* no weapon */

/*
 * Armor types
 */
 
#define LEATHER         0
#define RING_MAIL       1
#define STUDDED_LEATHER 2
#define SCALE_MAIL      3
#define PADDED_ARMOR    4
#define CHAIN_MAIL      5
#define SPLINT_MAIL     6
#define BANDED_MAIL     7
#define PLATE_MAIL      8
#define PLATE_ARMOR     9
#define MAXARMORS       10

/*
 * Ring types
 */
 
#define R_PROTECT       0
#define R_ADDSTR        1
#define R_SUSABILITY    2
#define R_SEARCH        3
#define R_SEEINVIS      4
#define R_ALERT         5
#define R_AGGR          6
#define R_ADDHIT        7
#define R_ADDDAM        8
#define R_REGEN         9
#define R_DIGEST        10
#define R_TELEPORT      11
#define R_STEALTH       12
#define R_ADDINTEL      13
#define R_ADDWISDOM     14
#define R_HEALTH        15
#define R_CARRY         16
#define R_LIGHT         17
#define R_DELUSION      18
#define R_FEAR          19
#define R_HEROISM       20
#define R_FIRE          21
#define R_WARMTH        22
#define R_VAMPREGEN     23
#define R_FREEDOM       24
#define R_TELCONTROL    25
#define MAXRINGS        26

/*
 * Rod/Wand/Staff types
 */
 
#define WS_LIGHT        0
#define WS_HIT          1
#define WS_ELECT        2
#define WS_FIRE         3
#define WS_COLD         4
#define WS_POLYMORPH    5
#define WS_MISSILE      6
#define WS_SLOW_M       7
#define WS_DRAIN        8
#define WS_CHARGE       9
#define WS_TELMON       10
#define WS_CANCEL       11
#define WS_CONFMON      12
#define WS_DISINTEGRATE 13
#define WS_PETRIFY      14
#define WS_PARALYZE     15
#define WS_MDEG         16
#define WS_CURING       17
#define WS_WONDER       18
#define WS_FEAR         19
#define MAXSTICKS       20

/*
 * miscellaneous magic items
 */
 
#define MM_JUG          0
#define MM_BEAKER       1
#define MM_BOOK         2
#define MM_ELF_BOOTS    3
#define MM_BRACERS      4
#define MM_OPEN         5
#define MM_HUNGER       6
#define MM_DISP         7
#define MM_PROTECT      8
#define MM_DRUMS        9
#define MM_DISAPPEAR    10
#define MM_CHOKE        11
#define MM_G_DEXTERITY  12
#define MM_G_OGRE       13
#define MM_JEWEL        14
#define MM_KEOGHTOM     15
#define MM_R_POWERLESS  16
#define MM_FUMBLE       17
#define MM_ADAPTION     18
#define MM_STRANGLE     19
#define MM_DANCE        20
#define MM_SKILLS       21
#define MM_CRYSTAL      22
#define MAXMM           23

/*
 * Relic types
 */
 
#define MUSTY_DAGGER            0
#define EMORI_CLOAK             1
#define HEIL_ANKH               2
#define MING_STAFF              3
#define ORCUS_WAND              4
#define ASMO_ROD                5
#define YENDOR_AMULET           6
#define BRIAN_MANDOLIN          7
#define GERYON_HORN             8
#define HRUGGEK_MSTAR           9
#define YEENOGHU_FLAIL          10
#define EYE_VECNA               11
#define AXE_AKLAD               12
#define QUILL_NAGROM            13
#define STONEBONES_AMULET       14
#define SURTUR_RING             15
#define ALTERAN_CARD            16
#define MAXRELIC                17

#define MAXDAEMONS      10
#define MAXFUSES        20

extern struct delayed_action d_list[MAXDAEMONS];
extern struct delayed_action f_list[MAXFUSES];
extern int demoncnt;        /* number of active daemons */
extern int fusecnt;

/* Now define the structures and types */

/*
 * character types
 */
 
struct character_types {
    char        name[40];       /* name of character class              */
    long        start_exp;      /* starting exp pts for 2nd level       */
    long        cap;            /* stop doubling here                   */
    int         hit_pts;        /* hit pts gained per level             */
    int         base;           /* Base to-hit value (AC 10)            */
    int         max_lvl;        /* Maximum level for changing value     */
    int         factor;         /* Amount base changes each time        */
    int         offset;         /* What to offset level                 */
    int         range;          /* Range of levels for each offset      */
};

/*
 * level types
 */
 
typedef enum {
        NORMLEV,        /* normal level */
        POSTLEV,        /* trading post level */
        MAZELEV,        /* maze level */
        OUTSIDE,        /* outside region */
        STARTLEV        /* beginning of the game */
} LEVTYPE;

/*
 * Help lists
 */
 
struct h_list {
    char h_ch;
    char h_desc[40];
};

struct item_list {
    unsigned char item_ch;
    char item_desc[40];
};

/*
 * Coordinate data type
 */
 
typedef struct {
    int x;
    int y;
} coord;

/*
 * structure for the ways to die
 */
 
struct death_type {
    int reason;
    char name[30];
};

/*
 * Linked list data type
 */
 
struct linked_list {
    struct linked_list *l_next;
    struct linked_list *l_prev;
    char *l_data;                       /* Various structure pointers */
};

/*
 * Stuff about magic items
 */
 
struct magic_item {
    char mi_name[30];
    int  mi_prob;
    int  mi_worth;
    int  mi_curse;
    int  mi_bless;
};

/*
 * Room structure
 */
 
struct room {
    coord r_pos;                        /* Upper left corner */
    coord r_max;                        /* Size of room */
    long r_flags;                       /* Info about the room */
    struct linked_list *r_fires;        /* List of fire creatures in room */
    struct linked_list *r_exit;         /* Linked list of exits */
};

/*
 * Array of all traps on this level
 */
 
struct trap {
    unsigned char tr_type;              /* What kind of trap */
    unsigned char tr_show;              /* Where disguised trap looks like */
    coord tr_pos;                       /* Where trap is */
    long tr_flags;                      /* Info about trap (i.e. ISFOUND) */
};

/*
 * Structure describing a fighting being
 */
 
struct stats {
    short s_str;                        /* Strength */
    short s_intel;                      /* Intelligence */
    short s_wisdom;                     /* Wisdom */
    short s_dext;                       /* Dexterity */
    short s_const;                      /* Constitution */
    short s_charisma;                   /* Charisma */
    unsigned long s_exp;                /* Experience */
    int s_lvladj;                       /* how much level is adjusted */
    int s_lvl;                          /* Level of mastery */
    int s_arm;                          /* Armor class */
    int s_hpt;                          /* Hit points */
    int s_pack;                         /* current weight of his pack */
    int s_carry;                        /* max weight he can carry */
    char s_dmg[30];                      /* String describing damage done */
};

/*
 * Structure describing a fighting being (monster at initialization)
 */
 
struct mstats {
    short ms_str;                        /* Strength */
    short ms_dex;                        /* dexterity */
    short ms_move;                       /* movement rate */
    unsigned long ms_exp;                /* Experience */
    short ms_lvl;                        /* Level of mastery */
    short ms_arm;                        /* Armor class */
    char ms_hpt[9];                        /* Hit points */
    char ms_dmg[30];                      /* String describing damage done */
};

/*
 * Structure for monsters and player
 */
 
struct thing {
    bool t_wasshot;                     /* Was character shot last round? */
    unsigned char t_type;                        /* What it is */
    unsigned char t_disguise;                    /* What mimic looks like */
    unsigned char t_oldch;                       /* Character that was where it was */
    short t_ctype;                      /* Character type */
    short t_index;                      /* Index into monster table */
    short t_no_move;                    /* How long the thing can't move */
    short t_quiet;                      /* used in healing */
    short t_movement;                   /* Base movement rate */
    short t_action;                     /* Action we're waiting to do */
    short t_artifact;                   /* base chance of using artifact */
    short t_wand;                       /* base chance of using wands */
    short t_summon;                     /* base chance of summoning */
    short t_cast;                       /* base chance of casting a spell */
    short t_breathe;                    /* base chance to swing at player */
    char  *t_name;                      /* name player gave his pet */
    coord t_doorgoal;                   /* What door are we heading to? */
    coord *t_dest;                      /* Where it is running to */
    coord t_pos;                        /* Position */
    coord t_oldpos;                     /* Last position */
    coord t_newpos;                     /* Where we want to go */
    unsigned long t_flags[16];          /* State word */
    struct linked_list *t_pack;         /* What the thing is carrying */
	struct linked_list *t_using;        /* What the thing is using */
	int t_selection;
    struct stats t_stats;               /* Physical description */
    struct stats maxstats;              /* maximum(or initial) stats */
    int    t_reserved;                  /* reserved for save/restore code */
};

/*
 * Array containing information on all the various types of monsters
 */
 
struct monster {
    char m_name[30];                    /* What to call the monster */
    short m_carry;                      /* Probability of carrying something */
    bool m_normal;                      /* Does monster exist? */
    bool m_wander;                      /* Does monster wander? */
    char m_appear;                      /* What does monster look like? */
    char m_intel[8];                    /* Intelligence range */
    long m_flags[MAXFLAGS];             /* Things about the monster */
    char m_typesum[30];                 /* type of creature can he summon */
    short m_numsum;                     /* how many creatures can he summon */
    short m_add_exp;                    /* Added experience per hit point */
    struct mstats m_stats;              /* Initial stats */
};

/*
 * Structure for a thing that the rogue can carry
 */
 
struct object {
    int o_type;                         /* What kind of object it is */
    coord o_pos;                        /* Where it lives on the screen */
    char o_launch;                      /* What you need to launch it */
    char o_damage[8];                   /* Damage if used like sword */
    char o_hurldmg[8];                  /* Damage if thrown */
    struct linked_list *contents;       /* contents of this object */
    int o_count;                        /* Count for plural objects */
    int o_which;                        /* Which object of a type it is */
    int o_hplus;                        /* Plusses to hit */
    int o_dplus;                        /* Plusses to damage */
    int o_ac;                           /* Armor class */
    long o_flags;                       /* Information about objects */
    int o_group;                        /* Group number for this object */
    int o_weight;                       /* weight of this object */
    unsigned char o_mark[MARKLEN];               /* Mark the specific object */
};

/*
 * weapon structure
 */
 
struct init_weps {
    char w_name[20];            /* name of weapon */
    char w_dam[8];              /* hit damage */
    char w_hrl[8];              /* hurl damage */
    char w_launch;              /* need to launch it */
    int  w_flags;               /* flags */
    int  w_rate;                /* rate of fire */
    int  w_wght;                /* weight of weapon */
    int  w_worth;               /* worth of this weapon */
};

/*
 * armor structure 
 */
 
struct init_armor {
        char a_name[30];        /* name of armor */
        int  a_prob;            /* chance of getting armor */
        int  a_class;           /* normal armor class */
        int  a_worth;           /* worth of armor */
        int  a_wght;            /* weight of armor */
};

struct spells {
    short s_which;              /* which scroll or potion */
    short s_cost;               /* cost of casting spell */
    short s_type;               /* scroll or potion */
    int   s_flag;               /* is the spell blessed/cursed? */
};

struct words
{
    char w_string[30];
};

#define NAMELEN 80
#define SYSLEN 10
#define LOGLEN 9

struct sc_ent {
    unsigned long       sc_score;
    char        sc_name[NAMELEN];
    char        sc_system[SYSLEN];
    char        sc_login[LOGLEN];
    short       sc_flags;
    short       sc_level;
    short       sc_ctype;
    short       sc_monster;
    short       sc_quest;
};

/*
 * Other structures
 */
 
struct linked_list  *find_mons(), *find_obj(), *get_item(), *new_item(),
                    *new_thing(), *wake_monster(), *get_hurl(), *spec_item(),
                    *creat_item(), *wield_weap();

struct room         *roomin();
struct trap         *trap_at();

char *getenv(), *tr_name(), *new(), *vowelstr(),
    *inv_name(), *num(),
    *ring_num(), *misc_num(), *blesscurse(), *p_kind(), *typ_name(),
    *prname(), *monster_name(), *weap_name();

coord   rndmove(), *fallpos(), *doorway(), get_coordinates();
int can_shoot(),misc_name();

short   randmonster(), id_monst(), movement();

int bugkill(), nohaste(), spell_recovery(), doctor(), runners(), swander(),
    unconfuse(), unsee(), fumble(), unclrhead(), unphase(), noslow(),
    rollwand(), stomach(), sight(), unstink(), suffocate(), cure_disease(),
    shoot_bolt(), changeclass(), appear(), dust_appear(), unchoke(),
    alchemy(), trap_look(), strangle(), ring_teleport(), ring_search(),
    grab(), dsrpt_player(), quill_charge(), make_sell_pack(), unskill(),
    findmindex(), nobolt(), nofire(), nocold(), usage_time(), eat_gold(),
    chant_recovery(), prayer_recovery(), dsrpt_monster(), opt_player();

bool    blue_light(), can_blink(), creat_mons(), add_pack(), invisible(),
    straight_shot(), maze_view(), lit_room(), getdelta(), save_file(),
    save_game(), m_use_it(), m_use_pack(), get_dir(), need_dir(),passwd();

long    check_level();
long    encread();
long    get_worth();
long    encwrite();

void    byebye(int sig), genmonsters(), quit(int sig),
    auto_save(int sig), endit(int sig), tstp();

void    teleport();
    
int undance(), land(), cloak_charge(), wghtchk();

int add_intelligence(), add_strength(), add_wisdom(), add_dexterity(),
    add_constitution(), add_charisma(), res_intelligence(), res_strength(),
    res_wisdom(), res_dexterity(), res_constitution(), res_charisma();

/*
 * Now all the global variables
 */
 
extern struct trap traps[];
extern struct character_types char_class[];  /* character classes */
extern struct room rooms[];             /* One for each room -- A level */
extern struct room *oldrp;              /* Roomin(&oldpos) */
extern struct linked_list *mlist;       /* List of monsters on the level */
extern struct linked_list *tlist;       /* list of monsters fallen down traps */
extern struct linked_list *rlist;       /* list of monsters that have died    */
extern struct death_type deaths[];      /* all the ways to die */
extern struct thing player;             /* The rogue */
extern struct monster monsters[NUMMONST+1];       /* The initial monster states */
extern struct linked_list *lvl_obj;     /* List of objects on this level */
extern struct linked_list *monst_dead;  /* Indicates monster that got killed */
extern struct object *cur_weapon;       /* Which weapon he is weilding */
extern struct object *cur_armor;        /* What a well dresssed rogue wears */
extern struct object *cur_ring[];       /* Which rings are being worn */
extern struct object *cur_misc[];       /* which MM's are in use */
extern struct magic_item things[];      /* Chances for each type of item */
extern struct magic_item s_magic[];     /* Names and chances for scrolls */
extern struct magic_item p_magic[];     /* Names and chances for potions */
extern struct magic_item r_magic[];     /* Names and chances for rings */
extern struct magic_item ws_magic[];    /* Names and chances for sticks */
extern struct magic_item m_magic[];     /* Names and chances for MM */
extern struct magic_item rel_magic[];   /* Names and chances for relics */
extern struct magic_item foods[];       /* Names and chances for foods */
extern struct spells magic_spells[];    /* spells for magicians */
extern struct spells cleric_spells[];   /* spells for clerics */
extern struct spells druid_spells[];    /* spells for druids */
extern struct spells quill_scrolls[];   /* scrolls for quill */
extern const char *cnames[][NUM_CNAMES];      /* Character level names */
extern struct words abilities[NUMABILITIES];   /* Names of the various abilities */
extern char curpurch[];                 /* name of item ready to buy */
extern char PLAYER;                     /* what the player looks like */
extern int nfloors;                     /* Number of floors in this dungeon */
extern int cols;                        /* number of columns on terminal */
extern int lines;                       /* number of lines in terminal */
extern int char_type;                   /* what type of character is player */
extern int foodlev;                     /* how fast he eats food */
extern int level;                       /* What level rogue is on */
extern int trader;                      /* number of purchases */
extern int curprice;                    /* price of an item */
extern long purse;                      /* How much gold the rogue has */
extern int mpos;                        /* Where cursor is on top line */
extern int ntraps;                      /* Number of traps on this level */
extern int inpack;                      /* Number of things in pack */
extern int total;                       /* Total dynamic memory bytes */
extern int lastscore;                   /* Score before this turn */
extern int no_food;                     /* Number of levels without food */
extern int foods_this_level;            /* num of foods this level */
extern int seed;                        /* Random number seed */
extern int count;                       /* Number of times to repeat command */
extern int max_level;                   /* Deepest player has gone */
extern int cur_max;                     /* Deepest player has gone currently */
extern int prev_max;                    /* A flag for worm hole */
extern int move_free;                   /* Free movement check */
extern int food_left;                   /* Amount of food in hero's stomach */
extern int group;                       /* Current group number */
extern int hungry_state;                /* How hungry is he */
extern int infest_dam;                  /* Damage from parasites */
extern int lost_str;                    /* Amount of strength lost */
extern int hold_count;                  /* Number of monsters holding player */
extern int trap_tries;                  /* Number of attempts to set traps */
extern int chant_time;                  /* Number of chant points/exp level */
extern int pray_time;                   /* Number of prayer points/exp level */
extern int spell_power;                 /* Spell power left at this level */
extern long turns;                      /* Number of turns player has taken */
extern int quest_item;                  /* Item hero is looking for */
extern int cur_relic[];                 /* Current relics */
extern char take;                       /* Thing the rogue is taking */
extern char prbuf[];                    /* Buffer for sprintfs */
extern char outbuf[];                   /* Output buffer for stdout */
extern char runch;                      /* Direction player is running */
extern char *s_names[];                 /* Names of the scrolls */
extern char *p_colors[];                /* Colors of the potions */
extern char *r_stones[];                /* Stone settings of the rings */
extern struct init_weps weaps[];        /* weapons and attributes */
extern struct init_armor armors[];      /* armors and attributes */
extern char *ws_made[];                 /* What sticks are made of */
extern char *release;                   /* Release number of rogue */
extern char whoami[];                   /* Name of player */
extern char fruit[];                    /* Favorite fruit */
extern char huh[];                      /* The last message printed */
extern char *s_guess[];                 /* Players guess at what scroll is */
extern char *p_guess[];                 /* Players guess at what potion is */
extern char *r_guess[];                 /* Players guess at what ring is */
extern char *ws_guess[];                /* Players guess at what wand is */
extern char *m_guess[];                 /* Players guess at what MM is */
extern char *ws_type[];                 /* Is it a wand or a staff */
extern char file_name[];                /* Save file name */
extern char score_file[];               /* Score file name */
extern char home[];                     /* User's home directory */
extern WINDOW *cw;                      /* Window that the player sees */
extern WINDOW *hw;                      /* Used for the help command */
extern WINDOW *mw;                      /* Used to store mosnters */
extern WINDOW *msgw;                    /* Message window */
extern bool pool_teleport;              /* just teleported from a pool */
extern bool inwhgt;                     /* true if from wghtchk() */
extern bool running;                    /* True if player is running */
extern bool playing;                    /* True until he quits */
extern bool wizard;                     /* True if allows wizard commands */
extern bool after;                      /* True if we want after daemons */
extern bool notify;                     /* True if player wants to know */
extern bool fight_flush;                /* True if toilet input */
extern bool terse;                      /* True if we should be short */
extern bool auto_pickup;                /* Pick up things automatically? */
extern bool menu_overlay;               /* Use overlay type menu */
extern bool door_stop;                  /* Stop running when we pass a door */
extern bool jump;                       /* Show running as series of jumps */
extern bool slow_invent;                /* Inventory one line at a time */
extern bool def_attr;                   /* True for default attributes */
extern bool firstmove;                  /* First move after setting door_stop */
extern bool waswizard;                  /* Was a wizard sometime */
extern bool askme;                      /* Ask about unidentified things */
extern bool s_know[];                   /* Does he know what a scroll does */
extern bool p_know[];                   /* Does he know what a potion does */
extern bool r_know[];                   /* Does he know what a ring does */
extern bool ws_know[];                  /* Does he know what a stick does */
extern bool m_know[];                   /* Does he know what a MM does */
extern bool in_shell;                   /* True if executing a shell */
extern bool daytime;                    /* Indicates whether it is daytime */
extern bool funfont;                    /* Is fun font available? */
extern coord oldpos;                    /* Position before last look() call */
extern coord grid[];                    /* used for random pos generation */
extern char *nothing;                   /* "Nothing seems to happen." */
extern char *spacemsg;
extern char *morestr;
extern char *retstr;
extern LEVTYPE levtype;
extern int (*add_abil[NUMABILITIES])(); /* Functions to change abilities */
extern int (*res_abil[NUMABILITIES])(); /* Functions to change abilities */
extern int mf_count;       /* move_free counter - see actions.c(m_act()) */
extern int mf_jmpcnt;      /* move_free counter for # of jumps        */
extern int killed_chance;  /* cumulative chance for goodies to loose it, fight.c */
extern coord move_nh;        /* move.c */
#define NCOLORS 32
#define NSYLLS  127
#define NSTONES 47
#define NWOOD 24
#define NMETAL 16
extern struct words rainbow[NCOLORS];
extern struct words sylls[NSYLLS];
extern struct words stones[NSTONES];
extern struct words wood[NWOOD];
extern struct words metal[NMETAL];

