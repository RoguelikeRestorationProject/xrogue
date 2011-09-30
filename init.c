/*
    init.c - global variable initializaton
 
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
#include "rogue.h"
#include "mach_dep.h"

/*
 * If there is any news, put it in a character string and assign it to
 * rogue_news.  Otherwise, assign NULL to rogue_news.
 */

static char *rogue_news = "Enter a number within the minimum and maximum \
range.  When satisfied with your  choices, enter a 'y'.  For help at any \
other time enter a '?' or a '='.";

/* replace the above line with this when descriptions are done  */
/* other time enter a '?' or a '='.         For character and item descriptions \
enter a '\\' on any other screen.";
*/

struct words rainbow[NCOLORS] = {
"Amber",                "Aquamarine",           "Beige",
"Black",                "Blue",                 "Brown",
"Clear",                "Crimson",              "Ecru",
"Gold",                 "Green",                "Grey",
"Indigo",               "Khaki",                "Lavender",
"Magenta",              "Orange",               "Pink",
"Plaid",                "Purple",               "Red",
"Silver",               "Saffron",              "Scarlet",
"Tan",                  "Tangerine",            "Topaz",
"Turquoise",            "Vermilion",            "Violet",
"White",                "Yellow",
};

struct words sylls[NSYLLS] = {
    "a",   "ae",  "ak",  "an",  "ax",  "ach", "ano", "ars", "bha", "bar", "bre",
    "cha", "cre", "cum", "cow", "duh", "dha", "e",   "ea",  "em",  "et",  "ey",
    "eck", "etk", "egg", "exl", "fu",  "fen", "fid", "gan", "gle", "h",   "ha",
    "hr",  "ht",  "how", "hex", "hip", "hoc", "i",   "ia",  "ig",  "it",  "iz",
    "ion", "ink", "ivi", "iss", "je",  "jin", "jha", "jyr", "ka",  "kho", "kal",
    "kli", "lu",  "lre", "lta", "lri", "m",   "ma",  "mh",  "mi",  "mr",  "mar",
    "myr", "moh", "mul", "nep", "nes", "o",   "oc",  "om",  "oq",  "ox",  "orn",
    "oxy", "olm", "ode", "po",  "pie", "pod", "pot", "qar", "que", "ran", "rah",
    "rok", "sa",  "sat", "sha", "sol", "sri", "ti",  "tem", "tar", "tki", "tch",
    "tox", "u",   "ub",  "uh",  "ur",  "uv",  "unk", "uwh", "ugh", "uyr", "va",
    "vil", "vit", "vom", "vux", "wah", "wex", "xu",  "xed", "xen", "ya",  "yep",
    "yih", "zef", "zen", "zil", "zym", "-"
};

struct words stones[NSTONES] = {
        "Agate",                "Alexandrite",          "Amethyst",
        "Azurite",              "Bloodstone",           "Cairngorm",
        "Carnelian",            "Chalcedony",           "Chrysoberyl",
        "Chrysolite",           "Chrysoprase",          "Citrine",
        "Coral",                "Diamond",              "Emerald",
        "Garnet",               "Heliotrope",           "Hematite",
        "Hyacinth",             "Jacinth",              "Jade",
        "Jargoon",              "Jasper",               "Kryptonite",
        "Lapis lazuli",         "Malachite",            "Mocca stone",
        "Moonstone",            "Obsidian",             "Olivine",
        "Onyx",                 "Opal",                 "Pearl",
        "Peridot",              "Quartz",               "Rhodochrosite",
        "Rhodolite",            "Ruby",                 "Sapphire",
        "Sardonyx",             "Serpentine",           "Spinel",
        "Tiger eye",            "Topaz",                "Tourmaline",
        "Turquoise",            "Zircon",
};

struct words wood[NWOOD] = {
        "Avocado wood", "Balsa",        "Banyan",       "Birch",
        "Cedar",        "Cherry",       "Cinnabar",     "Dogwood",
        "Driftwood",    "Ebony",        "Eucalyptus",   "Hemlock",
        "Ironwood",     "Mahogany",     "Manzanita",    "Maple",
        "Oak",          "Pine",         "Redwood",      "Rosewood",
        "Teak",         "Walnut",       "Aloe",         "Sandalwood",
};

struct words metal[NMETAL] = {
        "Aluminium",    "Bone",         "Brass",        "Bronze",
        "Copper",       "Chromium",     "Iron",         "Lead",
        "Magnesium",    "Pewter",       "Platinum",     "Silver",
        "Steel",        "Tin",          "Titanium",     "Zinc",
};

/*
 * make sure all the percentages specified in the tables add up to the
 * right amounts
 */

badcheck(name, magic, bound)
char *name;
register struct magic_item *magic;
register int bound;
{
    register struct magic_item *end;

    if (magic[bound - 1].mi_prob == 1000)
        return;
    printf("\nBad percentages for %s:\n", name);
    for (end = &magic[bound] ; magic < end ; magic++)
        printf("%4d%% %s\n", magic->mi_prob, magic->mi_name);
    printf(retstr);
    fflush(stdout);
    while (getchar() != '\n')
        continue;
}

/*
 * init_colors:
 *      Initialize the potion color scheme for this time
 */

init_colors()
{
    register int i;
    register char *str;

    for (i = 0 ; i < MAXPOTIONS ; i++)
    {
        do
            str = rainbow[rnd(NCOLORS)].w_string;
        until (isupper(*str));
        *str = tolower(*str);
        p_colors[i] = str;
        p_know[i] = FALSE;
        p_guess[i] = NULL;
        if (i > 0)
                p_magic[i].mi_prob += p_magic[i-1].mi_prob;
    }
    badcheck("potions", p_magic, MAXPOTIONS);
}

/*
 * do any initialization for food
 */

init_foods()
{
    register int i;

    for (i=0; i < MAXFOODS; i++) {
        if (i > 0)
            foods[i].mi_prob += foods[i-1].mi_prob;
    }
    badcheck("foods", foods, MAXFOODS);
}

/*
 * init_materials:
 *      Initialize the construction materials for wands and staffs
 */

init_materials()
{
    register int i;
    register char *str;

    for (i = 0 ; i < MAXSTICKS ; i++)
    {
        do
            if (rnd(100) > 50)
            {
                str = metal[rnd(NMETAL)].w_string;
                if (isupper(*str))
                        ws_type[i] = "wand";
            }
            else
            {
                str = wood[rnd(NWOOD)].w_string;
                if (isupper(*str))
                        ws_type[i] = "staff";
            }
        until (isupper(*str));
        *str = tolower(*str);
        ws_made[i] = str;
        ws_know[i] = FALSE;
        ws_guess[i] = NULL;
        if (i > 0)
                ws_magic[i].mi_prob += ws_magic[i-1].mi_prob;
    }
    badcheck("sticks", ws_magic, MAXSTICKS);
}

/*
 * do any initialization for miscellaneous magic
 */

init_misc()
{
    register int i;

    for (i=0; i < MAXMM; i++) {
        m_know[i] = FALSE;
        m_guess[i] = NULL;
        if (i > 0)
            m_magic[i].mi_prob += m_magic[i-1].mi_prob;
    }
    badcheck("miscellaneous magic", m_magic, MAXMM);
}

/*
 * init_names:
 *      Generate the names of the various scrolls
 */

init_names()
{
    register int nsyl;
    register char *cp, *sp;
    register int i, nwords;

    for (i = 0 ; i < MAXSCROLLS ; i++)
    {
        cp = prbuf;
        nwords = rnd(cols/20) + 1 + (cols > 40 ? 1 : 0);
        while(nwords--)
        {
            nsyl = rnd(5)+1;
            while(nsyl--)
            {
                sp = sylls[rnd(NSYLLS)].w_string;
                while(*sp)
                    *cp++ = *sp++;
            }
            *cp++ = ' ';
        }
        *--cp = '\0';
        s_names[i] = (char *) new(strlen(prbuf)+1);
        s_know[i] = FALSE;
        s_guess[i] = NULL;
        strcpy(s_names[i], prbuf);
        if (i > 0)
                s_magic[i].mi_prob += s_magic[i-1].mi_prob;
    }
    badcheck("scrolls", s_magic, MAXSCROLLS);
}

/*
 * init_player:
 *      roll up the rogue
 */

init_player()
{
    int stat_total, round = 0, minimum, maximum, ch, i, j = 0;
    short do_escape, *our_stats[NUMABILITIES-1];
    struct linked_list  *weap_item, *armor_item, *food_item;
    struct object *obj;

    weap_item = armor_item = food_item = NULL;

    if (char_type == -1) {  /* not set via options */
        /* See what type character will be */
        wclear(hw);
        touchwin(hw);
        wmove(hw,2,0);
        for(i=1; i<=NUM_CHARTYPES-1; i++) {
            wprintw(hw,"[%d] %s\n",i,char_class[i-1].name);
        }
        mvwaddstr(hw, 0, 0, "What character class do you desire? ");
        draw(hw);
        char_type = (wgetch(hw) - '0');
        while (char_type < 1 || char_type > NUM_CHARTYPES-1) {
            wmove(hw,0,0);
            wprintw(hw,"Please enter a character type between 1 and %d: ",
                    NUM_CHARTYPES-1);
            draw(hw);
            char_type = (wgetch(hw) - '0');
        }
        char_type--;
    }
    player.t_ctype = char_type;
    player.t_quiet = 0;
    pack = NULL;

    /* Select the gold */
    purse = 3000;
    switch (player.t_ctype) {
        case C_FIGHTER:
            purse += 200;
        when C_MAGICIAN:
        case C_CLERIC:
        case C_DRUID:
            purse += 100;
        when C_THIEF:
        case C_ASSASSIN:
        purse += 0;
        when C_RANGER:
        case C_PALADIN:
            purse -= 100;
        when C_MONK:
            purse -= 200;
    }
    /* 
     * allow me to describe a super character 
     */
        /* let's lessen the restrictions on this okay? */
    if (wizard && strcmp(getenv("SUPER"),"YES") == 0) {
        pstats.s_str = MAXATT;
        pstats.s_intel = MAXATT;
        pstats.s_wisdom = MAXATT;
        pstats.s_dext = MAXATT;
        pstats.s_const = MAXATT;
        pstats.s_charisma = MAXATT;
        pstats.s_exp = 10000000L;
        pstats.s_lvl = 1;
        pstats.s_lvladj = 0;
        pstats.s_hpt = 500;
        pstats.s_carry = totalenc(&player);
        strcpy(pstats.s_dmg,"4d8");
        check_level();
        wmove(hw,0,0);
        wclrtoeol(hw);
        draw(hw);
        mpos = 0;

    /* set quest item */
    if(player.t_ctype == C_FIGHTER)  quest_item = AXE_AKLAD;
    if(player.t_ctype == C_RANGER)   quest_item = BRIAN_MANDOLIN;
    if(player.t_ctype == C_PALADIN)  quest_item = HEIL_ANKH;
    if(player.t_ctype == C_MAGICIAN) quest_item = STONEBONES_AMULET;
    if(player.t_ctype == C_CLERIC)   quest_item = GERYON_HORN;
    if(player.t_ctype == C_THIEF)    quest_item = MUSTY_DAGGER;
    if(player.t_ctype == C_ASSASSIN) quest_item = EYE_VECNA;
    if(player.t_ctype == C_DRUID)    quest_item = QUILL_NAGROM;
    if(player.t_ctype == C_MONK)     quest_item = EMORI_CLOAK;

    /* armor */
        if (player.t_ctype == C_THIEF || player.t_ctype == C_ASSASSIN)
            j = STUDDED_LEATHER;
        else if (player.t_ctype == C_MONK) {
        armor_item = spec_item(MM, MM_BRACERS, 20, 0);
        obj = OBJPTR(armor_item);
            obj->o_weight = things[TYP_MM].mi_wght;
        whatis (armor_item);  /* identify it */
            obj->o_flags |= (ISKNOW | ISPROT);
            add_pack(armor_item, TRUE);
            cur_misc[WEAR_BRACERS] = obj;
        goto w_armorjmp; 
    }
        else j =  PLATE_ARMOR;

        armor_item = spec_item(ARMOR, j, 20, 0);
        obj = OBJPTR(armor_item);
        obj->o_weight = armors[j].a_wght;
        obj->o_flags |= (ISKNOW | ISPROT);
        add_pack(armor_item, TRUE);
        cur_armor = obj;

    w_armorjmp:  /* monk doesn't wear armor */

        /* weapons */
        if (player.t_ctype == C_THIEF || player.t_ctype == C_ASSASSIN ||
            player.t_ctype == C_MONK) 
        j = BASWORD;
        else if (player.t_ctype == C_FIGHTER || player.t_ctype == C_RANGER ||
        player.t_ctype == C_PALADIN)
        j = TWOSWORD;
    else j = TRIDENT;

        weap_item = spec_item(WEAPON, j, 20, 20);
        obj = OBJPTR(weap_item);
        obj->o_flags |= (ISKNOW | ISPROT);
        obj->o_weight = weaps[j].w_wght;
        add_pack(weap_item, TRUE);
        cur_weapon = obj;

    /* food */
        food_item = spec_item(FOOD, E_RATION, 0, 0);
        obj = OBJPTR(food_item);
        obj->o_flags |= ISKNOW;
        obj->o_weight = foods[TYP_FOOD].mi_wght;
        add_pack(food_item, TRUE); /* just one */

    /* give wizard plenty gold */
        purse = 50000;
    }
    else 
    /* default attributes checked */
    {
    if (def_attr == TRUE) {  /* "default" option used in ROGUEOPTS */
    switch(player.t_ctype) {
        /* set "default attributes" option and quest items here */
        case C_FIGHTER:
        case C_MONK:
                pstats.s_intel = 7;
                pstats.s_dext = 16;
                pstats.s_charisma = 11;
            if (player.t_ctype == C_FIGHTER) {
                    pstats.s_str = 16;
                    pstats.s_wisdom = 7;
                    pstats.s_const = 17;
                quest_item = AXE_AKLAD;
        }
        else {
                    pstats.s_str = 11;
                    pstats.s_wisdom = 11;
                    pstats.s_const = 18;
                quest_item = EMORI_CLOAK;
        }
        when C_RANGER:
        case C_PALADIN:
                pstats.s_str = 11;
                pstats.s_dext = 16;
                pstats.s_const = 16;
                pstats.s_charisma = 13;
            /* intelligence or wisdom */
            if (player.t_ctype == C_RANGER) {
                    pstats.s_intel = 11;
                    pstats.s_wisdom = 7;
                quest_item = BRIAN_MANDOLIN;
            }
            else {
                    pstats.s_intel = 7;
                    pstats.s_wisdom = 11;
                quest_item = HEIL_ANKH;
            }
        when C_THIEF:
        case C_ASSASSIN:
                pstats.s_intel = 7;
                pstats.s_str = 14;
                pstats.s_wisdom = 7;
                pstats.s_dext = 18;
                pstats.s_const = 17;
                pstats.s_charisma = 11;
            if (player.t_ctype == C_THIEF) 
                quest_item = MUSTY_DAGGER;
            else
                quest_item = EYE_VECNA;
        when C_MAGICIAN:
        case C_CLERIC:
        case C_DRUID:
                pstats.s_str = 10;
                pstats.s_dext = 16;
                pstats.s_const = 15;
                pstats.s_charisma = 12;
            /* intelligence & wisdom */
            if (player.t_ctype == C_MAGICIAN) {
                    pstats.s_intel = 14;
                    pstats.s_wisdom = 7;
            }
            else {
                    pstats.s_intel = 7;
                    pstats.s_wisdom = 14;
            }
            if (player.t_ctype == C_MAGICIAN) 
                quest_item = STONEBONES_AMULET;
            else if (player.t_ctype == C_CLERIC) 
                quest_item = GERYON_HORN;
            else
                quest_item = QUILL_NAGROM;
    }
        /* Intialize */
        pstats.s_exp = 0L;
        pstats.s_lvl = 1;
        pstats.s_lvladj = 0;
        pstats.s_exp = 0L;
        strcpy(pstats.s_dmg,"2d4");
        pstats.s_carry = totalenc(&player);
        check_level();
        wmove(hw,0,0);
        wclrtoeol(hw);
        draw(hw);
        mpos = 0;

        /* Get the hit points. */
        pstats.s_hpt = 12 + const_bonus();  /* Base plus bonus */

        /* Add in the component that varies according to class */
        pstats.s_hpt += char_class[player.t_ctype].hit_pts;

        /* dole out some armor */
        if (player.t_ctype == C_THIEF || player.t_ctype == C_ASSASSIN)
        j = STUDDED_LEATHER;
        else if (player.t_ctype == C_FIGHTER || player.t_ctype == C_RANGER ||
             player.t_ctype == C_PALADIN) {
         switch (rnd(4)) {
             case 0:         j = PLATE_ARMOR;
             when 1:         j = PLATE_MAIL;
             when 2: case 3: j = BANDED_MAIL;
        }
    }
        else if (player.t_ctype == C_MONK) {
        if (rnd(3) == 0) j = MM_PROTECT;
        else j = MM_BRACERS;
        armor_item = spec_item(MM, j, rnd(125)/60+3, 0);
        obj = OBJPTR(armor_item);
            obj->o_weight = things[TYP_MM].mi_wght;
        whatis (armor_item);  /* identify it */
            obj->o_flags |= ISKNOW;
            add_pack(armor_item, TRUE);
        goto p_armorjmp;
    }
    else {  /* other characters */
        switch (rnd(7)) {
        case 0:         j = PLATE_MAIL;
        when 1: case 2: j = BANDED_MAIL;
        when 3: case 4: j = SPLINT_MAIL;
        when 5: case 6: j = PADDED_ARMOR;
        }
    }
        armor_item = spec_item(ARMOR, j, rnd(100)/85, 0);
        obj = OBJPTR(armor_item);
        obj->o_weight = armors[j].a_wght;
        obj->o_flags |= ISKNOW;
        add_pack(armor_item, TRUE);

    p_armorjmp:  /* monk doesn't wear armor */

        /* give him a weapon */
        if (player.t_ctype == C_THIEF || player.t_ctype == C_ASSASSIN ||
            player.t_ctype == C_MONK) {
        switch (rnd(5)) {
        case 0:         j = BASWORD;
        when 1: case 2: j = TRIDENT;
        when 3: case 4: j = BARDICHE;
        }
    }
        else if (player.t_ctype == C_FIGHTER || player.t_ctype == C_RANGER ||
        player.t_ctype == C_PALADIN) {
        switch (rnd(5)) {
        case 0:         j= TWOSWORD;
        when 1: case 2: j= TRIDENT;
        when 3: case 4: j= SWORD;
        }
    }
        else {
        switch (rnd(7)) {
        case 0:         j = TRIDENT;
        when 1: case 2: j = SWORD;
        when 3: case 4: j = BARDICHE;
        when 5:         j = MACE;
        when 6:         j = SPETUM;
        }
    }
        weap_item = spec_item(WEAPON, j, rnd(155)/75, rnd(165)/80);
        obj = OBJPTR(weap_item);
        obj->o_weight = weaps[j].w_wght;
        obj->o_flags |= ISKNOW;
        add_pack(weap_item, TRUE);

        /* food rations */
        food_item = spec_item(FOOD, E_RATION, 0, 0);
        obj = OBJPTR(food_item);
        obj->o_weight = foods[TYP_FOOD].mi_wght;
        obj->o_flags |= ISKNOW;
        add_pack(food_item, TRUE);

    /* give him some fruit - coose from those w/o special effects */
    switch (rnd(6)) {
        case 0: j = E_BANANA;
        when 1: j = E_BLUEBERRY;
        when 2: j = E_ELDERBERRY;
        when 3: j = E_GUANABANA;
        when 4: j = E_CAPRIFIG;
        when 5: j = E_GOOSEBERRY;
    }
        food_item = spec_item(FOOD, j, 0, 0);
        obj = OBJPTR(food_item);
        obj->o_weight = foods[TYP_FOOD].mi_wght;
        obj->o_flags |= ISKNOW;
        add_pack(food_item, TRUE);

    /* adjust purse */
    purse = 2000;
    }
    else {  /* select attibutes */
        switch(player.t_ctype) {
            case C_FIGHTER:     round = A_STRENGTH;
            when C_RANGER:      round = A_CHARISMA;
            when C_PALADIN:     round = A_CHARISMA;
            when C_MAGICIAN:    round = A_INTELLIGENCE;
            when C_CLERIC:      round = A_WISDOM;
            when C_THIEF:       round = A_DEXTERITY;
            when C_ASSASSIN:    round = A_DEXTERITY;
            when C_DRUID:       round = A_WISDOM;
            when C_MONK:        round = A_CONSTITUTION;
        }

        do {
            wclear(hw);

            /* If there is any news, display it */
            if (rogue_news) {
                register int i;

                /* Print a separator line */
                wmove(hw, 12, 0);
                for (i=0; i<cols; i++) waddch(hw, '-');

                /* Print the news */
                mvwaddstr(hw, 14, 0, rogue_news);
            }

            stat_total = MAXSTATS;
            do_escape = FALSE;  /* No escape seen yet */

            /* Initialize abilities */
            pstats.s_intel = 0;
            pstats.s_str = 0;
            pstats.s_wisdom = 0;
            pstats.s_dext = 0;
            pstats.s_const = 0;
            pstats.s_charisma = 0;

            /* Initialize pointer into abilities */
            our_stats[A_INTELLIGENCE] = &pstats.s_intel;
            our_stats[A_STRENGTH] = &pstats.s_str;
            our_stats[A_WISDOM] = &pstats.s_wisdom;
            our_stats[A_DEXTERITY] = &pstats.s_dext;
            our_stats[A_CONSTITUTION] = &pstats.s_const;

            /* Let player distribute attributes */
            for (i=0; i<NUMABILITIES-1; i++) {
                wmove(hw, 2, 0);
                wprintw(hw, "You are creating a %s with %2d attribute points.",
                                char_class[player.t_ctype].name, stat_total);

                /*
                 * Player must have a minimum of 7 in any attribute and 11 in
                 * the player's primary attribute.
                 */
                minimum = (round == i ? 11 : 7);

                /* Subtract out remaining minimums */
                maximum = stat_total - (7 * (NUMABILITIES-1 - i));

                /* Subtract out remainder of profession minimum (11 - 7) */
                if (round > i) maximum -= 4;

                /* Maximum can't be greater than 18 */
                if (maximum > 18) maximum = 18;

                wmove(hw, 4, 0);
                wprintw(hw,
                   "Minimum: %2d;  Maximum: %2d  (%s corrects previous entry)",
                   minimum, maximum, unctrl('\b'));

                wmove(hw, 6, 0);
                wprintw(hw, "    Int: %-2d", pstats.s_intel);
                wprintw(hw, "    Str: %-2d", pstats.s_str);
                wprintw(hw, "    Wis: %-2d", pstats.s_wisdom); 
                wprintw(hw, "    Dex: %-2d", pstats.s_dext);
                wprintw(hw, "    Con: %-2d", pstats.s_const);
                wprintw(hw, "    Cha: %-2d", pstats.s_charisma);
                wclrtoeol(hw);
                wmove(hw, 6, 11*i + 9);
                if (do_escape == FALSE) draw(hw);

                /* Get player's input */
                if (do_escape || maximum == minimum) {
                    *our_stats[i] = maximum;
                    stat_total -= maximum;
                }
                else for (;;) {
                    ch = wgetch(hw);
                    if (ch == '\b') {   /* Backspace */
                        if (i == 0) continue;   /* Can't move back */
                        else {
                            stat_total += *our_stats[i-1];
                            *our_stats[i] = 0;
                            *our_stats[i-1] = 0;
                            i -= 2;     /* Back out */
                            break;
                        }
                    }
                    if (ch == '\033') { /* Escape */
                        /*
                         * Escape will result in using all maximums for
                         * remaining abilities.
                         */
                        do_escape = TRUE;
                        *our_stats[i] = maximum;
                        stat_total -= maximum;
                        break;
                    }

                    /* Do we have a legal digit? */
                    if (ch >= '0' && ch <= '9') {
                        ch -= '0';      /* Convert it to a number */
                        *our_stats[i] = 10 * *our_stats[i] + ch;

                        /* Is the number in range? */
                        if (*our_stats[i] >= minimum &&
                            *our_stats[i] <= maximum) {
                            stat_total -= *our_stats[i];
                            break;
                        }

                        /*
                         * If it's too small, get more - 1x is the only
                         * allowable case.
                         */
                        if (*our_stats[i] < minimum && *our_stats[i] == 1) {
                            /* Print the player's one */
                            waddch(hw, '1');
                            draw(hw);
                            continue;
                        }
                    }

                    /* Error condition */
                    putchar('\007');
                    *our_stats[i] = 0;
                    i--;        /* Rewind */
                    break;
                }
            }

            /* Discard extra points over 18 */
            if (stat_total > 18) stat_total = 18;

            /* Charisma gets what's left */
            pstats.s_charisma = stat_total;

            /* Intialize constants */
            pstats.s_lvl = 1;
            pstats.s_lvladj = 0;
            pstats.s_exp = 0L;
            strcpy(pstats.s_dmg,"2d4");
            pstats.s_carry = totalenc(&player);

            /* Get the hit points. */
            pstats.s_hpt = 12 + const_bonus();  /* Base plus bonus */

            /* Add in the component that varies according to class */
            pstats.s_hpt += char_class[player.t_ctype].hit_pts;

            /* Display the character */
            wmove(hw, 2, 0);
            wprintw(hw,"You are creating a %s.",
                        char_class[player.t_ctype].name);
            wclrtoeol(hw);

            /* Get rid of max/min line */
            wmove(hw, 4, 0);
            wclrtoeol(hw);

            wmove(hw, 6, 0);
            wprintw(hw, "    Int: %2d", pstats.s_intel);
            wprintw(hw, "    Str: %2d", pstats.s_str);
            wprintw(hw, "    Wis: %2d", pstats.s_wisdom); 
            wprintw(hw, "    Dex: %2d", pstats.s_dext);
            wprintw(hw, "    Con: %2d", pstats.s_const);
            wprintw(hw, "    Cha: %2d", pstats.s_charisma);
            wclrtoeol(hw);

            wmove(hw, 8, 0);
            wprintw(hw, "    Hp: %2d", pstats.s_hpt);
            wclrtoeol(hw);

            wmove(hw, 10, 0);
            wprintw(hw, "    Gold: %ld", purse);

            mvwaddstr(hw, 0, 0, "Is this character okay? ");
            draw(hw);
        } while(wgetch(hw) != 'y');
      }
    }
    pstats.s_arm = 10;
    max_stats = pstats;
    /* Set up initial movement rate */
    player.t_action = A_NIL;
    player.t_movement = 6;
    player.t_no_move = 0;
    player.t_using = NULL;
    wclear(hw);
}

/*
 * init_stones:
 *      Initialize the ring stone setting scheme for this time
 */

init_stones()
{
    register int i;
    register char *str;

    for (i = 0 ; i < MAXRINGS ; i++)
    {
        do
            str = stones[rnd(NSTONES)].w_string;
        until (isupper(*str));
        *str = tolower(*str);
        r_stones[i] = str;
        r_know[i] = FALSE;
        r_guess[i] = NULL;
        if (i > 0)
                r_magic[i].mi_prob += r_magic[i-1].mi_prob;
    }
    badcheck("rings", r_magic, MAXRINGS);
}

/*
 * init_things
 *      Initialize the probabilities for types of things
 */

init_things()
{
    register struct magic_item *mp;

    for (mp = &things[1] ; mp < &things[NUMTHINGS] ; mp++)
        mp->mi_prob += (mp-1)->mi_prob;
    badcheck("things", things, NUMTHINGS);
}

