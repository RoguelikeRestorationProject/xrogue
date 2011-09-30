/*
    fight.c - All the fighting gets done here
    
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
#include <string.h>
#include "rogue.h"

#define CONF_DAMAGE     -1
#define PARAL_DAMAGE    -2
#define DEST_DAMAGE     -3
#define DRAIN_DAMAGE    -4

int killed_chance = 0;  /* cumulative chance for goodies to loose it */

/*
 * returns true if player has a any chance to hit the monster
 */

player_can_hit(tp, weap)
register struct thing *tp;
register struct object *weap;
{
    if (off(*tp, CMAGICHIT) && off(*tp, BMAGICHIT) && off(*tp, MAGICHIT))
        return(TRUE);
    if (weap && weap->o_type == RELIC)
        return(TRUE);
    if (on(*tp, CMAGICHIT) && weap && (weap->o_hplus>2 || weap->o_dplus>2))
        return(TRUE);
    if (on(*tp, BMAGICHIT) && weap && (weap->o_hplus>1 || weap->o_dplus>1))
        return(TRUE);
    if (on(*tp,  MAGICHIT) && weap && (weap->o_hplus>0 || weap->o_dplus>0))
        return(TRUE);
    if (player.t_ctype == C_MONK) {
        if (on(*tp, CMAGICHIT) && pstats.s_lvl > 15)
            return(TRUE);
        if (on(*tp, BMAGICHIT) && pstats.s_lvl > 10)
            return(TRUE);
        if (on(*tp,  MAGICHIT) && pstats.s_lvl > 5)
            return(TRUE);
    }
    return(FALSE);
}

/*
 * fight:
 *      The player attacks the monster.
 */

fight(mp, weap, thrown)
register coord *mp;
struct object *weap;
bool thrown;
{
    register struct thing *tp;
    register struct linked_list *item;
    register bool did_hit = TRUE;
    bool see_def, back_stab = FALSE;
    register char *mname;

    /*
     * Find the monster we want to fight
     */
    if ((item = find_mons(mp->y, mp->x)) == NULL) {
        return(FALSE); /* must have killed him already */
    }
    tp = THINGPTR(item);

    /*
     * Since we are fighting, things are not quiet so no healing takes
     * place.  The -1 also tells us that we are in a fight.
     */
    player.t_quiet = -1;
    tp->t_quiet = -1;

    see_def = ((off(*tp, ISINVIS)     || on(player, CANSEE)) &&
               (off(*tp, ISSHADOW)    || on(player, CANSEE)) &&
               (!thrown || cansee(unc(tp->t_pos))));

    mname = see_def ? monster_name(tp) : "something";

    /*
     * if its in the wall, we can't hit it
     */
    if (on(*tp, ISINWALL) && off(player, CANINWALL))
        return(FALSE);

    if (on(*tp, ISSTONE)) {
        killed(item, FALSE, FALSE, FALSE);
        if (see_def) 
            msg("%s shatters into a million pieces!", prname(mname, TRUE));
        count = 0;
        return (TRUE);
    }
    /*
     * Let him know it was really a mimic (if it was one).
     */
    if (on(*tp, ISDISGUISE) && (tp->t_type != tp->t_disguise) &&
        off(player, ISBLIND))
    {
        if (see_def) {
            msg("Wait! That's a %s!", mname);
            turn_off(*tp, ISDISGUISE);
        }
        did_hit = thrown;
    }
    if (on(*tp, CANSURPRISE) && off(player, ISBLIND) && !ISWEARING(R_ALERT)) {
        if (see_def) {
            msg("Wait! There's a %s!", mname);
            turn_off(*tp, CANSURPRISE);
        }
        did_hit = thrown;
    }

    /*
     * if he's a thief or assassin and the creature is asleep then he gets 
     * a chance for a backstab
     */
    if ((player.t_ctype == C_THIEF || player.t_ctype == C_ASSASSIN) &&
        !thrown          &&
        !on(*tp, NOSTAB) &&
        !invisible(tp)   &&
        (!on(*tp, ISRUN) || on(*tp, ISHELD) || tp->t_action == A_FREEZE))
            back_stab = TRUE;

    /*
     * assassins get an assassination chance, if it fails then its normal
     * damage
     */
    if (back_stab && player.t_ctype == C_ASSASSIN) {
        int chance;

        chance = 50 + (pstats.s_lvl - tp->t_stats.s_lvl) * 5;
        if (cur_weapon && (cur_weapon->o_flags & ISPOISON))
            chance += 20;
        if (roll(1,100) > chance || on(*tp, ISUNIQUE))
            back_stab = FALSE;
    }

    runto(tp, &hero);

    /* Let the monster know that the player has missiles! */
    if (thrown) tp->t_wasshot = TRUE;

    if (did_hit)
    {

        did_hit = FALSE;
        if (!can_blink(tp)              && 
            player_can_hit(tp, weap)    &&
            roll_em(&player, tp, weap, thrown, cur_weapon, back_stab))
        {
            did_hit = TRUE;

            if (on(*tp, NOMETAL) && weap != NULL &&
                weap->o_type != RELIC && weap->o_flags & ISMETAL) {
                msg("Your %s passes right through %s!",
                    weaps[weap->o_which].w_name, prname(mname, FALSE));
            }
            else if (weap != NULL && weap->o_type == MISSILE && on(*tp, CARRYBAMULET)) {
                    msg("The magic missile has no effect on %s. ",
                        prname(mname, FALSE));
            }
            else {
                hit(thrown ? (struct object *)NULL : weap,
                    TRUE, see_def,
                    thrown ? weap_name(weap) : NULL,
                    mname, back_stab, thrown, terse);

                /* See if there are any special effects */
                if (effect(&player, tp, weap, thrown, TRUE, see_def) != 0)
                    killed(item, FALSE, FALSE, TRUE);
    
                /* 
                 * Merchants just disappear if hit 
                 */
                else if (on(*tp, CANSELL)) {
                    if (see_def)
                        msg("%s disappears with his wares in a flash! ",
                            prname(mname, FALSE));
                    killed(item, FALSE, FALSE, FALSE);
                }
    
                else if (tp->t_stats.s_hpt <= 0)
                    killed(item, TRUE, TRUE, TRUE);
    
                else {
                    /* If the victim was charmed, it now gets a saving throw! */
                    if (on(*tp, ISCHARMED) && save(VS_MAGIC, tp, 0)) {
                        msg("The eyes of %s turn clear.", prname(mname, FALSE));
                        turn_off(*tp, ISCHARMED);
                    }

                    dsrpt_monster(tp, FALSE, see_def); /* Disrupt a spell? */
                }
            }
        }
        else {
            miss(thrown ? (struct object *)NULL : weap,
                 TRUE, see_def,
                 thrown ? weap_name(weap) : (char *)NULL,
                 mname, thrown, terse);
        }
    }
    count = 0;
    return did_hit;
}

/*
 * attack:
 *      The monster attacks the player
 */

attack(mp, weapon, thrown)
register struct thing *mp;
register struct object *weapon;
bool thrown;
{
    register char *mname;
    register bool see_att, did_hit = FALSE;
    register struct object *wielded;    /* The wielded weapon */
    struct linked_list *get_wield;      /* Linked list header for wielded */

    /*
     * Since this is an attack, stop running and any healing that was
     * going on at the time.  The -1 also tells us that we're fighting.
     */
    running = FALSE;
    player.t_quiet = -1;
    mp->t_quiet = -1;

    if (on(*mp, ISDISGUISE) && off(player, ISBLIND))
        turn_off(*mp, ISDISGUISE);

    see_att = ((off(*mp, ISINVIS)     || on(player, CANSEE)) &&
               (off(*mp, ISSHADOW)    || on(player, CANSEE)) &&
               (!thrown || cansee(unc(mp->t_pos))));

    mname = see_att ? monster_name(mp) : "something";

    /*
     * Try to find a weapon to wield.  Wield_weap will return a
     * projector if weapon is a projectile (eg. bow for arrow).
     * If weapon is NULL, it will try to find a suitable weapon.
     */
    get_wield = wield_weap(weapon, mp);
    if (get_wield) wielded = OBJPTR(get_wield);
    else wielded = NULL;

    /* If we aren't wielding a weapon, wield what we found (could be NULL) */
    if (weapon == NULL) weapon = wielded;

    if (roll_em(mp, &player, weapon, thrown, wielded, FALSE)) {
        int death_type; /* From one of the effects of getting hit */

        did_hit = TRUE;

        if (weapon != NULL && weapon->o_type == MISSILE && cur_relic[STONEBONES_AMULET]) {
            hit(weapon, see_att, TRUE, mname, (char *)NULL, FALSE, thrown, terse);
            msg("Your amulet absorbs the magic missile. ");
        }
        else {
            hit(weapon, see_att, TRUE, mname, (char *)NULL, FALSE, thrown, terse);
            dsrpt_player();     /* see if we disrupted some activity */
            if (pstats.s_hpt <= 0)
                death(mp->t_index);     /* Bye bye life ... */
            death_type = effect(mp, &player, weapon, thrown, see_att, TRUE);
            if (death_type != 0) {
        pstats.s_hpt = -1;
        death(death_type);
        }
        }

    }
    else {
        /* If the thing was trying to surprise, no good */
        if (on(*mp, CANSURPRISE)) turn_off(*mp, CANSURPRISE);

        /* If it couldn't surprise, let's tell the player. */
        else miss(weapon, see_att, TRUE, mname, (char *)NULL, thrown, terse);
    }
    if (fight_flush) flushinp();
    count = 0;
    status(FALSE);
    return(did_hit);
}

/*
 * swing:
 *      returns true if the swing hits
 */

swing(class, at_lvl, op_arm, wplus)
short class;
int at_lvl, op_arm, wplus;
{
    register int res = rnd(20)+1;
    register int need;

    need = char_class[class].base -
           char_class[class].factor *
           ((min(at_lvl, char_class[class].max_lvl) -
            char_class[class].offset)/char_class[class].range) +
           (10 - op_arm);
    if (need > 20 && need <= 25) need = 20;

    return (res+wplus >= need);
}

/*
 * roll_em:
 *      Roll several attacks
 */

roll_em(att_er, def_er, weap, hurl, cur_weapon, back_stab)
struct thing *att_er, *def_er;
struct object *weap;
bool hurl;
struct object *cur_weapon;
bool back_stab;
{
    register struct stats *att, *def;
    register char *cp = NULL;
    register int ndice, nsides, nplus, def_arm;
    char dmgbuf[20];
    bool did_hit = FALSE;
    int prop_hplus, prop_dplus;
    int vampiric_damage;

    /* Get statistics */
    att = &att_er->t_stats;
    def = &def_er->t_stats;

    prop_hplus = prop_dplus = 0;
    if (weap == NULL) {
        /*
         * monks damage grows with level
         */
        if (att == &pstats && player.t_ctype == C_MONK) {
            sprintf(dmgbuf, "%dd4", att->s_lvl/3+2);
            cp = dmgbuf;
        }
        else
            cp = att->s_dmg;
    }
    else if (weap->o_type == RELIC) {
        switch (weap->o_which) {
            case MUSTY_DAGGER:
            if (player.t_ctype == C_THIEF) 
            cp = "4d8+2/4d8+2";
        else
                cp = "4d8/4d8";
            when YEENOGHU_FLAIL:
            cp = "4d8+3/paralyze/confuse";
            when HRUGGEK_MSTAR:
            cp = "4d8+3";
            when AXE_AKLAD:
            if (player.t_ctype == C_FIGHTER) {
            if (hurl)
            cp = "4d8+6/drain";
                    else
            cp = "4d8+4/drain";
        }
        else {
            if (hurl)
            cp = "4d8+4/drain";
                    else
            cp = "4d8+2/drain";
                }
            when MING_STAFF:
                cp = "4d8+4";
            when ASMO_ROD:
                cp = "4d8/4d8";
            when ORCUS_WAND:
            cp = "4d8/destroy";
        }
    }
    else if (hurl) {
        if ((weap->o_flags&ISMISL) && cur_weapon != NULL &&
          cur_weapon->o_which == weap->o_launch)
        {
            cp = weap->o_hurldmg;
            prop_hplus = cur_weapon->o_hplus;
            prop_dplus = cur_weapon->o_dplus;
        }
        else
            cp = (weap->o_flags&ISMISL ? weap->o_damage : weap->o_hurldmg);
    }
    else {
        cp = weap->o_damage;
        /*
         * Drain a staff of striking
         */
        if(weap->o_type==STICK && weap->o_which==WS_HIT && weap->o_charges==0)
        {
            strcpy(weap->o_damage,"4d8");
            weap->o_hplus = weap->o_dplus = 0;
        }
    }
    /*
     * If defender is wearing a cloak of displacement -- no damage
     * the first time. (unless its a hurled magic missile or the
     * attacker is very smart and can see thru the illusion)
     */
    if ((weap == NULL || weap->o_type != MISSILE)       &&
        def == &pstats                                  &&      
        off(*att_er, MISSEDDISP)                        &&
        att->s_intel < 21                               &&
        ((cur_misc[WEAR_CLOAK]!=NULL && 
          cur_misc[WEAR_CLOAK]->o_which==MM_DISP) ||
          cur_relic[EMORI_CLOAK])) {
        turn_on(*att_er, MISSEDDISP);
        if (cansee(att_er->t_pos.y, att_er->t_pos.x) && !invisible(att_er))
            msg("%s looks amazed! ", prname(monster_name(att_er), TRUE));
        return (FALSE);
    }
    if (on(*def_er, CARRYCLOAK)                         && 
        def != &pstats                                  && 
        (weap == NULL || weap->o_type != MISSILE)       && 
        off (*att_er, MISSEDDISP)                       &&
        pstats.s_intel < 21) {
            turn_on(*att_er, MISSEDDISP);
            msg("You feel amazed! ");
            return(FALSE);
    }
    for (;;)
    {
        int damage;
        int hplus = prop_hplus;
        int dplus = prop_dplus;

        if (weap != NULL && weap->o_type == RELIC) {
            switch (weap->o_which) {
                case MUSTY_DAGGER:
                    if (att != &pstats || /* Not player or good stats */
                        (str_compute() > 15 && dex_compute() > 15)) {

                        hplus += 6;
                        dplus += 6;

                        /* Give an additional strength and dex bonus */
                        if (att == &pstats) {
                            hplus += str_plus(str_compute()) +
                                     dext_plus(dex_compute());
                            dplus += dext_plus(dex_compute()) +
                                     add_dam(str_compute());
                        }
                        else {
                            hplus += str_plus(att->s_str) +
                                     dext_plus(att->s_dext);
                            dplus += dext_plus(att->s_dext) +
                                     add_dam(att->s_str);
                        }
                    }
                    else {
                        hplus -= 3;
                        dplus -= 3;
                    }
                when YEENOGHU_FLAIL:
                case HRUGGEK_MSTAR:
                    hplus += 3;
                    dplus += 3;
                when MING_STAFF:
                    hplus += 2;
                    dplus += 2;
                when AXE_AKLAD:
                    hplus += 5;
                    dplus += 5;
            }
        }
        else if (weap != NULL) {
            hplus += weap->o_hplus;
            dplus += weap->o_dplus;
        }

        /* Is attacker weak? */
        if (on(*att_er, HASSTINK)) hplus -= 2;

        if (att == &pstats)     /* Is the attacker the player? */
        {
            hplus += hitweight();       /* adjust for encumberence */
            dplus += hung_dam();        /* adjust damage for hungry player */
            dplus += ring_value(R_ADDDAM);
        }
        if (back_stab || (weap && att != &pstats && on(*att_er, CANBSTAB)))
            hplus += 4; /* add in pluses for backstabbing */

        /* Get the damage */
        while (isspace(*cp)) cp++;
        if (!isdigit(*cp)) {
            if (strncmp(cp, "confuse", 7) == 0) ndice = CONF_DAMAGE;
            else if (strncmp(cp, "paralyze", 8) == 0) ndice = PARAL_DAMAGE;
            else if (strncmp(cp, "destroy", 6) == 0) ndice = DEST_DAMAGE;
            else if (strncmp(cp, "drain", 5) == 0) ndice = DRAIN_DAMAGE;
            else ndice = 0;
            nsides = 0;
            nplus = 0;
        }
        else {
            char *oldcp;

            /* Get the number of damage dice */
            ndice = atoi(cp);
            if ((cp = strchr(cp, 'd')) == NULL)
                break;

            /* Skip the 'd' and get the number of sides per die */
            nsides = atoi(++cp);

            /* Check for an addition -- save old place in case none is found */
            oldcp = cp;
            if ((cp = strchr(cp, '+')) != NULL) nplus = atoi(++cp);
            else {
                nplus = 0;
                cp = oldcp;
            }
        }

        if (def == &pstats) { /* Monster attacks player */
            if (on(*att_er, NOMETAL))
                def_arm = ac_compute(TRUE) - dext_prot(dex_compute());
            else
                def_arm = ac_compute(FALSE) - dext_prot(dex_compute());
            hplus += str_plus(att->s_str)+dext_plus(att->s_dext);
        }
        else if (att == &pstats) {      /* Player attacks monster */
            def_arm = def->s_arm - dext_prot(def->s_dext);
            if (player.t_ctype == C_MONK) /* no strength bonus for monk */
                if (weap == NULL) 
                    hplus += att->s_lvl/5; /* monks hplus varies with level */
            else
                hplus += str_plus(str_compute())+dext_plus(dex_compute());
        }
        else {  /* Monster attacks monster */
            def_arm = def->s_arm - dext_prot(def->s_dext);
            hplus += str_plus(att->s_str)+dext_plus(att->s_dext);
        }

        if (swing(att_er->t_ctype, att->s_lvl, def_arm, hplus)) {
            register int proll;

            /* Take care of special effects */
            switch (ndice) {
              case CONF_DAMAGE:
                if (def == &pstats) { /* Monster attacks player */
                    if (!save(VS_MAGIC, &player, 0) && off(player, ISCLEAR)) {
                        msg("You feel disoriented.");
                        if (find_slot(unconfuse))
                            lengthen(unconfuse, HUHDURATION);
                        else
                            fuse(unconfuse, (VOID *)NULL, HUHDURATION, AFTER);
                        turn_on(player, ISHUH);
                    }
                    else msg("You feel dizzy, but it quickly passes.");
                }
                /* Player or monster hits monster */
                else if (!save(VS_MAGIC, def_er, 0) && off(*def_er, ISCLEAR)) { 
                    if (att == &pstats) {
            if (rnd(10) > 6)
                            msg("The artifact warms you with pleasure! ");
            }
                    turn_on(*def_er, ISHUH);
                }
                did_hit = TRUE;
              when PARAL_DAMAGE:
                if (def == &pstats) { /* Monster attacks player */
                    if (!save(VS_MAGIC, &player, 0) && off(player, CANINWALL)) {
                        msg("You stiffen up.");
                        player.t_no_move += movement(&player) * FREEZETIME;
                        player.t_action = A_FREEZE;
                    }
                }
                else if (!save(VS_MAGIC, def_er, 0)) { /* Player hits monster */
                    if (att == &pstats) {
            if (rnd(10) > 6)
                msg("The artifact hums happily! ");
            }
                    turn_off(*def_er, ISRUN);
                    turn_on(*def_er, ISHELD);
                }
                did_hit = TRUE;
              when DEST_DAMAGE:
                if (def == &pstats) {   /* Monster attacks player */
            if (rnd(10) > 5)
                        msg("You feel a tug at your life force.");
                    if (!save(VS_MAGIC, &player, -4)) {
                        msg("The wand devours your soul!  --More--");
            wait_for(' ');
                        def->s_hpt = -1;
            death(D_RELIC);
                    }
                }
                /* Player hits monster */
                else if (!save(VS_MAGIC, def_er, -4)) {
                    if (att == &pstats) {
            if (rnd(10) > 4)
                            msg("The artifact draws some energy.");
                     }
                    /* The player loses some major hit pts  */
                    att->s_hpt -= (att->s_hpt/5)+1;
            if (att->s_hpt <= 0) {
            msg("The wand has devoured your soul!  --More--");
            wait_for(' ');
            att->s_hpt = -1;
            death(D_RELIC);
            }
                    /* Kill the monster */
                    def->s_hpt = 0;
                }
                did_hit = TRUE;
            when DRAIN_DAMAGE:
                if (def == &pstats) {       /* Monster attacks player */
                    if (!save(VS_MAGIC, &player, -4)) {
                        lower_level(att_er->t_index);
                    }
                }
                /* Player hits monster */
                else if (!save(VS_MAGIC, def_er, -4)) {
                    def->s_hpt -= roll(1, 8);
                    def->s_lvl--;
                    if (def->s_lvl <= 0)
                        def->s_hpt = 0;     /* he's dead */
                    if (att == &pstats) {
            if (rnd(10) > 7)
                        msg("The artifact cackles with laughter! ");
            }
                }
                did_hit = TRUE;
              otherwise:
                /* Heil's ankh always gives maximum damage */
                if (att == &pstats && cur_relic[HEIL_ANKH])
                    proll = ndice * nsides;
                else proll = roll(ndice, nsides);

                if (ndice + nsides > 0 && proll < 1)
                    debug("Damage for %dd%d came out %d.",
                                ndice, nsides, proll);
                damage = dplus + proll + nplus;
                if (att == &pstats) {
                    /*
                     * Monks do not get strength bonus on damage.  Instead,
                     * if they are wielding a weapon, they get at extra
                     * 1/2 point per level of damage.
                     */
                    if(player.t_ctype == C_MONK) {
                        /* Bonus does not apply for hands. */
                        if (weap != NULL) damage += att->s_lvl / 2;
                    }
                    else
                        damage += add_dam(str_compute());
                }
                else
                    damage += add_dam(att->s_str);

                /* Check for half damage monsters */
                if (on(*def_er, HALFDAMAGE)) damage /= 2;

                /* add in multipliers for backstabbing */
                if (back_stab || 
                    (weap && att != &pstats && on(*att_er, CANBSTAB))) {
                    int mult = 2 + (att->s_lvl-1)/4; /* Normal multiplier */

                    if (mult > 5)
                        mult = 5;
                    if (weap->o_type == RELIC && weap->o_which == MUSTY_DAGGER)
                        mult++;
                    damage *= mult;
                }
                if (att == &pstats) {
                    if (cur_weapon && (cur_weapon->o_flags & ISPOISON)) {
                        cur_weapon->o_flags &= ~ISPOISON;
                        if (save(VS_POISON, def_er, -2))
                            damage += def->s_hpt/4;
                        else
                            damage += def->s_hpt/2;
                    }
                    if (back_stab && player.t_ctype == C_ASSASSIN)
                        damage = def->s_hpt + 1;
                }
                /* Check for no-damage and division */
                if (on(*def_er, BLOWDIVIDE)) {
                    damage = 0;
                    creat_mons(def_er, def_er->t_index, FALSE);
                    if (cansee(unc(def_er->t_pos))) light(&hero);
                }
                /* check for immunity to metal -- RELICS are always bad */
                if (on(*def_er, NOMETAL) && weap != NULL &&
                    weap->o_type != RELIC && weap->o_flags & ISMETAL) {
                    damage = 0;
                }
                if (weap != NULL && weap->o_type == MISSILE) {
                    if ((def == &pstats && cur_relic[STONEBONES_AMULET]) ||
                        (att == &pstats && on(*def_er, CARRYBAMULET))) {
                        damage = 0;
                    }
                }
                def->s_hpt -= max(0, damage);   /* Do the damage */
                did_hit = TRUE;
                vampiric_damage = damage;
                if (def->s_hpt < 0)     /* only want REAL damage inflicted */
                    vampiric_damage += def->s_hpt;
                if (vampiric_damage < 0)
                    vampiric_damage = 0;
                if (att == &pstats && ISWEARING(R_VAMPREGEN) && !hurl) {
                    if ((pstats.s_hpt += vampiric_damage/2) > max_stats.s_hpt)
                        pstats.s_hpt = max_stats.s_hpt;
                }
        if (hplus < 0) hplus = 0;
        if (damage < 0) damage = 0;
                debug ("hplus=%d dmg=%d", hplus, damage);
            }
        }
        if ((cp = strchr(cp, '/')) == NULL)
            break;
        cp++;
    }
    return did_hit;
}

/*
 * prname:
 *      The print name of a combatant
 */

char *
prname(who, upper)
register char *who;
bool upper;
{
    static char tbuf[LINELEN];

    *tbuf = '\0';
    if (who == 0)
        strcpy(tbuf, "you"); 
    else if (on(player, ISBLIND) || strcmp(who, "something") == 0)
        strcpy(tbuf, "something");
    else
    {
        /* If we have a name (starts with a capital), don't use a "the" */
        if (islower(*who)) strcpy(tbuf, "the ");
        strcat(tbuf, who);
    }
    if (upper)
        *tbuf = toupper(*tbuf);
    return tbuf;
}

/*
 * hit:
 *      Print a message to indicate a succesful hit
 */

hit(weapon, see_att, see_def, er, ee, back_stab, thrown, short_msg)
register struct object *weapon;
bool see_att, see_def;
register char *er, *ee;
bool back_stab, thrown, short_msg;
{
    register char *s = NULL;
    char          att_name[LINELEN],    /* Name of attacker */
                  def_name[LINELEN]; /* Name of defender */

    /* If we can't see either the attacker or defender, don't say anything */
    if (!see_att && !see_def) return;

    /* What do we call the attacker? */
    strcpy(att_name, see_att ? prname(er, TRUE) : "Something");
    if (er) {   /* A monster is attacking */

        /* If the monster is using a weapon and we can see it, report it */
        if (weapon != NULL && (see_att || thrown)) {
            strcat(att_name, "'s ");
            strcat(att_name, weap_name(weapon));
        }
    }

    /* What do we call the defender? */
    strcpy(def_name, see_def ? prname(ee, FALSE) : "something");

    addmsg(att_name);
    if (short_msg) {
        if (back_stab) {
            if (player.t_ctype == C_ASSASSIN)
                s = (er == 0 ? " assassinate!" : " assassinates!");
            else
                s = (er == 0 ? " backstab!" : " backstabs!");
        }
        else
            s = " hit.";
    }
    else {
        if (back_stab) {
            if (player.t_ctype == C_ASSASSIN)
                s = (er == 0 ? " have assassinated " : " has assassinated ");
            else
                s = (er == 0 ? " have backstabbed " : " has backstabbed ");
        }
        else {
            switch (rnd(thrown ? 2 : 3))
            {
                case 0: s = " hit ";
                when 1: s = " injured ";
                when 2: s = " smacked ";
            }
        }
    }
    if (short_msg) addmsg(s);
    else addmsg("%s%s.", s, def_name);
    endmsg();
}

/*
 * miss:
 *      Print a message to indicate a poor swing
 */

miss(weapon, see_att, see_def, er, ee, thrown, short_msg)
register struct object *weapon;
bool see_att, see_def;
register char *er, *ee;
bool thrown, short_msg;
{
    register char *s = NULL;
    char          att_name[LINELEN],    /* Name of attacker */
                  def_name[LINELEN];    /* Name of defender */

    /* If we can't see either the attacker or defender, don't say anything */
    if (!see_att && !see_def) return;

    /* What do we call the attacker? */
    strcpy(att_name, see_att ? prname(er, TRUE) : "Something");
    if (er) {   /* A monster is attacking */

        /* If the monster is using a weapon and we can see it, report it */
        if (weapon != NULL && (see_att || thrown)) {
            strcat(att_name, "'s ");
            strcat(att_name, weap_name(weapon));
        }
    }

    /* What do we call the defender? */
    strcpy(def_name, see_def ? prname(ee, FALSE) : "something");

    addmsg(att_name);
    switch (short_msg ? 0 : rnd(thrown ? 3 : 2))
    {
        case 0: s = (er == 0 ? " miss" : " misses");
        when 1: s = (er == 0 ? " don't hit" : " doesn't hit");
        when 2: s = (" whizzes by");
    }
    if (short_msg) addmsg("%s.", s);
    else addmsg("%s %s.", s, def_name);
    endmsg();
}

/*
 * dext_plus:
 *      compute to-hit bonus for dexterity
 */

dext_plus(dexterity)
register int dexterity;
{
        return (dexterity > 10 ? (dexterity-13)/3 : (dexterity-10)/3);
}


/*
 * dext_prot:
 *      compute armor class bonus for dexterity
 */

dext_prot(dexterity)
register int dexterity;
{
    return ((dexterity-10)/2);
}

/*
 * str_plus:
 *      compute bonus/penalties for strength on the "to hit" roll
 */

str_plus(str)
register short str;
{
    return((str-10)/3);
}

/*
 * add_dam:
 *      compute additional damage done for exceptionally high or low strength
 */

add_dam(str)
register short str;
{
    return((str-9)/2);
}

/*
 * hung_dam:
 *      Calculate damage depending on players hungry state
 */

hung_dam()
{
        reg int howmuch = 0;

        switch(hungry_state) {
                case F_SATIATED:
                case F_OKAY:
                case F_HUNGRY:  howmuch = 0;
                when F_WEAK:    howmuch = -1;
                when F_FAINT:   howmuch = -2;
        }
        return howmuch;
}

/*
 * is_magic:
 *      Returns true if an object radiates magic
 */

is_magic(obj)
register struct object *obj;
{
    switch (obj->o_type)
    {
        case ARMOR:
            return obj->o_ac != armors[obj->o_which].a_class;
        when WEAPON:
            return obj->o_hplus != 0 || obj->o_dplus != 0;
        when POTION:
        case SCROLL:
        case STICK:
        case RING:
        case MM:
        case RELIC:
            return TRUE;
    }
    return FALSE;
}

/*
 * killed:
 *      Called to put a monster to death
 */

killed(item, pr, points, treasure)
register struct linked_list *item;
bool pr, points, treasure;
{
    register struct thing *tp, *mp;
    register struct linked_list *pitem, *nexti, *mitem;
    char *monst;
    int adj;    /* used for hit point adj. below. */
    long temp;

    tp = THINGPTR(item);

    if (pr)
    {
        addmsg(terse ? "Defeated " : "You have defeated ");
        if (on(player, ISBLIND))
            msg("it.");
        else
        {
            if (cansee(tp->t_pos.y, tp->t_pos.x) && !invisible(tp))
                monst = monster_name(tp);
            else {
                if (terse) monst = "something";
                else monst = "thing";
            }
            if (!terse)
                addmsg("the ");
            msg("%s.", monst);
        }
    }

    /* Take care of any residual effects of the monster */
    check_residue(tp);

    /* Make sure that no one is still chasing us */
    for (mitem = mlist; mitem != NULL; mitem = next(mitem)) {
        mp = THINGPTR(mitem);
        if (mp->t_dest == &tp->t_pos) {
            mp->t_dest = &hero;
            mp->t_wasshot = FALSE;
            turn_off(*mp, ISFLEE);      /* Be sure we aren't running away! */
        }
    }
    if (points) {   /* you feel uneasy for a moment */
        if ((off(*tp, ISMEAN) || on(*tp, ISFRIENDLY)) &&
            (player.t_ctype == C_RANGER || player.t_ctype == C_PALADIN ||
             player.t_ctype == C_MONK)) {
                if (tp->t_stats.s_exp > pstats.s_exp)
                    pstats.s_exp = 0;
                else
                    pstats.s_exp -= tp->t_stats.s_exp;
                /* Take care of hit points. */
                if (level <= 12) adj = rnd(2)+1;
                else if (level <= 25) adj = rnd(3)+2;
                else if (level <= 50) adj = rnd(4)+3;
                else if (level <= 80) adj = rnd(5)+4;
                else adj = rnd(6)+5;
        /* adjust hit points */
                max_stats.s_hpt -= adj;
                pstats.s_hpt -= adj;
                /* Are hit points now too low? */
                if (pstats.s_hpt <= 0) {
            pstats.s_hpt = -1;
            death(D_STRENGTH);
        }
                killed_chance += rnd(3)+1;
                if (on(*tp, ISUNIQUE)) /* real bad news to kill a diety */
                    killed_chance += 25;
                if (roll(1,100) < killed_chance) {
            msg("You had a feeling this was going to happen... ");
                    msg("**POOF**  ");
                    temp = C_ASSASSIN;  /* make him pay */
                    changeclass(&temp);
                }
                else {
                    switch (rnd(9)) {
                    case 0:
            msg("You become solid and stiff for a while. ");
                        player.t_no_move += (5*movement(&player)*FREEZETIME);
                        player.t_action = A_FREEZE;
                    when 1:
            msg("You collapse, losing it totally. ");
                        player.t_no_move += (2*movement(&player)*FREEZETIME);
                        player.t_action = A_FREEZE;
                    when 2:
            msg("Your face changes shape!  ARGGHH!!!! ");
                        pstats.s_charisma -= rnd(8)+3;
                        if (pstats.s_charisma <= 3) pstats.s_charisma = 3;
                    when 3:
            case 4:
            msg("You cry out, I didn't mean to do that!  Honest!! ");
                        player.t_no_move += (movement(&player)*FREEZETIME);
            msg("The Great Old Ones grant you a reprieve. ");
                    otherwise: msg("You feel uneasy for a moment.. ");
                    }
        }
        }
        else {
                unsigned long test;      /* For overflow check */
                /* 
                 * Do an overflow check before increasing experience 
                 */
                test = pstats.s_exp + tp->t_stats.s_exp;
                if (test > pstats.s_exp) 
                        pstats.s_exp = test;
        }

        /*
         * Do adjustments if he went up a level
         */
        check_level();
    }

    /*
     * Empty the monsters pack
     */
    pitem = tp->t_pack;

    /*
     * Get rid of the monster.
     */
    mvwaddch(mw, tp->t_pos.y, tp->t_pos.x, ' ');
    mvwaddch(cw, tp->t_pos.y, tp->t_pos.x, tp->t_oldch);
    detach(mlist, item);
    if (on(*tp, AREMANY) && levtype == NORMLEV) /* AREMANYs stick together */
        wake_room(roomin(&tp->t_pos));
    /*
     * empty his pack
     */
    while (pitem != NULL)
    {
        nexti = next(pitem);
        (OBJPTR(pitem))->o_pos = tp->t_pos;
        detach(tp->t_pack, pitem);
        if (treasure) 
            fall(pitem, FALSE);
        else 
            o_discard(pitem);
        pitem = nexti;
    }

    turn_on(*tp,ISDEAD);
    attach(rlist,item);
}

/*
 * Returns a pointer to the weapon the monster is wielding corresponding to
 * the given thrown weapon.  If no thrown item is given, try to find any
 * decent weapon.
 */

struct linked_list *
wield_weap(thrown, mp)
struct object *thrown;
struct thing *mp;
{
    int look_for = 0,   /* The projectile weapon we are looking for */
        new_rate,       /* The rating of a prospective weapon */
        cand_rate = -1; /* Rating of current candidate -- higher is better */
    register struct linked_list *pitem, *candidate = NULL;
    register struct object *obj;

    if (thrown != NULL) {       /* Using a projectile weapon */
      switch (thrown->o_which) {
        case BOLT:      look_for = CROSSBOW;    /* Find the crossbow */
        when ARROW:     look_for = BOW;         /* Find the bow */
        when ROCK:      look_for = SLING;       /* find the sling */
        otherwise:      return(NULL);
      }
    }
    else if (off(*mp, ISUNIQUE) && off(*mp, CARRYWEAPON)) return(NULL);

    for (pitem=mp->t_pack; pitem; pitem=next(pitem)) {
        obj = OBJPTR(pitem);

        /*
         * If we have a thrown weapon, just return the first match
         * we come to.
         */
        if (thrown != NULL && obj->o_type == WEAPON && obj->o_which == look_for)
            return(pitem);

        /* If we have a usable RELIC, return it */
        if (thrown == NULL && obj->o_type == RELIC) {
            switch (obj->o_which) {
                case MUSTY_DAGGER:
                case YEENOGHU_FLAIL:
                case HRUGGEK_MSTAR:
                case AXE_AKLAD:
                case MING_STAFF:
                case ASMO_ROD:
                case ORCUS_WAND:
                    return(pitem);
            }
        }

        /* Otherwise if it's a usable weapon, it is a good candidate */
        else if (thrown == NULL && obj->o_type == WEAPON) {
            switch (obj->o_which) {
                case DAGGER:
                case SPEAR:
                    new_rate = 0;
                when BATTLEAXE:
                    new_rate = 1;
                when MACE:
                    new_rate = 2;
                when SWORD:
                    new_rate = 3;
                when PIKE:
                    new_rate = 4;
                when HALBERD:
                case SPETUM:
                    new_rate = 6;
                when BARDICHE:
                    new_rate = 7;
                when TRIDENT:
                    new_rate = 8;
                when BASWORD:
                    new_rate = 9;
                when TWOSWORD:
                    new_rate = 10;
                otherwise:
                    new_rate = -1;
            }

            /* Only switch if this is better than the current candidate */
            if (new_rate > cand_rate) {
                cand_rate = new_rate;
                candidate = pitem;
            }
        }
    }

    return(candidate);
}
explode(tp)
register struct thing *tp;
{

    register int x,y, damage;
    struct linked_list *item;
    struct thing *th;

    /*
     * check to see if it got the hero
     */
     if (off(player, ISINWALL) &&
         DISTANCE(hero.x, hero.y, tp->t_pos.x, tp->t_pos.y) <= 25) {
        msg("The explosion hits you! ");
        damage = roll(6,6);
        if (save(VS_WAND, &player, 0))
            damage /= 2;
        pstats.s_hpt -= damage;
    }

    /*
     * now check for monsters in vicinity
     */
     for (x = tp->t_pos.x-5; x<=tp->t_pos.x+5; x++) {
         if (x < 0 || x > cols - 1) 
             continue;
         for (y = tp->t_pos.y-5; y<=tp->t_pos.y+5; y++) {
            if (y < 1 || y > lines - 3)
                continue;
            if (isalpha(mvwinch(mw, y, x))) {
                if ((item = find_mons(y, x)) != NULL) {
                    th = THINGPTR(item);
                    if (th == tp || /* don't count gas spore */
                        on(*th, ISINWALL)) /* Don't count monsters in wall */
                        continue;
                    damage = roll(6, 6);
                    if (save(VS_WAND, th, 0))
                        damage /= 2;
                    runto(th, &hero);
                    if ((th->t_stats.s_hpt -= damage) <= 0) {
                        msg("The explosion kills %s! ", 
                            prname(monster_name(th), FALSE));
                        killed(item, FALSE, FALSE, TRUE);
                    }
                }
            }
        }
    }
}

/*
 * skirmish:
 *      Called when one monster attacks another monster.
 */

skirmish(attacker, mp, weap, thrown)
register struct thing *attacker;
register coord *mp;
struct object *weap;
bool thrown;
{
    register struct thing *defender;
    register struct linked_list *item;
    register bool did_hit = TRUE, see_att, see_def;
    char attname[LINELEN+1], defname[LINELEN+1];
    struct object *wielded;     /* The wielded weapon */
    struct linked_list *get_wield;      /* Linked list header for wielded */

    /*
     * Find the monster we want to fight
     */
    if ((item = find_mons(mp->y, mp->x)) == NULL) {
        return(FALSE); /* must have killed him already */
    }
    defender = THINGPTR(item);

    /* Can the player see either of the fighters? */
    see_att = (cansee(unc(attacker->t_pos)) &&
               (off(*attacker, ISINVIS)     || on(player, CANSEE)) &&
               (off(*attacker, ISSHADOW)    || on(player, CANSEE)));
    see_def = (cansee(unc(defender->t_pos)) &&
               (off(*defender, ISINVIS)     || on(player, CANSEE)) &&
               (off(*defender, ISSHADOW)    || on(player, CANSEE)));

    /*
     * Since we are fighting, things are not quiet so no healing takes
     * place.  The -1 also tells us that we are in a fight.
     */
    attacker->t_quiet = -1;
    defender->t_quiet = -1;

    if (see_att) strcpy(attname, monster_name(attacker));
    else strcpy(attname, "something");

    if (see_def) strcpy(defname, monster_name(defender));
    else strcpy(defname, "something");

    /*
     * if its in the wall, we can't hit it
     */
    if (on(*defender, ISINWALL) && off(*attacker, CANINWALL))
        return(FALSE);

    if (on(*defender, ISSTONE)) {
        killed(item, FALSE, FALSE, FALSE);
        if (see_def)
            msg("%s shatters into a million pieces!", prname(defname, TRUE));
        return (TRUE);
    }

    /*
     * Let him know it was really a mimic (if it was one).
     */
    if (see_def && on(*defender, ISDISGUISE) &&
        (defender->t_type != defender->t_disguise)) {
        msg("Wait!  There's a %s!", defname);
        turn_off(*defender, ISDISGUISE);
        did_hit = thrown;
    }

    if (see_def && on(*defender, CANSURPRISE) && !ISWEARING(R_ALERT)) {
        msg("Wait!  There's a %s!", defname);
        turn_off(*defender, CANSURPRISE);
        did_hit = thrown;
    }

    if (did_hit) {

        did_hit = FALSE;

        /*
         * Try to find a weapon to wield.  Wield_weap will return a
         * projector if weapon is a projectile (eg. bow for arrow).
         * If weapon is NULL, it will try to find a suitable weapon.
         */
        get_wield = wield_weap(weap, attacker);
        if (get_wield) wielded = OBJPTR(get_wield);
        else wielded = NULL;

#ifdef DOBLINK
        /*
         * For now Blink Dogs will not blink away from monsters.  We
         * have to fix can_blink so it isn't dependant on the player
         * before we can add it.
         */
        if (!can_blink(defender) &&
#endif
        if (((weap && weap->o_type == RELIC) ||
             ((off(*defender, MAGICHIT) ||
               attacker->t_stats.s_lvl > 4 ||
               (weap && (weap->o_hplus > 0 || weap->o_dplus > 0))) &&
              (off(*defender, BMAGICHIT) ||
               attacker->t_stats.s_lvl > 6 ||
               (weap && (weap->o_hplus > 1 || weap->o_dplus > 1))) &&
              (off(*defender, CMAGICHIT) ||
               attacker->t_stats.s_lvl > 8 ||
               (weap && (weap->o_hplus > 2 || weap->o_dplus > 2)))))
            && roll_em(attacker, defender, weap, thrown, wielded, FALSE))
        {
            did_hit = TRUE;

            /* Should we start to chase this creature? */
            if (attacker->t_index != defender->t_index  &&
                (off(*defender, ISRUN) || rnd(100) < 50)) {
                /*
                 * If we're intelligent enough to realize that this
                 * is a friendly monster, we will attack the hero instead.
                 */
                if (on(*attacker, ISFRIENDLY) &&
                    roll(3,6) < defender->t_stats.s_intel) {
                    runto(defender, &hero);
                    debug("%s attacking %s's hero", defname, attname);
                }

                /* Otherwise, let's chase the monster */
                else {
                    runto(defender, &attacker->t_pos);
                    debug("%s now attacking %s", defname, attname);
                }
            }
            else if (off(*defender, ISRUN)) runto(defender, &hero);

            /* Let the defender know that the attacker has missiles! */
            if ((defender->t_dest == &attacker->t_pos) && thrown)
                defender->t_wasshot = TRUE;

            if (on(*defender, NOMETAL) && weap != NULL &&
                weap->o_type != RELIC && weap->o_flags & ISMETAL) {
                if (see_def && see_att)
                    msg("The %s passes right through %s!",
                        weaps[weap->o_which].w_name, prname(defname, FALSE));
            }
            else {
                hit(weap, see_att, see_def,
                    attname, defname, FALSE, thrown, FALSE);
            }

            /* See if there are any special effects */
            if (effect(attacker, defender,
                       weap, thrown, see_att, see_def) != 0) {
                killed(item, FALSE, FALSE, TRUE);
                if (see_def) msg("%s dies.", prname(defname, TRUE));
                else msg("You hear a blood-curdling scream! ");
            }

            /* 
             * Merchants just disappear if hit 
             */
            else if (on(*defender, CANSELL)) {
                if (see_def)
                    msg("%s disappears with his wares in a flash! ",
                            prname(defname, TRUE));
                killed(item, FALSE, FALSE, FALSE);
            }

            else if (defender->t_stats.s_hpt <= 0) {
                killed(item, FALSE, FALSE, TRUE);
                if (see_def) msg("%s dies.", prname(defname, TRUE));
                else msg("You hear a blood-curdling scream! ");
            }

            else {
                /* Did we disrupt a spell?                              */
                /* Don't turn on WASDISRUPTED since player didn't do it */
                if (defender->t_action == A_SUMMON ||
                    defender->t_action == A_MISSILE) {
                    /* Just make the old fellow start over again */
                    defender->t_action = A_NIL;
                    defender->t_no_move = movement(defender);
                    defender->t_using = NULL;

                    if (see_def)
                        msg("%s was disrupted.", prname(defname, TRUE));
                }

#ifdef FLEEMONST
                /*
                 * If the monster is fairly intelligent and about to die,
                 * it may turn tail and run.
                 */
                if ((tp->t_stats.s_hpt < max(10, tp->maxstats.s_hpt/10)) &&
                    (rnd(21) < tp->t_stats.s_intel)) {
                        turn_on(*tp, ISFLEE);

                    /* If monster was suffocating, stop it */
                    if (on(*tp, DIDSUFFOCATE)) {
                        turn_off(*tp, DIDSUFFOCATE);
                        extinguish(suffocate);
                    }

                    /* If monster held us, stop it */
                    if (on(*tp, DIDHOLD) && (--hold_count == 0))
                            turn_off(player, ISHELD);
                    turn_off(*tp, DIDHOLD);

                    /* It is okay to turn tail */
                    tp->t_oldpos = tp->t_pos;
                }
#endif
            }
        }
        else {
            /* If the thing was trying to surprise, no good */
            if (on(*attacker, CANSURPRISE)) {
                /* If we can't see it, it keeps surprise (from us) */
                if (see_att) turn_off(*attacker, CANSURPRISE);
            }

            miss(weap, see_att, see_def, attname, defname, thrown, FALSE);
        }
    }
    return did_hit;
}

