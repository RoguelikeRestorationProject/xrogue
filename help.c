/*
    help.c  -  Routines having to do with help
    
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
#include "mach_dep.h"
#include "rogue.h"

/*
 * Give character descripts
 */

static char *game_fighter = "Strength is the main attribute of the Fighter.  \
He can wield any weapon and the two-handed sword is his weapon of choice.  He \
can also wear any type of armor.  Plate armor being the best choice and \
leather armor the worst.  The Fighter is  able to sense both traps and gold \
at higher experience levels.  His natural     quest item is the Axe of Aklad.  \
Due to his superior form, the Fighter usually  receives more hit-points per \
new experience level than the other characters.    The Fighter is neither good \
or evil; he is a neutral character.  The default    attribute values of the \
Fighter are: Int=7, Str=16, Wis=7, Dxt=16, Con=17, and  Chr=11.  Default gold \
amount is 2000 pieces and default hit-points are 24.";

static char *game_thief = "Dexterity is the main attribute of the Thief.  His \
stealth allows him to move   quietly, thus disturbing less monsters.  He can \
sense traps and gold and can    take (steal) things from the monsters.  The \
Thief can not wield the two-handed  sword so the bastard sword is his weapon \
of choice.  He can only wear studded   leather armor.  The Thief's natural \
quest item is the Daggers of Musty Doit.    With higher dexterity the Thief \
is able to \"backstab\" monsters, thereby killing them with a single blow.  \
His character type fluctuates between that of good,   neutral, and evil.  The \
default attribute values of the Thief are:  Int=7,      Str=14, Wis=7, Dxt=18, \
Con=17, and Chr=11.  Default gold amount is 2000 pieces  and default \
hit-points are 23.";

static char *game_assassin = "Dexterity is the main attribute of the \
Assassin.  Like the Thief, he moves with an extra degree of stealth.  He can \
sense gold and steal things from monsters.  The ability to sense traps comes \
at higher experience levels.  The Assassin can not wield the two-handed sword \
and he can only wear studded leather armor.  The natural quest item of the \
Assassin is the Eye of Vecna.  He is also skilled in  the use of poison.  \
Higher dexterity enables him to \"assassinate\" monsters with a single blow.  \
The Assassin is aligned with the powers of evil.  The default   attribute \
values of the Assassin are: Int=7, Str=14, Wis=7, Dxt=18, Con=17, and \
Chr=11.  Default gold amount is 2000 pieces and default hit-points are 23.";

static char *game_ranger = "Charisma is the main attribute of the Ranger who \
also has a secondary attribute of Intelligence.  Like the Magician, this \
gives him the ability to cast spells  which increases as he attains higher \
experience levels.  Like the Fighter, he   can wield any weapon and wear any \
armor.  The Ranger's natural quest item is    the Mandolin of Brian.  He is \
aligned with the powers of good.  Therefore, he   can be made to suffer and \
even become cursed by the very powers that allow him  to cast spells if he \
happens to cause the demise of a likewise good creature.   The default \
attribute values of the Ranger are: Int=11, Str=11, Wis=7, Dxt=16,  Con=16, \
and Chr=13.  Default gold amount is 2000 pieces and default hit-points  \
are 22.";

static char *game_paladin = "Charisma is the main attribute of the Paladin \
who has a secondary attribute of  Wisdom.  Like the Cleric, this gives him \
the ability to offer prayers, receive  what his heart desires, and an ability \
to turn the undead.  This ability will   increase as he gains higher \
experience levels.  Like the Fighter, he can wield  any weapon and wear any \
armor.  The Ankh of Heil is the Paladin's natural quest item.  Like the \
Ranger, the Paladin is aligned with the powers of good.  This   can cause \
him to suffer and become cursed if he brings ruin to a likewise good  \
creature.  The default attribute values of the Paladin are:  Int=7, \
Str=11,     Wis=11, Dxt=16, Con=16, and Chr=13.  Default gold amount is \
2000 pieces and     default hit-points are 22.";

static char *game_druid = "Wisdom is the main attribute of the Druid.  This \
gives him the ability to chant secret words and mantras, which is much \
greater than that of the Monk.  The     Druid can not wield the two-handed or \
bastard swords but he can wear any armor. His natural quest item is the Quill \
of Nagrom.  Like the Magician and Cleric,   the Druid is aligned neutral.  He \
therefore, must rely upon his Wisdom and his  chanting ability in order to \
remain alive.  Likewise, he does not receive as    many new hit-points per \
new experience level.  The default attribute values of  the Druid are: Int=7, \
Str=10, Wis=14, Dxt=16, Con=15, and Chr=12.  Default gold amount is 2000 \
pieces and default hit-points are 21.";

static char *game_monk = "Constitution is the main attribute of the Monk who \
has a secondary aspect of    Wisdom.  Like the Druid, this gives him the \
ability to chant mantras which will increase as he gains higher experience.  \
The Monk can not wield the two-handed  sword and he can not wear any armor.  \
The Cloak of Emori is the Monk's natural  quest item.  The Monk can also \
sense traps, though much less than the Thief or  Assassin.  He is the most \
healthy character.  Like the Ranger and Paladin, he   is aligned with the \
powers of good.  Therefore, he is made to suffer and can    become cursed if \
he kills a likewise good creature.  The default attribute      values of the \
Monk are: Int=7, Str=11, Wis=11, Dxt=16, Con=18, and Chr=11.      Default \
gold amount is 2000 pieces and default hit-points are 22.";

static char *game_magician = "Intelligence is the main attribute of the \
Magician.  The Magician's ability to  cast spells is much greater than that \
of the Ranger.  He can not wield the two- handed or bastard swords, but he \
can wear any kind of armor.  His natural quest item is the Amulet of \
Stonebones.  The Magician is aligned neutral.  He must    rely upon his \
Intelligence and spell casting abilities to remain alive.  There- fore, he \
does not receive as many new hit-points per new experience level.  The \
default attribute values of the Magician are: Int=14, Str=10, Wis=7, \
Dxt=16,    Con=15, and Chr=12.  Default gold amount is 2000 pieces and \
default hit-points  are 21.";

static char *game_cleric = "Wisdom is the main attribute of the Cleric.  The \
Cleric's ability to give or    offer prayers, receive their due, and affect \
the undead are much greater than   that of the Paladin.  Like the Magician, \
the Cleric can not wield the two-      handed or bastard swords, but he can \
wear any armor.  His natural quest item is the Horn of Geryon.  The Cleric \
is aligned neutral  and he must rely upon his   Wisdom and prayer ability to \
remain alive.  He therefore, does not receive as   many new hit-points per \
new experience level.  The default attribute values of  the Cleric are: Int=7, \
Str=10, Wis=14, Dxt=16, Con=15, and Chr=12.  The default gold amount is 2000 \
pieces and default hit-points are 21.";


static char *game_food ="There are three types of food, regular food rations, \
various fruits, and slime- molds.  Eating regular food will add 750 points to \
your current food level      [see the CTRL(E) command].  Eating fruit adds \
300 points.  Certain fruits also  cure you, add an attribute point, add a \
hit-point, increase your armor, give    you additional prayer, chant, or spell \
casting abilities, or add experience     points.  Eating slime-mold (monster \
food) can make you ill, but they will add   100 points to your current food \
level.  If your food level points drop below    100 you will become weak.  You \
will faint and might die if they drop to 0 or    below.  At the other extreme, \
if your food level points reach 2000 (and above)  you will become satiated.  \
Risk eating more and you could choke to death.";

/*
static char *game_monst ="To be updated.";
static char *game_potion ="To be updated...";
static char *game_scroll ="To be updated...";
static char *game_ring ="To be updated...";
static char *game_stick ="To be updated...";
static char *game_weapon ="To be updated...";
static char *game_armor ="To be updated...";
static char *game_miscm ="To be updated...";
static char *game_qitems ="To be updated...";
static char *game_dungeon ="To be updated...";
static char *game_traps ="To be updated...";
static char *game_mazes ="To be updated...";
static char *game_option ="To be updated...";
static char *game_begin ="To be updated...";
*/

/* help list */
static struct h_list helpstr[] = {
    '?',     "    Print help",
    '/',     "    Identify object",
    '=',     "    Identify a screen character",
    ' ',     "",
    'h',     "    Move left",
    'j',     "    Move down",
    'k',     "    Move up",
    'l',     "    Move right",
    'y',      "    Move up and left",
    'u',        "    Move up and right",
    'b',        "    Move down and left",
    'n',        "    Move down and right",
    'H',        "    Run left",
    'J',        "    Run down",
    'K',        "    Run up",
    'L',        "    Run right",
    'Y',        "    Run up & left",
    'U',        "    Run up & right",
    'B',        "    Run down & left",
    'N',        "    Run down & right",
    ' ',        "",
    '>',        "    Go down a staircase",
    '<',        "    Go up a staircase",
    '\\',        "  Game descriptions",
    '.',        "   Rest for a while",
    '*',        "   Count gold pieces",
    'a',        "   Affect the undead",
    'A',        "   Choose artifact (equipage)",
    'c',        "   Chant a mantra",
    'C',        "   Cast a spell",
    'd',        "   Drop something",
    'D',        "   Dip something (into a pool)",
    'e',        "   Eat food or fruit",
    'f',        "<dir>  Forward until find something",
    'F',        "   Frighten a monster",
    'g',        "   Give food to monster",
    'G',        "   Sense for gold",
    'i',        "   Inventory",
    'I',        "   Inventory (single item)",
    'm',        "   Mark an object (specific)",
    'o',        "   Examine and/or set options",
    'O',        "   Character type and quest item",
    'p',        "   Pray to the powers that be",
    'P',        "   Pick up object(s)",
    'q',        "   Quaff a potion",
    'Q',        "   Quit the game",
    'r',        "   Read a scroll",
    's',        "   Search for a trap/secret door",
    'S',        "   Save your game",
    't',        "<dir>  Throw something",
    'T',        "   Take off something",
    'v',        "   Print program version",
    'w',        "   Wield a weapon",
    'W',        "   Wear something",
    'X',        "   Sense for traps",
    'z',        "<dir>  Zap a wand or staff",
    ' ',        "",
    '^',        "   Set a trap",
    '$',        "   Price an item (trading post)",
    '#',        "   Buy an item   (trading post)",
    '%',        "   Sell an item  (trading post)",
    '!',        "   Shell escape",
    ESC,        "   Cancel command (Esc)",
    ' ',        "",
    CTRL('B'),    " Current score (if you win)",
    CTRL('E'),    " Current food level",
    CTRL('L'),    " Redraw the screen",
    CTRL('N'),    " Name an object or a monster",
    CTRL('O'),    " Character affect status",
    CTRL('R'),    " Repeat last message",
    CTRL('T'),    "<dir>    Take (steal) from (direction)",
    CTRL('U'),    " Use a magic item",
    0, 0
} ;

/* wizard help list */
static struct h_list wiz_help[] = {
    ' ',        "",
    '+',        "   Random fortunes",
    'M',        "   Make an object",
    'V',        "   Display vlevel and turns",
    CTRL('A'),    " System activity",
    CTRL('C'),    " Move to another dungeon level",
    CTRL('D'),    " Go down 1 dungeon level",
    CTRL('F'),    " Display the entire level",
    CTRL('G'),    " Charge wands and staffs",
    CTRL('H'),    " Jump 9 experience levels",
    CTRL('I'),    " Inventory of level",
    CTRL('J'),    " Teleport somewhere",
    CTRL('K'),    " Identify an object",
    CTRL('M'),    " Recharge wand or staff",
    CTRL('P'),    " Toggle wizard status",
    CTRL('X'),    " Detect monsters",
    CTRL('Y'),    " Display food levels",
    0, 0
};

/* item help list */
static struct item_list item_help[] = {
    '@',   "   You (visible)",
    '_',   "   You (invisible)",
    ' ',   "",
    ':',   "   Food ration or fruit (eat)",
    '!',   "   Potion (quaff)",
    '?',   "   Scroll (read)",
    '=',   "   Ring (wear)",
    ')',   "   Weapon (wield)",
    ']',   "   Armor (wear)",
    '/',   "   Wand or staff (zap)",
    ';',   "   Magic item (use)",
    ',',   "   Artifact (quest item)",
    '*',   "   Gold or zapped missile",
    ' ',   "",
    '$',   "   Magical item in room",
    '>',   "   Blessed magical item",
    '<',   "   Cursed magical item",
    ' ',   " ",
    '`',   "   Dart trap",
    '{',   "   Arrow trap",
    '}',   "   Bear trap",
    '~',   "   Teleport trap",
    '$',   "   Sleeping gas trap",
    '>',   "   Trap door",
    '<',   "   Outer region entrance",
    '\'',   "   Maze entrance",
    '^',   "   Trading post entrance",
    '"',   "   Magic pool or lake",
    ' ',   "   Solid rock or mountain",
    '.',   "   Floor of a room or meadow",
    '%',   "   Stairs (up or down)",
    '+',   "   Doorway",
    '&',   "   Secret doorway",
    '#',   "   Passage between rooms",
    '\\',   "   Forest",
    HORZWALL,   "   Horizontal wall of a room",
    VERTWALL,   "   Vertical wall of a room",
    0, 0
};

ident_hero()
{
    bool doit = TRUE;

    wclear(hw);
    wprintw(hw, "Characters, Items, and Game Descriptions:\n");
    wprintw(hw, "-----------------------------------------\n");
    wprintw(hw, "a) Fighter        m) Scrolls\n");
    wprintw(hw, "b) Thief          n) Rings\n");
    wprintw(hw, "c) Assassin       o) Wands and Staffs\n");
    wprintw(hw, "d) Ranger         p) Weapons\n");
    wprintw(hw, "e) Paladin        q) Armors\n");
    wprintw(hw, "f) Monk           r) Miscellaneous Magic Items\n");
    wprintw(hw, "g) Magician       s) Quest Items (Artifacts and Relics)\n");
    wprintw(hw, "h) Cleric         t) The Dungeon\n");
    wprintw(hw, "i) Druid          u) Traps\n");
    wprintw(hw, "j) Monsters       v) Mazes and Outer Regions\n");
    wprintw(hw, "k) Foods          w) Setting game options\n");
    wprintw(hw, "l) Potions        x) Starting out\n");
    wprintw(hw, "\nEnter a letter: ");
    draw(hw);
    while (doit) {
    switch (wgetch(cw)) {
       case EOF:
       case ESC:
        doit = FALSE;
       when 'a':
            wclear(hw);
            wprintw(hw, "Fighter Characteristics:");
            mvwaddstr(hw, 2, 0, game_fighter);
        draw(hw);
        doit = FALSE;
       when 'b':
            wclear(hw);
            wprintw(hw, "Thief Characteristics:");
            mvwaddstr(hw, 2, 0, game_thief);
        draw(hw);
        doit = FALSE;
       when 'c':
            wclear(hw);
            wprintw(hw, "Assassin Characteristics:");
            mvwaddstr(hw, 2, 0, game_assassin);
            draw(hw);
        doit = FALSE;
        when 'd':
            wclear(hw);
            wprintw(hw, "Ranger Characteristics:");
            mvwaddstr(hw, 2, 0, game_ranger);
            draw(hw);
        doit = FALSE;
        when 'e':
            wclear(hw);
            wprintw(hw, "Paladin Characteristics:");
            mvwaddstr(hw, 2, 0, game_paladin);
            draw(hw);
        doit = FALSE;
        when 'f':
            wclear(hw);
            wprintw(hw, "Monk Characteristics:");
            mvwaddstr(hw, 2, 0, game_monk);
            draw(hw);
        doit = FALSE;
        when 'g':
            wclear(hw);
            wprintw(hw, "Magician Characteristics:");
            mvwaddstr(hw, 2, 0, game_magician);
            draw(hw);
        doit = FALSE;
        when 'h':
            wclear(hw);
            wprintw(hw, "Cleric Characteristics:");
            mvwaddstr(hw, 2, 0, game_cleric);
            draw(hw);
        doit = FALSE;
        when 'i':
            wclear(hw);
            wprintw(hw, "Druid Characteristics:");
            mvwaddstr(hw, 2, 0, game_druid);
            draw(hw);
        doit = FALSE;
        when 'j':
            wclear(hw);
            wprintw(hw, "Monster Characteristics:");
            draw(hw);
        doit = FALSE;
        when 'k':
            wclear(hw);
            wprintw(hw, "Foods:");
            mvwaddstr(hw, 2, 0, game_food);
            draw(hw);
        doit = FALSE;
        when 'l':
            wclear(hw);
            wprintw(hw, "Potions:");
            draw(hw);
        doit = FALSE;
        when 'm':
            wclear(hw);
            wprintw(hw, "Scrolls:");
            draw(hw);
        doit = FALSE;
        when 'n':
            wclear(hw);
            wprintw(hw, "Rings:");
            draw(hw);
        doit = FALSE;
        when 'o':
            wclear(hw);
            wprintw(hw, "Wands and Staffs:");
            draw(hw);
        doit = FALSE;
        when 'p':
            wclear(hw);
            wprintw(hw, "Weapons:");
            draw(hw);
        doit = FALSE;
        when 'q':
            wclear(hw);
            wprintw(hw, "Armors:");
            draw(hw);
        doit = FALSE;
        when 'r':
            wclear(hw);
            wprintw(hw, "Miscellaneous Magic Items:");
            draw(hw);
        doit = FALSE;
        when 's':
            wclear(hw);
            wprintw(hw, "Quest Items (Artifacts and Relics):");
            draw(hw);
        doit = FALSE;
        when 't':
            wclear(hw);
            wprintw(hw, "The Dungeon:");
            draw(hw);
        doit = FALSE;
        when 'u':
            wclear(hw);
            wprintw(hw, "Traps:");
            draw(hw);
        doit = FALSE;
        when 'v':
            wclear(hw);
            wprintw(hw, "Mazes and Outer Regions:");
            draw(hw);
        doit = FALSE;
        when 'w':
            wclear(hw);
            wprintw(hw, "Setting game options:");
            draw(hw);
        doit = FALSE;
        when 'x':
            wclear(hw);
            wprintw(hw, "Starting out:");
            draw(hw);
        doit = FALSE;
        otherwise:
        doit = TRUE;
    }
    }
    wmove(hw, lines-1, 0);
    wprintw(hw, spacemsg);
    draw(hw);
    wait_for(' ');
    wclear(hw);
    draw(hw);
    wmove(cw, 0, 0);
    wclrtoeol(cw);
    status(FALSE);
    touchwin(cw);
}

/*
 * Real Help
 */

help()
{
    register struct h_list *strp = helpstr;
    register struct item_list *itemp = item_help;
    struct h_list *wizp = wiz_help;
    register char helpch;
    register int cnt;

    msg("Character you want help for (* for commands, @ for items): ");
    helpch = wgetch(cw);
    mpos = 0;
    /*
     * If it's not a *, @, or +, then just print help string
     * for the character entered.  
     */
    if (helpch != '*' && helpch != '@' && helpch != '+') {
        wmove(msgw, 0, 0);
        while (strp->h_ch) {
            if (strp->h_ch == helpch) {
                msg("%s%s", unctrl(strp->h_ch), strp->h_desc);
                return;
            }
            strp++;
        }
        if (wizard) {
            while (wizp->h_ch) {
                if (wizp->h_ch == helpch) {
                    msg("%s%s", unctrl(wizp->h_ch), wizp->h_desc);
                    return;
                }
                wizp++;
            }
        }
        msg("Unknown command '%s'", unctrl(helpch));
        return;
    }

    /* fortunes - but let's not say so - explicitly */
    if (helpch == '+') {
    msg("Meaningless command '+'");
    return;
    }

    /*
     * Print help for everything else
     */
    if (helpch == '*') {
        wclear(hw);
        cnt = 0;
        while (strp->h_ch) {
            mvwaddstr(hw, cnt % 23, cnt > 22 ? 40 : 0, unctrl(strp->h_ch));
            waddstr(hw, strp->h_desc);
            strp++;
            if (++cnt >= 46 && strp->h_ch) {
                wmove(hw, lines-1, 0);
                wprintw(hw, morestr);
                draw(hw);
                wait_for(' ');
                wclear(hw);
                cnt = 0;
            }
        }
        if (wizard) {
            while (wizp->h_ch) {
                mvwaddstr(hw, cnt % 23, cnt > 22 ? 40 : 0, unctrl(wizp->h_ch));
                waddstr(hw, wizp->h_desc);
                wizp++;
                if (++cnt >= 46 && wizp->h_ch) {
                    wmove(hw, lines-1, 0);
                    wprintw(hw, morestr);
                    draw(hw);
                    wait_for(' ');
                    wclear(hw);
                    cnt = 0;
                }
            }
        }
    }
    if (helpch == '@') {
        wclear(hw);
        cnt = 0;
        while (itemp->item_ch) {
            mvwaddstr(hw, cnt % 23, cnt > 22 ? 40 : 0, unctrl(itemp->item_ch));
            waddstr(hw, itemp->item_desc);
            itemp++;
            if (++cnt >= 46 && itemp->item_ch) {
                wmove(hw, lines-1, 0);
                wprintw(hw, morestr);
                draw(hw);
                wait_for(' ');
                wclear(hw);
                cnt = 0;
            }
        }
    }
    wmove(hw, lines-1, 0);
    wprintw(hw, spacemsg);
    draw(hw);
    wait_for(' ');
    wclear(hw);
    draw(hw);
    wmove(cw, 0, 0);
    wclrtoeol(cw);
    status(FALSE);
    touchwin(cw);
}

/*
 * identify:
 *      Tell the player what a certain thing is.
 */

identify(ch)
register unsigned char ch;
{
    register char *str = NULL;

    if (ch == 0) {
        msg("What do you want identified? ");
        ch = wgetch(cw);
        mpos = 0;
        if (ch == ESC)
        {
            msg("");
            return;
        }
    }
    if (isalpha(ch))
        msg("Use the \"=\" command to identify monsters. ");
    else switch(ch)
    {
        case VPLAYER:   str = "You (visibly)";
        when IPLAYER:   str = "You (invisibly)";
        when GOLD:      str = "Gold";
        when STAIRS:    str = (levtype == OUTSIDE) ? "Entrance to the dungeon"
                                                   : "Stairway";
        when DOOR:      str = "Doorway";
        when SECRETDOOR:str = "Secret door";
        when FLOOR:     str = (levtype == OUTSIDE) ? "Meadow" : "Room floor";
        when PASSAGE:   str = "Passage";
        when VERTWALL:
        case HORZWALL:
            str = (levtype == OUTSIDE) ? "Boundary of sector"
                                       : "Wall of a room";
        when POST:      str = "Trading post";
        when POOL:      str = (levtype == OUTSIDE) ? "Lake"
                                                   : "A shimmering pool";
        when TRAPDOOR:  str = "Trap door";
        when ARROWTRAP: str = "Arrow trap";
        when SLEEPTRAP: str = "Sleeping gas trap";
        when BEARTRAP:  str = "Bear trap";
        when TELTRAP:   str = "Teleport trap";
        when DARTTRAP:  str = "Dart trap";
        when MAZETRAP:  str = "Entrance to a maze";
        when WORMHOLE:  str = "Entrance to a worm hole";
        when FOREST:    str = "Forest";
        when ' ' :      str = (levtype == OUTSIDE) ? "Mountain"
                                                   : "Solid rock";
        when FOOD:      str = "Food";
        when POTION:    str = "Potion";
        when SCROLL:    str = "Scroll";
        when RING:      str = "Ring";
        when WEAPON:    str = "Weapon";
        when ARMOR:     str = "Armor";
        when MM:        str = "Miscellaneous magic";
        when STICK:     str = "Wand or staff";
        when RELIC:     str = "Artifact";
        otherwise:      str = "Unknown character";
    }
    if (!isalpha(ch))
        msg("%s     %s", unctrl(ch), str);
}


