/*
    misc.c - routines dealing specifically with miscellaneous magic
    
    XRogue: Expeditions into the Dungeons of Doom
    Copyright (C) 1991 Robert Pietkivitch
    All rights reserved.
    
    Based on "Advanced Rogue"
    Copyright (C) 1984, 1985 Michael Morgan, Ken Dalka and AT&T
    All rights reserved.

    See the file LICENSE.TXT for full copyright and licensing information.
*/

#include <curses.h>
#include <ctype.h>
#include "rogue.h"

/*
 * changeclass:
 *      Change the player's class to the specified one.
 */

changeclass(newclass)
long *newclass;
{
    if (*newclass == player.t_ctype) {
        msg("You feel more skillful.");
        raise_level();
    }
    else {
        /*
         * reset his class and then use check_level to reset hit
         * points and the right level for his exp pts
         * drop exp pts by 10%
         */
        long save;

        msg("You are transformed into a %s! ", char_class[*newclass].name);

        /*
         * if he becomes a thief or an assassin give him studded leather armor
         */
        if ((*newclass == C_THIEF || *newclass == C_ASSASSIN) &&
            cur_armor != NULL && cur_armor->o_which != STUDDED_LEATHER)
                cur_armor->o_which = STUDDED_LEATHER;
        /*
         * if he becomes a monk he can't wear any armor
     * so give him a cloak of protection
         */
        if (*newclass == C_MONK && cur_armor != NULL) {
                cur_armor->o_ac = armors[cur_armor->o_which].a_class - 
                                  cur_armor->o_ac;
                cur_armor->o_type = MM;
                cur_armor->o_which = MM_PROTECT;
                cur_armor->o_flags &= ~(ISPROT | ISKNOW);
                cur_misc[WEAR_CLOAK] = cur_armor;
                cur_armor = NULL;
        }
    /*
     * otherwise give him plate armor
     */
        if ((*newclass != C_THIEF ||
         *newclass != C_ASSASSIN || *newclass != C_MONK) &&
         cur_armor != NULL && cur_armor->o_which != PLATE_ARMOR) 
                cur_armor->o_which = PLATE_ARMOR;

        /*
         * if he used to be a spell caster of some sort, kill the fuse
         */
        if (player.t_ctype == C_MAGICIAN || player.t_ctype == C_RANGER)
                extinguish(spell_recovery);
        if (player.t_ctype == C_DRUID || player.t_ctype == C_MONK)
                extinguish(chant_recovery);
        if ((player.t_ctype == C_CLERIC || player.t_ctype == C_PALADIN) &&
             !cur_relic[HEIL_ANKH])
                extinguish(prayer_recovery);

        /*
         * if he becomes a spell caster of some kind, give him a fuse
         */
        if (*newclass == C_MAGICIAN || *newclass == C_RANGER)
                fuse(spell_recovery, (VOID *)NULL, SPELLTIME, AFTER);
        if (*newclass == C_DRUID || *newclass == C_MONK)
                fuse(chant_recovery, (VOID *)NULL, SPELLTIME, AFTER);
        if ((*newclass==C_CLERIC || *newclass==C_PALADIN) && !cur_misc[HEIL_ANKH])
                fuse(prayer_recovery, (VOID *)NULL, SPELLTIME, AFTER);
        /*
         * if he's changing from a fighter, ranger, or paladin then we
     * may have to change his sword since only these types can wield
     * the two-handed sword.
         */
        if ((player.t_ctype == C_FIGHTER ||
        player.t_ctype == C_RANGER   ||
            player.t_ctype == C_PALADIN) &&
        cur_weapon != NULL && cur_weapon->o_type == WEAPON &&
            (cur_weapon->o_which == BASWORD  ||
        cur_weapon->o_which == TWOSWORD) &&
            !(*newclass == C_FIGHTER || *newclass == C_RANGER ||
            *newclass == C_PALADIN)  &&
        cur_weapon->o_which == TWOSWORD)
                cur_weapon->o_which = SWORD;

        /*
         * if he's changing from a thief, assassin, fighter, or monk
     * then we may have to change his sword again since only these
     * types can wield the bastard sword.
         */
        if ((player.t_ctype == C_THIEF  || player.t_ctype == C_ASSASSIN ||
            player.t_ctype == C_FIGHTER || player.t_ctype == C_MONK)    &&
        cur_weapon != NULL && cur_weapon->o_type == WEAPON &&
            (cur_weapon->o_which == BASWORD  ||
            cur_weapon->o_which == TWOSWORD) &&
            !(*newclass == C_THIEF || *newclass == C_ASSASSIN ||
            *newclass == C_MONK)   &&
        cur_weapon->o_which == BASWORD)
                cur_weapon->o_which = SWORD;

        /*
         * if he was a thief, assassin, or monk then take out
     * the trap_look() daemon
         */
        if (player.t_ctype == C_THIEF || player.t_ctype == C_MONK ||
            player.t_ctype == C_ASSASSIN)
                kill_daemon(trap_look);

        /*
         * if he becomes a thief, assassin, or monk then add 
     * the trap_look() daemon
         */
        if (*newclass == C_THIEF || *newclass == C_ASSASSIN ||
        *newclass == C_MONK)
                daemon(trap_look, (VOID *)NULL, AFTER);

    /* adjust stats */
        char_type = player.t_ctype = *newclass;
        save = pstats.s_hpt;
        max_stats.s_hpt = pstats.s_hpt = 0;
        max_stats.s_lvl = pstats.s_lvl = 0; 
        max_stats.s_lvladj = pstats.s_lvladj = 0; 
        max_stats.s_exp = pstats.s_exp + rnd(4);
        check_level();
        if (pstats.s_hpt > save) /* don't add to current hits */
            pstats.s_hpt = save;
    }
    dsrpt_player(); /* this should disrupt whatever we were doing */
}

/*
 * Use the relic that our monster is wielding.
 */

m_use_relic(monster)
register struct thing *monster;
{
    register struct object *obj;

    /* Make sure we really have it */
    if (monster->t_using) obj = OBJPTR(monster->t_using);
    else {
        debug("Relic not set!");
        monster->t_action = A_NIL;
        return;
    }

    /* Now let's see what we're using */
    if (obj->o_type == RELIC) switch (obj->o_which) {
        case MING_STAFF: {
            static struct object missile = {
              MISSILE, {0,0}, 0, "", "0d4 " , NULL, 0, WS_MISSILE, 100, 1
            };

            debug("Firing Ming's staff");
            sprintf(missile.o_hurldmg, "%dd4", monster->t_stats.s_lvl);
            do_motion(&missile,
                       monster->t_newpos.y, monster->t_newpos.x, monster);
            hit_monster(unc(missile.o_pos), &missile, monster);
            monster->t_artifact = monster->t_artifact * 4 / 5;
        }
        when EMORI_CLOAK:
            debug("stunning with Emori's cloak");
            do_zap(monster, obj, &monster->t_newpos, WS_PARALYZE, NULL);
            obj->o_charges = 0;

        when ASMO_ROD: {
            char *name;

            switch (rnd(3)) { /* Select a function */
                case 0:    name = "lightning bolt";
                when 1:    name = "flame";
                otherwise: name = "ice";
            }
            shoot_bolt( monster, 
                        monster->t_pos, 
                        monster->t_newpos, 
                        FALSE, 
                        monster->t_index, 
                        name, 
                        roll(monster->t_stats.s_lvl,6));
            monster->t_artifact /= 2;
        }
        when BRIAN_MANDOLIN:
            /* Make sure the defendant is still around */
            if (DISTANCE(monster->t_pos.y, monster->t_pos.x,
                         hero.y, hero.x) < 25) {
                if (!save(VS_MAGIC, &player, -4) &&
                    !ISWEARING(R_ALERT)) {
                    msg("Some beautiful music enthralls you.");
                    player.t_no_move += movement(&player) * FREEZETIME;
                    player.t_action = A_FREEZE;
                    monster->t_artifact = monster->t_artifact * 2 / 3;
                }
                else {
                    msg("You wince at a sour note.");
                    monster->t_artifact /= 3;
                }
            }
        when GERYON_HORN:
            /* Make sure the defendant is still around */
            if (DISTANCE(monster->t_pos.y, monster->t_pos.x,
                         hero.y, hero.x) < 25) {
                if (!ISWEARING(R_HEROISM) &&
                    !save(VS_MAGIC, &player, -4)) {
                        turn_on(player, ISFLEE);
                        player.t_dest = &monster->t_pos;
                        msg("A shrill blast terrifies you.");
                        monster->t_artifact = monster->t_artifact * 3 / 4;
                }
                else  {
                    msg("A shrill blast sends chills up your spine! ");
                    monster->t_artifact /= 3;
                }
            }

        otherwise:
            /* Unknown RELIC! */
            debug("Unknown wielded relic %d", obj->o_which);
    }
    else debug("Declared relic is %d", obj->o_type);

    turn_off(*monster, CANSURPRISE);
    /* Reset the monsters actions */
    monster->t_action = A_NIL;
    monster->t_using = NULL;
}
 
/*
 * add something to the contents of something else
 */

put_contents(bag, item)
register struct object *bag;            /* the holder of the items */
register struct linked_list *item;      /* the item to put inside  */
{
    register struct linked_list *titem;
    register struct object *tobj;

    bag->o_ac++;
    tobj = OBJPTR(item);
    for (titem = bag->contents; titem != NULL; titem = next(titem)) {
        if ((OBJPTR(titem))->o_which == tobj->o_which)
            break;
    }
    if (titem == NULL) {        /* if not a duplicate put at beginning */
        attach(bag->contents, item);
    }
    else {
        item->l_prev = titem;
        item->l_next = titem->l_next;
        if (next(titem) != NULL) 
            (titem->l_next)->l_prev = item;
        titem->l_next = item;
    }
}

/*
 * remove something from something else
 */

take_contents(bag, item)
register struct object *bag;            /* the holder of the items */
register struct linked_list *item;
{

    if (bag->o_ac <= 0) {
        msg("Nothing to take out");
        return;
    }
    bag->o_ac--;
    detach(bag->contents, item);
    if (!add_pack(item, FALSE))
        put_contents(bag, item);
}


do_bag(item)
register struct linked_list *item;
{

    register struct linked_list *titem = NULL;
    register struct object *obj, *tobj;
    bool doit = TRUE;

    obj = OBJPTR(item);
    while (doit) {
        msg("What do you want to do? (* for a list): ");
        mpos = 0;
        switch (wgetch(cw)) {
            case EOF:
            case ESC:
                msg ("");
                doit = FALSE;
            when '1':
                inventory(obj->contents, ALL);

            when '2':
                if (obj->o_ac >= MAXCONTENTS) {
                    msg("the %s is full", m_magic[obj->o_which].mi_name);
                    break;
                }
                switch (obj->o_which) {
                case MM_BEAKER:
                    titem = get_item(pack, "put in", POTION, FALSE, FALSE);
                when MM_BOOK:
                    titem = get_item(pack, "put in", SCROLL, FALSE, FALSE);
                }
                if (titem == NULL)
                    break;
                detach(pack, titem);
                inpack--;
                put_contents(obj, titem);
            
            when '3':
                titem = get_item(obj->contents,"take out",ALL,FALSE,FALSE);
                if (titem == NULL)
                    break;
                take_contents(obj, titem);
                
            when '4': 
                switch (obj->o_which) {
                case MM_BEAKER:
                    titem = get_item(obj->contents,"quaff",ALL,FALSE,FALSE);
                    if (titem == NULL)
                        break;
                    tobj = OBJPTR(titem);
                    obj->o_ac--;
                    detach(obj->contents, titem);
                    quaff(tobj->o_which, 
                          tobj->o_kind,
                          tobj->o_flags,
                          TRUE);
                    if (p_know[tobj->o_which] && p_guess[tobj->o_which])
                    {
                        free(p_guess[tobj->o_which]);
                        p_guess[tobj->o_which] = NULL;
                    }
                    else if (!p_know[tobj->o_which]             && 
                             askme                              &&
                             (tobj->o_flags & ISKNOW) == 0      &&
                             (tobj->o_flags & ISPOST) == 0      &&
                             p_guess[tobj->o_which] == NULL) {
                        nameitem(titem, FALSE);
                    }
                    o_discard(titem);
                when MM_BOOK:   
                    if (on(player, ISBLIND)) {
                        msg("You can't see to read anything! ");
                        break;
                    }
                    titem = get_item(obj->contents,"read",ALL,FALSE,FALSE);
                    if (titem == NULL)
                        break;
                    tobj = OBJPTR(titem);
                    obj->o_ac--;
                    detach(obj->contents, titem);
                    read_scroll(tobj->o_which, 
                                tobj->o_flags & (ISCURSED|ISBLESSED),
                                TRUE);
                    if (s_know[tobj->o_which] && s_guess[tobj->o_which])
                    {
                        free(s_guess[tobj->o_which]);
                        s_guess[tobj->o_which] = NULL;
                    }
                    else if (!s_know[tobj->o_which]             && 
                             askme                              &&
                             (tobj->o_flags & ISKNOW) == 0      &&
                             (tobj->o_flags & ISPOST) == 0      &&
                             s_guess[tobj->o_which] == NULL) {
                        nameitem(titem, FALSE);
                    }
                    o_discard(titem);
                }
                doit = FALSE;

            otherwise:
                wclear(hw);
                touchwin(hw);
                mvwaddstr(hw,0,0,"The following operations are available:");
                mvwaddstr(hw,2,0,"[1]\tInventory\n");
                wprintw(hw,"[2]\tPut something in the %s\n",
                        m_magic[obj->o_which].mi_name);
                wprintw(hw,"[3]\tTake something out of the %s\n",
                        m_magic[obj->o_which].mi_name);
                switch(obj->o_which) {
                    case MM_BEAKER: waddstr(hw,"[4]\tQuaff a potion\n");
                    when MM_BOOK:   waddstr(hw,"[4]\tRead a scroll\n");
                }
        /* this is confusing! <press space to continue> */
                /* waddstr(hw,"[ESC]\tLeave this menu\n"); */
                mvwaddstr(hw, lines-1, 0, spacemsg);
                draw(hw);
                wait_for (' ');
                restscr(cw);
        }
    }
}

do_panic(who)
int who;        /* Kind of monster to panic (all if who is NULL) */
{
    register int x,y;
    register struct linked_list *mon, *item;
    register struct thing *th;

    for (x = hero.x-2; x <= hero.x+2; x++) {
        for (y = hero.y-2; y <= hero.y+2; y++) {
            if (y < 1 || x < 0 || y > lines - 3  || x > cols - 1) 
                continue;
            if (isalpha(mvwinch(mw, y, x))) {

                if ((mon = find_mons(y, x)) != NULL) {
                    th = THINGPTR(mon);

                    /* Is this the right kind of monster to panic? */
                    if (who && th->t_index != who) continue;

                    if ((who && th->t_stats.s_intel < 14) || 
                        (!on(*th, ISUNDEAD) && !save(VS_MAGIC, th, 0) &&
              off(*th, WASTURNED))) {
                          msg("%s %s.", prname(monster_name(th), TRUE),
                            terse ? "panics" : "turns to run in panic");

                        turn_on(*th, ISFLEE);
                        turn_on(*th, WASTURNED);
                        turn_off(*th, CANSURPRISE);

                        /* Disrupt what it was doing */
                        dsrpt_monster(th, TRUE, TRUE);

                        /* If monster was suffocating, stop it */
                        if (on(*th, DIDSUFFOCATE)) {
                            turn_off(*th, DIDSUFFOCATE);
                            extinguish(suffocate);
                        }

                        /* If monster held us, stop it */
                        if (on(*th, DIDHOLD) && (--hold_count == 0))
                                turn_off(player, ISHELD);
                        turn_off(*th, DIDHOLD);

                        /*
                         * if he has something he might drop it
                         */
                        if ((item = th->t_pack) != NULL         && 
                            (OBJPTR(item))->o_type != RELIC     && 
                            rnd(100) < 67) {
                                detach(th->t_pack, item);
                                fall(item, FALSE);
                        }

                        /* It is okay to turn tail */
                        th->t_oldpos = th->t_pos;
                    }
                    runto(th, &hero);
                }
            }
        }
    }
}

/*
 * print miscellaneous magic bonuses
 */

int
misc_name(str,obj)
char *str;
register struct object *obj;
{
    char buf1[LINELEN];

    *str = 0;
    buf1[0] = 0;

    if (!(obj->o_flags & ISKNOW))
    {
        strcat(str,m_magic[obj->o_which].mi_name);
        return(0);
    }

    switch (obj->o_which) 
    {
        case MM_BRACERS:
        case MM_PROTECT:
            strcat(str, num(obj->o_ac, 0));
            strcat(str, " ");
    }
    switch (obj->o_which) {
    case MM_CRYSTAL:
            if (obj->o_flags & ISBLESSED)
                strcat(str, "glowing ");
    }
    switch (obj->o_which) {
        case MM_G_OGRE:
        case MM_G_DEXTERITY:
        case MM_JEWEL:
        case MM_STRANGLE:
        case MM_R_POWERLESS:
        case MM_DANCE:
            if (obj->o_flags & ISCURSED)
                strcat(str, "cursed ");
        when MM_CRYSTAL:
            if (obj->o_flags & ISCURSED)
                strcat(str, "opaque ");
    }
    strcat(str, m_magic[obj->o_which].mi_name);

    switch (obj->o_which) 
    {
        case MM_JUG:
            if (obj->o_ac == JUG_EMPTY)
                strcat(buf1, " [empty]");
            else if (p_know[obj->o_ac])
                sprintf(buf1, " [containing a potion of %s (%s)]",
                        p_magic[obj->o_ac].mi_name,
                        p_colors[obj->o_ac]);
            else sprintf(buf1, " [containing a%s %s liquid]", 
                        vowelstr(p_colors[obj->o_ac]),
                        p_colors[obj->o_ac]);
        when MM_BEAKER:         
        case MM_BOOK: {
            sprintf(buf1, " [containing %d]", obj->o_ac);
        }
        when MM_OPEN:
        case MM_HUNGER:
            sprintf(buf1, " [%d ring%s]", obj->o_charges, 
                          obj->o_charges == 1 ? "" : "s");
        when MM_DRUMS:
            sprintf(buf1, " [%d beat%s]", obj->o_charges, 
                          obj->o_charges == 1 ? "" : "s");
        when MM_DISAPPEAR:
        case MM_CHOKE:
            sprintf(buf1, " [%d pinch%s]", obj->o_charges, 
                          obj->o_charges == 1 ? "" : "es");
        when MM_KEOGHTOM:
            sprintf(buf1, " [%d application%s]", obj->o_charges, 
                          obj->o_charges == 1 ? "" : "s");
        when MM_SKILLS:
            sprintf(buf1, " [%s]", char_class[obj->o_ac].name);
    }
    strcat(str, buf1);
    
    return(0);
}

use_emori()
{
    char selection;     /* Cloak function */
    int state = 0;      /* Menu state */

    msg("What do you want to do? (* for a list): ");
    do {
        selection = wgetch(cw);
        switch (selection) {
            case '*':
              if (state != 1) {
                wclear(hw);
                touchwin(hw);
                mvwaddstr(hw, 2, 0,  "[1] Fly\n[2] Stop flying\n");
                waddstr(hw,          "[3] Turn invisible\n[4] Turn Visible\n");
                mvwaddstr(hw, 0, 0, "What do you want to do? ");
                draw(hw);
                state = 1;      /* Now in prompt window */
              }
              break;

            case ESC:
                if (state == 1) {
                    restscr(cw);
                }
                msg("");

                after = FALSE;
                return;

            when '1':
            case '2':
            case '3':
            case '4':
                if (state == 1) {       /* In prompt window */
                    restscr(cw);
                }

                msg("");

                state = 2;      /* Finished */
                break;

            default:
                if (state == 1) {       /* In the prompt window */
                    mvwaddstr(hw, 0, 0,
                                "Please enter a selection between 1 and 4:  ");
                    draw(hw);
                }
                else {  /* Normal window */
                    mpos = 0;
                    msg("Please enter a selection between 1 and 4:  ");
                }
        }
    } while (state != 2);

    /* We now must have a selection between 1 and 4 */
    switch (selection) {
        case '1':       /* Fly */
            if (on(player, ISFLY)) {
                extinguish(land);       /* Extinguish in case of potion */
                msg("%slready flying.", terse ? "A" : "You are a");
            }
            else {
                msg("You feel lighter than air!");
                turn_on(player, ISFLY);
            }
        when '2':       /* Stop flying */
            if (off(player, ISFLY))
                msg("%sot flying.", terse ? "N" : "You are n");
            else {
                if (find_slot(land))
                    msg("%sot flying by the cloak.",
                        terse ? "N" : "You are n");
                else land();
            }
        when '3':       /* Turn invisible */
            if (off(player, ISINVIS)) {
                turn_on(player, ISINVIS);
                msg("You have a tingling feeling all over your body. ");
                PLAYER = IPLAYER;
                light(&hero);
            }
            else {
                extinguish(appear);     /* Extinguish in case of potion */
                extinguish(dust_appear);/* dust of disappearance        */
                msg("%slready invisible.", terse ? "A" : "You are a");
            }
        when '4':       /* Turn visible */
            if (off(player, ISINVIS))
                msg("%sot invisible.", terse ? "N" : "You are n");
            else {
                if (find_slot(appear) || find_slot(dust_appear))
                    msg("%sot invisible by the cloak.",
                        terse ? "N" : "You are n");
                else appear();
            }
    }
}

/*
 * try to write a scroll with the quill of Nagrom
 */

use_quill(obj)
struct object *obj;
{
    struct linked_list  *item;
    register int        i,
                        scroll_ability;
    int                 which_scroll,
                        curlen,
                        maxlen = 0,
                        dummy = 0;
    bool                nohw = FALSE;

    i = which_scroll = 0;
    scroll_ability = obj->o_charges;

    /* Prompt for scrolls */
    msg("Which scroll are you writing? (* for list): ");

    which_scroll = (int) (wgetch(cw) - 'a');
    msg("");    /* Get rid of the prompt */
    if (which_scroll == (int) ESC - (int) 'a') {
        after = FALSE;
        return;
    }
    if (which_scroll >= 0 && which_scroll < MAXQUILL) nohw = TRUE;

    else if (slow_invent) {
        register char c;

        nohw = TRUE;
        do {
            for (i=0; i<MAXQUILL; i++) {
                msg("");
                mvwaddch(msgw, 0, 0, '[');
                waddch(msgw, (char) ((int) 'a' + i));
                waddstr(msgw, "] A scroll of ");
                waddstr(msgw, s_magic[quill_scrolls[i].s_which].mi_name);
                waddstr(msgw, morestr);
                clearok(msgw, FALSE);
                draw(msgw);
                do {
                    c = wgetch(cw);
                } while (c != ' ' && c != ESC);
                if (c == ESC)
                    break;
            }
            msg("");
            mvwaddstr(msgw, 0, 0, "Which scroll are you writing? ");
            clearok(msgw, FALSE);
            draw(msgw);

            which_scroll = (int) (wgetch(cw) - 'a');
        } while (which_scroll != (int) (ESC - 'a') &&
                 (which_scroll < 0 || which_scroll >= MAXQUILL));

        if (which_scroll == (int) (ESC - 'a')) {
            mpos = 0;
            msg("");
            after = FALSE;
            return;
        }
    }
    else {
        /* Now display the possible scrolls */
        wclear(hw);
        touchwin(hw);
        mvwaddstr(hw, 2, 0, "   Cost            Scroll");
        mvwaddstr(hw, 3, 0,
                "-----------------------------------------------");
        maxlen = 47;    /* Maximum width of header */

        for (i=0; i<MAXQUILL; i++) {
            wmove(hw, i+4, 0);
            sprintf(prbuf, "[%c]        %3d     A scroll of %s",
                    (char) ((int) 'a' + i),
                    quill_scrolls[i].s_cost,
                    s_magic[quill_scrolls[i].s_which].mi_name);
            waddstr(hw, prbuf);

            /* Get the length of the line */
            getyx(hw, dummy, curlen);
            if (maxlen < curlen) maxlen = curlen;
        }

        sprintf(prbuf, "[Current scroll power = %d]", scroll_ability);
        mvwaddstr(hw, 0, 0, prbuf);
        waddstr(hw, " Which scroll are you writing? ");
        getyx(hw, dummy, curlen);
        if (maxlen < curlen) maxlen = curlen;

        /* Should we overlay? */
        if (menu_overlay && MAXQUILL + 3 < lines - 3) {
            over_win(cw, hw, MAXQUILL + 5, maxlen + 3, 0, curlen, NULL);
        }
        else draw(hw);
    }

    if (!nohw) {
        which_scroll = (int) (wgetch(cw) - 'a');
        while (which_scroll < 0 || which_scroll >= MAXQUILL) {
            if (which_scroll == (int) ESC - (int) 'a') {
                after = FALSE;

                /* Restore the screen */
                if (MAXQUILL + 3 < lines / 2) {
                    clearok(cw, FALSE);
                    touchwin(cw);
                }
                else restscr(cw);
                return;
            }
            wmove(hw, 0, 0);
            wclrtoeol(hw);
            waddstr(hw, "Please enter one of the listed scrolls. ");
            getyx(hw, dummy, curlen);
            if (maxlen < curlen) maxlen = curlen;

            /* Should we overlay? */
            if (menu_overlay && MAXQUILL + 3 < lines - 3) {
                over_win(cw, hw, MAXQUILL + 5, maxlen + 3,
                            0, curlen, NULL);
            }
            else draw(hw);

            which_scroll = (int) (wgetch(cw) - 'a');
        }
    }

    /* Now restore the screen if we have to */
    if (!nohw) {
        if (MAXQUILL + 3 < lines / 2) {
            touchwin(cw);
            clearok(cw, FALSE);
        }
        else restscr(cw);
    }

    /* We've waited our required time. */
    player.t_using = NULL;
    player.t_action = A_NIL;

    if (quill_scrolls[which_scroll].s_cost > scroll_ability) {
        msg("Your attempt fails.");
        return;
    }

    obj->o_charges -= quill_scrolls[which_scroll].s_cost;
    item = spec_item(SCROLL, quill_scrolls[which_scroll].s_which, 0, 0);
    if (add_pack(item, FALSE) == FALSE) {
        (OBJPTR(item))->o_pos = hero;
        fall(item, TRUE);
    }

    which_scroll = dummy; /* Hack to stop IRIX complaint about dummy not */
                          /* being used */
}

/*
 * Use something
 */

use_mm(which)
int which;
{
    register struct object *obj = NULL;
    register struct linked_list *item = NULL;
    bool is_mm;

    is_mm = FALSE;

    if (which < 0) {    /* A real miscellaneous magic item  */
        /* This is miscellaneous magic.  It takes 3 movement periods to use */
        if (player.t_action != C_USE) {
            int units;  /* Number of movement units for the item */

            item = get_item(pack, "use", USEABLE, FALSE, FALSE);

            /*
             * Make certain that it is a micellaneous magic item
             */
            if (item == NULL)
                return;

            units = usage_time(item);
            if (units < 0) return;

            player.t_using = item;      /* Remember what it is */
            player.t_action = C_USE;    /* We are quaffing */
            player.t_no_move = units * movement(&player);
            return;
        }

        /* We have waited our time, let's use the item */
        item = player.t_using;
        player.t_using = NULL;
        player.t_action = A_NIL;

        is_mm = TRUE;

        obj = OBJPTR(item);
        which = obj->o_which;
    }

    if (obj->o_type == POTION) {                /* A potion */
        is_mm = FALSE;
        inpack--;
        detach (pack, item);
        switch (obj->o_which) {
            case P_POISON:
                if (cur_weapon) {
                    if (cur_weapon->o_type == RELIC) {
                        msg("The poison burns off %s", 
                            inv_name(cur_weapon,FALSE));
                    }
                    else {
                        cur_weapon->o_flags |= ISPOISON;
                        msg("Your weapon has %s gooey stuff on it",
                            p_colors[cur_weapon->o_which]);
                    }
                }
                else 
                    msg("The poison pours on the floor and disappears!");
        }
        o_discard(item);
    }
    else if (obj->o_type == RELIC) {            /* An artifact */
        is_mm = FALSE;
        switch (obj->o_which) {
            case EMORI_CLOAK:
                use_emori();
            when QUILL_NAGROM:
                use_quill(obj);
            when BRIAN_MANDOLIN:
                /* Put monsters around us to sleep */
                read_scroll(S_HOLD, 0, FALSE);
            when GERYON_HORN:
                /* Chase close monsters away */
                msg("The horn blasts a shrill tone.");
                do_panic(NULL);
            when EYE_VECNA:
                msg("The pain slowly subsides.. ");
            when HEIL_ANKH:
                msg("Your hand grows very warm. ");
            when YENDOR_AMULET:
                msg("Your chest glows! ");
                do_panic(findmindex("frost giant"));
            when STONEBONES_AMULET:
                msg("Your chest glows! ");
                do_panic(findmindex("storm giant"));
            when SURTUR_RING:
                do_panic(findmindex("fire giant"));
            when ALTERAN_CARD:  /* the card allows you to teleport anywhere */
        do_teleport();
        }
    }
    else switch (which) {               /* Miscellaneous Magic */
        /*
         * the jug of alchemy manufactures potions when you drink
         * the potion it will make another after a while
         */
        case MM_JUG:
            if (obj->o_ac == JUG_EMPTY) {
                msg("The jug is empty");
                break;
            }
            quaff (obj->o_ac, NULL, NULL, FALSE);
            obj->o_ac = JUG_EMPTY;
            fuse (alchemy, obj, ALCHEMYTIME, AFTER);
            if (!(obj->o_flags & ISKNOW))
                whatis(item);

        /*
         * the beaker of plentiful potions is used to hold potions
         * the book of infinite spells is used to hold scrolls
         */
        when MM_BEAKER:
        case MM_BOOK:
            do_bag(item);

        /*
         * the chime of opening opens up secret doors
         */
        when MM_OPEN:
        {
            register struct linked_list *exit;
            register struct room *rp;
            register coord *cp;

            if (obj->o_charges <= 0) {
                msg("The chime is cracked!");
                break;
            }
            obj->o_charges--;
            msg("chime... chime... hime... ime... me... e...");
            if ((rp = roomin(&hero)) == NULL) {
                search(FALSE, TRUE); /* Non-failing search for door */
                break;
            }
            for (exit = rp->r_exit; exit != NULL; exit = next(exit)) {
                cp = DOORPTR(exit);
                if (winat(cp->y, cp->x) == SECRETDOOR) {
                    mvaddch (cp->y, cp->x, DOOR);
                    if (cansee (cp->y, cp->x))
                        mvwaddch(cw, cp->y, cp->x, DOOR);
                }
            }
        }

        /*
         * the chime of hunger just makes the hero hungry
         */
        when MM_HUNGER:
            if (obj->o_charges <= 0) {
                msg("The chime is spent. ");
                break;
            }
            obj->o_charges--;
            if (food_left >= MORETIME + 5) {
                food_left = MORETIME + 5;
                msg("A strange sensation comes over you.. ");
                msg(terse? "Getting hungry" : "You are starting to get hungry");
                hungry_state = F_HUNGRY;
            }
            if (player.t_ctype == C_PALADIN ||
                player.t_ctype == C_RANGER  || player.t_ctype == C_MONK) {
                    msg("You feel a chilling sensation!");
                    aggravate(TRUE, FALSE);
            }
            else {
                aggravate(TRUE, TRUE);
            }

        /*
         * the drums of panic make all creatures within two squares run
         * from the hero in panic unless they save or they are mindless
         * undead
         */
        when MM_DRUMS:
            if (obj->o_charges <= 0) {
                msg("The drum is broken.");
                break;
            }
            obj->o_charges--;
            do_panic(NULL);
        return;
        /*
         * dust of disappearance makes the player invisible for a while
         */
        when MM_DISAPPEAR:
            m_know[MM_DISAPPEAR] = TRUE;
            if (obj->o_charges <= 0) {
                msg("No more dust!");
                break;
            }
            obj->o_charges--;
            if (terse) msg("You sneeze! ");
            else msg("Ahh.. Ahh... Choo!! ");
            if (!find_slot(dust_appear)) {
                turn_on(player, ISINVIS);
                fuse(dust_appear, (VOID *)NULL, DUSTTIME, AFTER);
                PLAYER = IPLAYER;
                light(&hero);
            }
            else lengthen(dust_appear, DUSTTIME);

        /*
         * dust of choking and sneezing can kill the hero if he misses
         * the save
         */
        when MM_CHOKE:
            m_know[MM_CHOKE] = TRUE;
            if (obj->o_charges <= 0) {
                msg("No more dust!");
                break;
            }
            obj->o_charges--;
            if (terse) msg("You snort! ");
            else msg("aaAAACHOOOooo. Cough. Cough. Sneeze. Sneeze.");
            if (!cur_relic[SURTUR_RING] && !save(VS_POISON, &player, 0)) {
                msg ("You choke to death!!!  --More--");
                wait_for(' ');
                pstats.s_hpt = -1;  /* in case he hangs up the phone! */
                death(D_CHOKE);
            }
            else {
                msg("You begin to cough and choke uncontrollably! ");
                if (find_slot(unchoke))
                    lengthen(unchoke, DUSTTIME);
                else
                    fuse(unchoke, (VOID *)NULL, DUSTTIME, AFTER);
                turn_on(player, ISHUH);
                turn_on(player, ISBLIND);
                light(&hero);
            }
                
        when MM_KEOGHTOM:
            /*
             * this is a very powerful healing ointment
             * but it takes a while to put on...
             */
            obj->o_charges--;
            if (on(player, HASDISEASE)) {
                extinguish(cure_disease);
                cure_disease();
                msg(terse ? "You feel yourself improving."
                          : "You begin to feel yourself improving again.");
            }
            if (on(player, HASINFEST)) {
                turn_off(player, HASINFEST);
                infest_dam = 0;
                msg(terse ? "You feel yourself improving."
                          : "You begin to feel yourself improving again.");
            }
            if (on(player, DOROT)) {
                msg("You feel your skin returning to normal.");
                turn_off(player, DOROT);
            }
            pstats.s_hpt += roll(pstats.s_lvl, 6);
            if (pstats.s_hpt > max_stats.s_hpt)
                pstats.s_hpt = max_stats.s_hpt;
            sight();
            msg("You begin to feel much better.");
                
        /*
         * The book has a character class associated with it.
         * if your class matches that of the book, it will raise your 
         * level by one. If your class does not match the one of the book, 
         * it change your class to that of book.
         * Note that it takes a while to read.
         */
        when MM_SKILLS:
            detach (pack, item);
            inpack--;
            changeclass(&obj->o_ac);
    when MM_CRYSTAL:
    {
        register char *str;

            detach (pack, item);
            inpack--;
            if (obj->o_flags & ISCURSED) {
            if (is_mm && !m_know[MM_CRYSTAL])
                str = "rock in a curious sort of way";
        else
            str = "crystal briefly";
        msg("You rub the %s and yell out in agony! ", str);
          /* curse his pack */
        read_scroll(S_REMOVE, obj->o_flags & ISCURSED, FALSE);
          /* aggravate monsters */
        read_scroll(S_HOLD, obj->o_flags & ISCURSED, FALSE);
                player.t_no_move += (2 * movement(&player) * FREEZETIME);
                player.t_action = A_FREEZE;
          /* loss of 1/4 total hit points */
        pstats.s_hpt -= ((max_stats.s_hpt / 4));
                max_stats.s_hpt -= rnd(3)+3;
        if (pstats.s_hpt > max_stats.s_hpt) 
                pstats.s_hpt = max_stats.s_hpt;
                if ((pstats.s_hpt < 1) || (max_stats.s_hpt < 1)) {
            pstats.s_hpt = -1;
            msg("The crystal has absorbed you...  --More--");
            wait_for(' ');
                    death(D_CRYSTAL);
        }
        }
        else {  /* if normal, give him a bonus */
            if (is_mm && !m_know[MM_CRYSTAL])
                    str = "flashes brightly";
        else
            str = "vibrates softly";
        msg("You rub the crystal and it %s...  ", str);
          /* cure him */
        read_scroll(S_CURING, NULL, FALSE);
          /* give him weird hands */
                turn_on(player, CANHUH);
        msg("Your fingertips turn blue.  ");
          /* add intelligence */
                if (player.t_ctype == C_MAGICIAN) {
                max_stats.s_intel += 1;
                pstats.s_intel += 1;
        }
          /* add strength */
                if (player.t_ctype == C_FIGHTER) {
            max_stats.s_str += 1;
                pstats.s_str += 1;
        }
          /* add wisdom */
                if (player.t_ctype == C_CLERIC || player.t_ctype == C_PALADIN) {
                    max_stats.s_wisdom += 1;
                    pstats.s_wisdom += 1;
        }
          /* add dexterity */
                if (player.t_ctype == C_THIEF || player.t_ctype == C_ASSASSIN) {
                    max_stats.s_dext += 1;
                    pstats.s_dext += 1;
        }
              /* add constitution */
                if (player.t_ctype == C_MONK) {
                    max_stats.s_const += 1;
                    pstats.s_const += 1;
        }
          /* add charisma */
                if (player.t_ctype == C_RANGER || player.t_ctype == C_PALADIN) {
                max_stats.s_charisma += 1;
                pstats.s_charisma += 1;
        }
        }
            if (obj->o_flags & ISBLESSED) {  /* if blessed */
            if (is_mm && !m_know[MM_CRYSTAL])
            msg("The crystal disappears from your hands.  ");
        else
            msg("Your hands absorb the medicine crystal.  ");
            /* set hit points to at least 50 */
        if (max_stats.s_hpt < 50) {
            max_stats.s_hpt = 50;
                    pstats.s_hpt = max_stats.s_hpt;
        }
        else {  /* or just add 10% */
            max_stats.s_hpt += (max_stats.s_hpt / 10);
                    pstats.s_hpt = max_stats.s_hpt;
        }
            /* heck, really make it memorable */
        read_scroll(S_REMOVE, obj->o_flags & ISBLESSED, FALSE);
        }
    }
        otherwise:
            msg("What a strange magic item you have!");
    }
    status(FALSE);
    if (is_mm && m_know[which] && m_guess[which]) {
        free(m_guess[which]);
        m_guess[which] = NULL;
    }
    else if (is_mm && !m_know[which] && askme &&
             (obj->o_flags & ISKNOW) == 0 &&
             m_guess[which] == NULL) {
        nameitem(item, FALSE);
    }
    if (item != NULL && (which == MM_SKILLS || which == MM_CRYSTAL))
        o_discard(item);
    updpack(TRUE, &player);
}

/*
 * usage_time:
 *      Return how long it takes to use an item.  For now we only give time
 *      for MM, RELIC, SCROLL, and POTION items.
 */

int
usage_time(item)
struct linked_list *item;
{
    register struct object *obj;
    register int units = -1;

    obj = OBJPTR(item);
    switch (obj->o_type) {
        case SCROLL:    units = 4;
        when POTION:    units = 3;
        when RELIC:                     /* An artifact */
            switch (obj->o_which) {
                case BRIAN_MANDOLIN:
                case GERYON_HORN:       units = 4;
                when QUILL_NAGROM:
                case EMORI_CLOAK:
                case HEIL_ANKH:         units = 3;
                when YENDOR_AMULET:
                case STONEBONES_AMULET: units = 2;
                when EYE_VECNA:         units = 6;
                    /* The eye will do nothing other than give a headache */
                    pstats.s_hpt -= rnd(25)+1;
                    msg("You feel a sharp pain shoot through your forehead!");
                    if (pstats.s_hpt < 1) {
               pstats.s_hpt = -1;
                       msg ("The pain is too much for you to bear!  --More--");
                       wait_for(' ');
                       death(D_RELIC);
                    }
                when SURTUR_RING:
                    units = 3;
                    msg("Your nose tickles a bit.");
        when ALTERAN_CARD:
            units = 2;
            msg("You gaze intently at the card... ");
            }
        when MM:
            switch (obj->o_which) {     /* Miscellaneous Magic */
                case MM_JUG:
                    if (obj->o_ac == JUG_EMPTY) {
                        msg("The jug is empty");
                        return (-1);
                    }
                    units = 2;
                when MM_BEAKER:
                case MM_BOOK:
                    /* This is a strange case because it can go forever */
                    units = 1;
                case MM_CHOKE: /* Dust */
                when MM_HUNGER: /* Chimes */
                    units = 3;
                when MM_OPEN:
                case MM_DRUMS:
                case MM_DISAPPEAR:
                    units = 4;
                when MM_KEOGHTOM:
            /* Ointment */
                    if (obj->o_charges <= 0) {
                        msg("The jar is empty!");
                        return (-1);
                    }
                    units = 5;
                when MM_SKILLS:
                    /* A whole book! */
                    units = 15;
                when MM_CRYSTAL:
            /* Enhance player's quest */
            units = 5;
                otherwise:
                    /* What is it? */
                    units = -1;
            }
        otherwise:      units = -1;
    }

    return (units);
}

