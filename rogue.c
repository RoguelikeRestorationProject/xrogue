/*
    rogue.c  -  Global game variables
    
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

#include <ctype.h>
#include <curses.h>
#include "rogue.h"

/*
 * Now all the global variables
 */

struct trap traps[MAXTRAPS];
struct room rooms[MAXROOMS];            /* One for each room -- A level */
struct room *oldrp;                     /* Roomin(&player.t_oldpos) */
struct thing player;                    /* The rogue */
struct object *cur_armor;               /* What a well dresssed rogue wears */
struct object *cur_ring[NUM_FINGERS];   /* Which rings are being worn */
struct object  *cur_misc[NUM_MM];       /* which MM's are in use */
int cur_relic[MAXRELIC];                /* Currently used relics */
struct linked_list *lvl_obj = NULL; 
struct linked_list *mlist = NULL;
struct linked_list *rlist = NULL;       /* list of dead monsters to be reaped */
struct linked_list *tlist = NULL;       /* list of monsters fallen down traps */
struct linked_list *monst_dead = NULL;  /* monster killed by monster    */
struct object *cur_weapon = NULL;
int char_type = -1;                     /* what type of character is player */
int foodlev = 1;                        /* how fast he eats food */
int ntraps;                             /* Number of traps on this level */
int trader = 0;                         /* no. of purchases */
int curprice = -1;                      /* current price of item */
int seed;                               /* Random number seed */
int max_level;                          /* Deepest player has gone ever */
int cur_max;                            /* Deepest player has gone currently */
int prev_max;                           /* A flag indicating worm hole */
int move_free = 0;                      /* Movement check (io.c & actions.c) */
int mpos = 0;
int level = 0;
long purse = 0;
int inpack = 0;
int total = 0;
int no_food = 0;                        /* how long has he gone with no food */
int foods_this_level = 0;               /* foods made per level */
int count = 0;
int food_left = STOMACHSIZE-MORETIME-1;
int group = 1;
int hungry_state = F_OKAY;
int infest_dam=0;
int lost_str=0;
int lastscore = -1;
int hold_count = 0;
int trap_tries = 0;
int chant_time = 0;
int pray_time = 0;
int spell_power = 0;
long turns = 0;                         /* Number of turns player has taken */
int quest_item = 0;                     /* Item player is looking for */
int cols = 0;                           /* number of columns in terminal */
int lines = 0;                          /* number of lines on the terminal */
int nfloors = -1;                       /* Number of floors in this dungeon */
char curpurch[LINELEN];                 /* name of item ready to buy */
char PLAYER = VPLAYER;                  /* what the player looks like */
char take;                              /* Thing the rogue is taking */
char prbuf[LINELEN*2];                  /* Buffer for sprintfs */
char runch;                             /* Direction player is running */
char *s_names[MAXSCROLLS];              /* Names of the scrolls */
char *p_colors[MAXPOTIONS];             /* Colors of the potions */
char *r_stones[MAXRINGS];               /* Stone settings of the rings */
char *ws_made[MAXSTICKS];               /* What sticks are made of */
char whoami[LINELEN];                   /* Name of player */
char huh[LINELEN];                       /* The last message printed */
char *s_guess[MAXSCROLLS];              /* Players guess at what scroll is */
char *p_guess[MAXPOTIONS];              /* Players guess at what potion is */
char *r_guess[MAXRINGS];                /* Players guess at what ring is */
char *ws_guess[MAXSTICKS];              /* Players guess at what wand is */
char *m_guess[MAXMM];                   /* Players guess at what MM is */
char *ws_type[MAXSTICKS];               /* Is it a wand or a staff */
char file_name[LINELEN];                /* Save file name */
char score_file[LINELEN];               /* Score file name */
char home[LINELEN];                     /* User's home directory */
WINDOW *cw;                             /* Window that the player sees */
WINDOW *hw;                             /* Used for the help command */
WINDOW *mw;                             /* Used to store monsters */
WINDOW *msgw;                           /* Used to display messages */
bool pool_teleport = FALSE;             /* just teleported from a pool */
bool inwhgt = FALSE;                    /* true if from wghtchk() */
bool after;                             /* True if we want after daemons */
bool waswizard;                         /* Was a wizard sometime */
bool s_know[MAXSCROLLS];                /* Does he know what a scroll does */
bool p_know[MAXPOTIONS];                /* Does he know what a potion does */
bool r_know[MAXRINGS];                  /* Does he know what a ring does */
bool ws_know[MAXSTICKS];                /* Does he know what a stick does */
bool m_know[MAXMM];                     /* Does he know what a MM does */

/* options */
bool playing = TRUE;        /* Defaults */
bool running = FALSE; 
bool wizard = FALSE;
bool notify = TRUE; 
bool fight_flush = FALSE;
bool terse = FALSE; 
bool auto_pickup = FALSE;
bool def_attr = FALSE;      /* default attributes */
bool menu_overlay = TRUE;
bool door_stop = TRUE;
bool jump = TRUE; 
bool slow_invent = FALSE; 
bool firstmove = FALSE; 
bool askme = TRUE;
bool in_shell = FALSE; 
bool daytime = TRUE;
bool funfont = FALSE;

LEVTYPE levtype;           /* what type of level am i'm on? */

char *nothing  =  "Nothing seems to happen. ";
char *spacemsg =  "--Press space to continue--";
char *morestr  =  " --More--";
char *retstr   =  "[Press return to continue]";

/* 
 * This lays out all the class specific details
 *
 * Here are the beginning experience levels for all players.
 * All further experience levels are computed by muliplying by 2
 * up through MAXDOUBLE. Then exp pts are calculated by adding
 * in the cap figure. You must change MAXDOUBLE if you change the 
 * cap figure. 
 */

struct character_types char_class[NUM_CHARTYPES] = {
/* name         exppts  cap     hitpts  Base   Maxlvl, Factor, Offset, Range */
{ "fighter",    90,    1310720,  13,    10,     30,     2,      1,      3 },
{ "ranger",     110,   2293760,  10,    10,     22,     2,      1,      2 },
{ "paladin",    110,   1966080,  10,    10,     23,     2,      1,      2 },
{ "magician",   105,   2129920,   9,    10,     24,     2,      1,      2 },
{ "cleric",     105,   1802240,   9,    10,     24,     2,      1,      2 },
{ "thief",      95,    1228800,  11,    10,     28,     2,      1,      3 },
{ "assassin",   95,    1392640,  11,    10,     26,     2,      1,      3 },
{ "druid",      105,   1638400,   9,    10,     24,     2,      1,      2 },
{ "monk",       100,   1556480,  10,    10,     25,     2,      1,      2 },
{ "monster",    0,     0,         8,    10,     20,     1,      0,      2 },
};

/*
 * This array lists the names of the character's abilities.  It must be ordered
 * according to the ability definitions in rogue.h.
 */

struct words abilities[NUMABILITIES] = {
  "Intelligence", "Strength", "Wisdom", "Dexterity", "Constitution", "Charisma"
};

/*
 * NOTE: the ordering of the points in this array is critical. They MUST
 *       be listed in the following sequence:
 *
 *              7   4   6
 *              1   0   2
 *              5   3   8
 */

coord grid[9] = {{0,0},
                 { 0,-1}, { 0, 1}, {-1, 0}, { 1, 0},
                 {-1,-1}, { 1, 1}, { 1,-1}, {-1, 1}
                };

struct death_type deaths[DEATHNUM] = {
    { D_ARROW,          "an arrow"},
    { D_DART,           "a dart"},
    { D_BOLT,           "a bolt"},
    { D_POISON,         "poison"},
    { D_POTION,         "a cursed potion"},
    { D_PETRIFY,        "petrification"},
    { D_SUFFOCATION,    "suffocation"},
    { D_INFESTATION,    "a parasite"},
    { D_DROWN,          "drowning"},
    { D_ROT,            "body rot"},
    { D_CONSTITUTION,   "poor health"},
    { D_STRENGTH,       "being too weak"},
    { D_SIGNAL,         "a bug"},
    { D_CHOKE,          "dust of choking"},
    { D_STRANGLE,       "strangulation"},
    { D_FALL,           "a fall"},
    { D_RELIC,          "an artifact's wrath"},
    { D_STARVATION,     "starvation"},
    { D_FOOD_CHOKE,     "choking on food"},
    { D_SCROLL,         "reading a scroll"},
    { D_FRIGHT,         "being too frightened"},
    { D_CRYSTAL,        "being absorbed"},
    { D_CARD,           "the face of death"},
};

/*
 * weapons and their attributes
 */

struct init_weps weaps[MAXWEAPONS] = {
    { "mace",           "2d10","2d10", NONE,     ISMETAL, 6, 150, 15 },
    { "long sword",     "3d4",  "2d8", NONE,     ISMETAL, 5, 200, 25 },
    { "short bow",      "1d1",  "1d1", NONE,     0, 8, 50, 4 },
    { "arrow",          "2d4",  "1d6", BOW,      ISMANY|ISMISL, 1, 5, 4 },
    { "dagger",         "2d8",  "1d6", NONE,     ISMETAL|ISMISL|ISMANY, 2,10,7},
    { "rock",           "2d4",  "1d6", SLING,    ISMANY|ISMISL, 1, 20, 3 },
    { "two-handed sword","3d10","3d8", NONE,     ISMETAL, 4, 250, 40 },
    { "sling",          "1d1",  "1d1", NONE,     0, 8, 25, 3 },
    { "dart",           "2d4",  "2d6", NONE,     ISMANY|ISMISL, 2, 15, 7 },
    { "crossbow",       "1d1",  "1d1", NONE,     0, 8, 75, 5 },
    { "crossbow bolt",  "2d4",  "2d4", CROSSBOW, ISMANY|ISMISL, 1, 10, 5 },
    { "spear",          "2d6", "3d10", NONE,     ISMISL,  7, 100, 15 },
    { "trident",        "3d6",  "3d4", NONE,     ISMETAL, 4, 200, 30 },
    { "spetum",         "2d6",  "2d8", NONE,     ISMETAL, 6, 150, 20 },
    { "bardiche",       "3d4", "2d10", NONE,     ISMETAL, 5, 150, 25 },
    { "pike",           "2d8",  "2d8", NONE,     ISMETAL, 7, 100, 15 },
    { "bastard sword",  "3d8",  "3d6", NONE,     ISMETAL, 4, 175, 30 },
    { "halberd",        "2d8",  "2d4", NONE,     ISMETAL, 6, 100, 10 },
    { "battle axe",     "2d8",  "3d8", NONE,     ISMETAL, 5, 150, 15 },
} ;
 
struct init_armor armors[MAXARMORS] = {
        { "leather armor",          10, 8, 200, 100 },
        { "ring mail",              20, 7, 250, 200 },
        { "studded leather armor",  30, 5, 320, 250 },
        { "scale mail",             40, 7, 280, 250 },
        { "padded armor",           50, 6, 350, 300 },
        { "chain mail",             60, 6, 350, 600 },
        { "splint mail",            70, 5, 370, 400 },
        { "banded mail",            80, 5, 370, 350 },
        { "plate mail",             90, 4, 400, 400 },
        { "plate armor",           100, 3, 500, 450 },
};

struct magic_item things[NUMTHINGS] = {
    { "potion",                 220,   10 },    /* potion               */
    { "scroll",                 220,   30 },    /* scroll               */
    { "food",                   190,   20 },    /* food                 */
    { "weapon",                  90,    0 },    /* weapon               */
    { "armor",                   90,    0 },    /* armor                */
    { "ring",                    70,    5 },    /* ring                 */
    { "stick",                   70,    0 },    /* stick                */
    { "miscellaneous magic",     50,   50 },    /* miscellaneous magic  */
    { "artifact",                 0,   10 },    /* artifact             */
};

struct magic_item s_magic[MAXSCROLLS] = {
    { "monster confusion",       40, 125,  0,  0 },
    { "magic mapping",           60, 150,  0,  5 },
    { "light",                   60, 100, 15, 15 },
    { "hold monster",            30, 200, 20, 20 },
    { "sleep",                   20, 150, 25,  0 },
    { "enchantment",            130, 200, 15, 15 },
    { "identify",               170, 100,  0, 20 },
    { "scare monster",           40, 250, 20, 30 },
    { "gold detection",          30, 110,  0,  0 },
    { "teleportation",           60, 165, 20, 20 },
    { "create monster",          20,  75,  0,  0 },
    { "remove curse",            80, 120, 15, 15 },
    { "petrification",           30, 185,  0,  0 },
    { "genocide",                10, 300,  0,  0 },
    { "cure disease",            80, 160,  0,  0 },
    { "acquirement",             10, 700,  0,  5 },
    { "protection",              30, 190, 10,  0 },
    { "trap finding",            50, 180,  0,  0 },
    { "runes",                   20,  50,  0,  0 },
    { "charm monster",           30, 275,  0, 20 },
};

struct magic_item p_magic[MAXPOTIONS] = {
    { "clear thought",           50, 180, 10,  5 },
    { "gain ability",           160, 210, 10, 10 },
    { "see invisible",           40, 150, 20, 20 },
    { "healing",                140, 130, 15, 15 },
    { "monster detection",       40, 120,  0,  0 },
    { "magic detection",         70, 105,  0,  0 },
    { "raise level",             10, 450, 10,  5 },
    { "haste self",              50, 180, 20,  5 },
    { "restore abilities",      130, 140,  0, 15 },
    { "phasing",                 60, 210, 10, 10 },
    { "invisibility",            20, 230,  0, 10 },
    { "flying",                  50, 130,  0, 15 },
    { "food detection",          20, 150,  0,  0 },
    { "skill",                   10, 200, 20,  5 },
    { "fire resistance",         40, 250, 10,  5 },
    { "cold resistance",         40, 250, 10,  5 },
    { "lightning protection",    40, 250, 20,  5 },
    { "poison",                  30, 205, 25,  0 },
};

struct magic_item r_magic[MAXRINGS] = {
    { "protection",              60, 200,  25, 25 },
    { "add strength",            50, 200,  25, 25 },
    { "sustain ability",         50, 500,   0,  0 },
    { "searching",               40, 400,   0,  0 },
    { "extra sight",             60, 350,   0,  0 },
    { "alertness",               40, 380,   0,  0 },
    { "aggravate monster",       30, 100, 100,  0 },
    { "dexterity",               50, 220,  25, 25 },
    { "increase damage",         60, 220,  25, 25 },
    { "regeneration",            40, 600,   0,  0 },
    { "slow digestion",          50, 240,  20, 20 },
    { "teleportation",           20, 100,  90,  0 },
    { "stealth",                 20, 300,   0,  0 },
    { "add intelligence",        50, 240,  25, 25 },
    { "increase wisdom",         40, 220,  25, 25 },
    { "sustain health",          80, 500,   0,  0 },
    { "carrying",                10, 100,  90,  0 },
    { "illumination",            30, 520,   0,  0 },
    { "delusion",                10, 100, 100,  0 },
    { "fear",                    20, 100,  75,  0 },
    { "heroism",                 50, 390,   0,  0 },
    { "fire resistance",         40, 400,   0,  0 },
    { "warmth",                  40, 400,   0,  0 },
    { "vampiric regeneration",   10,1000,   0,  0 },
    { "free action",             40, 370,   0,  0 },
    { "teleport control",        10, 700,   0,  0 },
};

struct magic_item ws_magic[MAXSTICKS] = {
    { "light",                   80, 120, 15, 15 },
    { "striking",                50, 115,  0,  0 },
    { "lightning",               40, 200,  0,  0 },
    { "fire",                    30, 200,  0,  0 },
    { "cold",                    30, 200,  0,  0 },
    { "polymorph",               80, 150,  0,  0 },
    { "magic missile",           90, 170,  0,  0 },
    { "slow",                    70, 220, 20, 10 },
    { "drain life",              50, 210, 20,  0 },
    { "charging",                70, 400,  0,  0 },
    { "teleport",                90, 140, 20, 10 },
    { "cancellation",            50, 130,  0,  0 },
    { "confusion",               30, 100, 20,  0 },
    { "disintegration",          20, 300, 25,  0 },
    { "petrification",           30, 240,  0,  0 },
    { "paralyzation",            30, 180, 10,  0 },
    { "degeneration",            30, 250, 20,  0 },
    { "curing",                  50, 250, 20,  5 },
    { "wonder",                  40, 110, 20, 20 },
    { "fear",                    40, 180,  0,  0 },
};

/*
 * WARNING: unique miscellaneous magic items must be put at the end
 *          of this list. They MUST be the last items. The function
 *          create_obj() in wizard.c depends on it.
 */

struct magic_item m_magic[MAXMM] = {
    { "alchemy jug",               40,  240,   0,  0 },
    { "beaker of potions",         60,  300,   0,  0 },
    { "book of spells",            60,  300,   0,  0 },
    { "boots of elvenkind",        50,  500,   0,  0 },
    { "bracers of defense",        80,  400,  20, 10 },
    { "chime of opening",          30,  250,   0,  0 },
    { "chime of hunger",           20,  100, 100,  0 },
    { "cloak of displacement",     60,  500,   0,  0 },
    { "cloak of protection",       80,  400,  20, 10 },
    { "drums of panic",            60,  350,   0,  0 },
    { "dust of disappearance",     30,  300,   0,  0 },
    { "dust of choking",           30,  100, 100,  0 },
    { "gauntlets of dexterity",    40,  600,  25,  0 },
    { "gauntlets of ogre power",   40,  600,  25,  0 },
    { "jewel of attacks",          50,  150, 100,  0 },
    { "keoghtoms ointment",        60,  350,   0,  0 },
    { "robe of powerlessness",     20,  100, 100,  0 },
    { "gauntlets of fumbling",     30,  100, 100,  0 },
    { "necklace of adaptation",    50,  500,   0,  0 },
    { "necklace of strangulation", 30,  110, 100,  0 },
    { "boots of dancing",          40,  120, 100,  0 },
    { "book of skills",            30,  650,   0,  0 },
    { "medicine crystal",          10,  800,  25,  5 },
};

struct magic_item rel_magic[MAXRELIC] = {
    { "Daggers of Musty Doit",     0, 50000,  0, 0},
    { "Cloak of Emori",            0, 50000,  0, 0},
    { "Ankh of Heil",              0, 50000,  0, 0},
    { "Staff of Ming",             0, 50000,  0, 0},
    { "Wand of Orcus",             0, 50000,  0, 0},
    { "Rod of Asmodeus",           0, 50000,  0, 0},
    { "Amulet of Yendor",          0, 50000,  0, 0},
    { "Mandolin of Brian",         0, 50000,  0, 0},
    { "Horn of Geryon",            0, 50000,  0, 0},
    { "Morning Star of Hruggek",   0, 50000,  0, 0},
    { "Flail of Yeenoghu",         0, 50000,  0, 0},
    { "Eye of Vecna",              0, 50000,  0, 0},
    { "Axe of Aklad",              0, 50000,  0, 0},
    { "Quill of Nagrom",           0, 50000,  0, 0},
    { "Amulet of Stonebones",      0, 50000,  0, 0},
    { "Ring of Surtur",            0, 50000,  0, 0},
    { "Card of Alteran",           0, 50000,  0, 0},
};

/*
 * food and fruits that you get
 */
struct magic_item foods[MAXFOODS] = {

    { "food ration",    690, 50, 750,  0},
    { "apple",           10, 20, 300,  0},
    { "banana",          30, 20, 300,  0},
    { "blueberry",       30, 20, 300,  0},
    { "candleberry",     10, 20, 300,  0},
    { "caprifig",        20, 20, 300,  0},
    { "dewberry",        10, 20, 300,  0},
    { "elderberry",      30, 20, 300,  0},
    { "gooseberry",      20, 20, 300,  0},
    { "guanabana",       30, 20, 300,  0},
    { "hagberry",        10, 20, 300,  0},
    { "jaboticaba",      10, 20, 300,  0},
    { "peach",           10, 20, 300,  0},
    { "pitanga",         10, 20, 300,  0},
    { "prickly pear",    10, 20, 300,  0},
    { "rambutan",        10, 20, 300,  0},
    { "sapodilla",       10, 20, 300,  0},
    { "soursop",         10, 20, 300,  0},
    { "strawberry",      10, 20, 300,  0},
    { "sweetsop",        10, 20, 300,  0},
    { "whortleberry",    10, 20, 300,  0},
    { "slime-mold",      10, 10, 100,  0},
};

/*
 * these are the spells that a magician can cast
 */

struct spells magic_spells[MAXSPELLS] = {
        { P_TFIND,         3,     TYP_POTION,   0         },
        { S_IDENT,         5,     TYP_SCROLL,   0         },
        { S_LIGHT,         7,     TYP_SCROLL,   ISBLESSED },
        { S_REMOVE,       10,     TYP_SCROLL,   0         },
        { S_FINDTRAPS,    15,     TYP_SCROLL,   0         },
        { P_FLY,          20,     TYP_POTION,   0         },
        { S_TELEP,        25,     TYP_SCROLL,   0         },
        { S_SLEEP,        30,     TYP_SCROLL,   0         },
        { P_SEEINVIS,     35,     TYP_POTION,   ISBLESSED },
        { P_CLEAR,        40,     TYP_POTION,   0         },
        { WS_COLD,        45,     TYP_STICK,    0         },
        { P_PHASE,        50,     TYP_POTION,   0         },
        { WS_FIRE,        55,     TYP_STICK,    0         },
        { P_HASTE,        60,     TYP_POTION,   ISBLESSED },
        { WS_ELECT,       65,     TYP_STICK,    0         },
        { S_HOLD,         70,     TYP_SCROLL,   ISBLESSED },
};

/*
 * these are the spells that a cleric can cast
 */

struct spells cleric_spells[MAXPRAYERS] = {
        { P_MFIND,         3,     TYP_POTION,   0         },
        { P_TFIND,         5,     TYP_POTION,   0         },
        { S_LIGHT,         7,     TYP_SCROLL,   ISBLESSED },
        { S_REMOVE,       10,     TYP_SCROLL,   0         },
        { P_FFIND,        15,     TYP_POTION,   0         },
        { P_FLY,          20,     TYP_POTION,   0         },
        { P_HEALING,      25,     TYP_POTION,   0         },
        { S_CURING,       30,     TYP_SCROLL,   0         },
        { P_RESTORE,      35,     TYP_POTION,   0         },
        { S_MAP,          40,     TYP_SCROLL,   0         },
        { P_SEEINVIS,     45,     TYP_POTION,   ISBLESSED },
        { P_CLEAR,        50,     TYP_POTION,   0         },
        { P_PHASE,        55,     TYP_POTION,   0         },
        { WS_CURING,      60,     TYP_STICK,    ISBLESSED },
        { WS_PARALYZE,    65,     TYP_STICK,    0         },
        { S_ALLENCH,      70,     TYP_SCROLL,   0         },
};

/*
 * these are the spells that a druid can chant
 */

struct spells druid_spells[MAXCHANTS] = {
        { P_MFIND,         3,     TYP_POTION,   0         },
        { P_TFIND,         5,     TYP_POTION,   0         },
        { S_LIGHT,         7,     TYP_SCROLL,   ISBLESSED },
        { S_REMOVE,       10,     TYP_SCROLL,   0         },
        { S_FINDTRAPS,    15,     TYP_SCROLL,   0         },
        { S_CONFUSE,      20,     TYP_SCROLL,   0         },
        { P_FFIND,        25,     TYP_POTION,   0         },
        { P_HEALING,      30,     TYP_POTION,   0         },
        { S_MAP,          35,     TYP_SCROLL,   0         },
        { P_CLEAR,        40,     TYP_POTION,   0         },
        { P_COLD,         45,     TYP_POTION,   0         },
        { P_FIRE,         50,     TYP_POTION,   0         },
        { P_PHASE,        55,     TYP_POTION,   0         },
        { P_LIGHTNING,    60,     TYP_POTION,   0         },
        { S_CHARM,        65,     TYP_SCROLL,   ISBLESSED },
        { S_HOLD,         70,     TYP_SCROLL,   ISBLESSED },
};

/*
 * these are the scrolls that a quill can write
 */

struct spells quill_scrolls[MAXQUILL] = {
        { S_GFIND,       5,   },
        { S_IDENT,       10,  },
        { S_LIGHT,       10,  },
        { S_REMOVE,      15,  },
        { S_MAP,         20,  },
        { S_CONFUSE,     25,  },
        { S_SLEEP,       30,  },
        { S_CURING,      40,  },
        { S_TELEP,       50,  },
        { S_SCARE,       60,  },
        { S_HOLD,        70,  },
        { S_PETRIFY,     80,  },
        { S_PROTECT,     90,  },
        { S_ALLENCH,     100, },
};

/*
 * Experience-level names of each character  (see NUM_CNAMES in rogue.h)
 */

const char *cnames[NUM_CHARTYPES-1][NUM_CNAMES] = {
{       "Veteran",                      "Fighter",              /* Fighter */
        "Ruffian",                      "Tussler",
        "Swordsman",                    "Hero",
        "Bravo",                        "Picador",
        "Stalwart",                     "Bashar",
        "Swashbuckler",                 "Myrmidon",
        "Fusileer",                     "Pugilist",
        "Champion",                     "Superhero",
        "Warrior",                      "Lord",
        "Lord I",                       "Lord II",
        "Lord III",                     "Lord IV",
        "Lord V",                       "Lord VI",
        "Lord VII",                     "Warrior Lord"
},
{       "Runner",                       "Strider",              /* Ranger */
        "Warden",                       "Steward",
        "Scout",                        "Courser",
        "Tracker",                      "Guide",
        "Protector",                    "Bartizan",
        "Gendarme",                     "Sentinel",
        "Vigilant",                     "Pathfinder",
        "Guardian",                     "Overseer",
        "Castellan",                    "Ranger",
        "Lord Ranger I",                "Lord Ranger II",
        "Lord Ranger III",              "Lord Ranger IV",
        "Lord Ranger V",                "Lord Ranger VI",
        "Lord Ranger VII",              "Master Ranger"
},
{       "Gallant",                      "Keeper",               /* Paladin */
        "Bravado",                      "Brazen",
        "Protector",                    "Defender",
        "Warder",                       "Guardian",
        "Champion",                     "Bulwark",
        "Venturist",                    "Inspirator",
        "Chevalier",                    "Justiciar",
        "Undaunteer",                   "Plautus",
        "Knight",                       "Paladin",
        "Paladin I",                    "Paladin II",
        "Paladin III",                  "Paladin IV",
        "Paladin V",                    "Paladin VI",
        "Paladin VII",                  "Lord Paladin"
},
{       "Prestidigitator",              "Evoker",               /* Magic User */
        "Summoner",                     "Invoker",
        "Conjurer",                     "Theurgist",
        "Illusionist",                  "Diviner",
        "Thaumaturgist",                "Magician",
        "Thelemist",                    "Magus",
        "Enchanter",                    "Warlock",
        "Witch",                        "Shaman",
        "Sorcerer",                     "Wizard",
        "Wizard I",                     "Wizard II",
        "Wizard III",                   "Wizard IV",
        "Wizard V",                     "Wizard VI",
        "Wizard VII",                   "Lord Magus"
},
{       "Acolyte",                      "Adept",                /* Cleric */
        "Charmer",                      "Friar",
        "Priest",                       "Curate",
        "Vicar",                        "Deacon",
        "Sabiast",                      "Cabalist",
        "Prefect",                      "Canon",
        "Minister",                     "Cardinal",
        "Bishop",                       "Patriarch",
        "Exorcist",                     "Archdeacon",
        "High Priest I",                "High Priest II",
        "High Priest III",              "High Priest IV",
        "High Priest V",                "High Priest VI",
        "High Priest VII",              "Reverend Lord"
},
{       "Rogue",                        "Footpad",              /* Thief */
        "Cutpurse",                     "Robber",
        "Vagrant",                      "Truant",
        "Burglar",                      "Filcher",
        "Sharper",                      "Magsman",
        "Racketeer",                    "Prowler",
        "Crook",                        "Bounder",
        "Quisling",                     "Malfeasor",
        "Swindler",                     "Thief",
        "Master Thief I",               "Master Thief II",
        "Master Thief III",             "Master Thief IV",
        "Master Thief V",               "Master Thief VI",
        "Master Thief VII",             "Master Rogue"
},
{       "Bravo",                        "Rutterkin",            /* Assassin */
        "Waghalter",                    "Murderer",
        "Butcher",                      "Desperado",
        "Thug",                         "Killer",
        "Cutthroat",                    "Executioner",
        "Eradicator",                   "Obliterator",
        "Mechanic",                     "Wiseguy",
        "Nihilist",                     "Berserker",
        "Assassin",                     "Expert Assassin",
        "Prime Assassin I",             "Prime Assassin II",
        "Prime Assassin III",           "Prime Assassin IV",
        "Prime Assassin V",             "Prime Assassin VI",
        "Prime Assassin VII",           "Master Assassin"
},
{       "Aspirant",                     "Ovate",                /* Druid */
        "Practitioner",                 "Devoutist",
        "Initiate 1st Circle",          "Initiate 2nd Circle",
        "Initiate 3rd Circle",          "Initiate 4th Circle",
        "Initiate 5th Circle",          "Initiate 6th Circle",
        "Initiate 7th Circle",          "Initiate 8th Circle",
        "Initiate 9th Circle",          "Illuminati",
        "Lesser Druid",                 "Arch Druid",
        "Druid",                        "Master Druid",
        "Master Druid I",               "Master Druid II",
        "Master Druid III",             "Master Druid IV",
        "Master Druid V",               "Master Druid VI",
        "Master Druid VII",             "Lord Druid"
},
{       "Novice",                       "Initiate",             /* Monk */
        "Brother",                      "Disciple",
        "Canon",                        "Elder",
        "Precept",                      "Lama",
        "Immaculate",                   "Wizard",
        "Shaman",                       "Master",
        "Superior Master",              "Master of Dragons",
        "Master of North Wind",         "Master of West Wind",
        "Master of South Wind",         "Master of East Wind",
        "Grand Master I",               "Grand Master II",
        "Grand Master III",             "Grand Master IV",
        "Grand Master V",               "Grand Master VI",
        "Grand Master VII",             "Lord Monk"
}
};

