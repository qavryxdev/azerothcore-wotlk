/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "CreatureScript.h"
#include "ScriptedCreature.h"
#include "SpellAuraDefines.h"

enum Spells
{
    SPELL_ARCANE_EXPLOSION                        = 46608,
    SPELL_CONE_OF_COLD                            = 38384,
    SPELL_FIREBALL                                = 46988,
    SPELL_FROSTBOLT                               = 46987,
    SPELL_WATER_BOLT                              = 46983,
    SPELL_SUMMON_WATER_ELEMENTAL                  = 45067,
    SPELL_ICEBLOCK                                = 46604
};

enum Yells
{
    SAY_AGGRO                                   = 0,
    SAY_EVADE                                   = 1,
    SAY_SALVATION                               = 2,
};

enum Creatures
{
    NPC_WATER_ELEMENTAL                           = 25040
};

namespace
{
    constexpr uint32 BALINDA_ELEMENTAL_HEALTH = 180000;
    constexpr uint32 BALINDA_ELEMENTAL_WATER_BOLT_MIN_DAMAGE = 850;
    constexpr uint32 BALINDA_ELEMENTAL_WATER_BOLT_MAX_DAMAGE = 1050;
    constexpr float BALINDA_ELEMENTAL_WATER_BOLT_RANGE = 45.0f;
    constexpr uint32 BALINDA_ELEMENTAL_MELEE_MIN_DAMAGE = 500;
    constexpr uint32 BALINDA_ELEMENTAL_MELEE_MAX_DAMAGE = 700;
    constexpr uint32 BALINDA_ELEMENTAL_ATTACK_TIME = 2000;

    void ApplyCrowdControlImmunities(Creature* creature)
    {
        for (uint32 mechanic = MECHANIC_CHARM; mechanic < MAX_MECHANIC; ++mechanic)
            if (IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK & (1ULL << mechanic))
                creature->ApplySpellImmune(0, IMMUNITY_MECHANIC, mechanic, true);
    }

    void ConfigureWaterElementalCombatStats(Creature* elemental)
    {
        elemental->SetMaxHealth(BALINDA_ELEMENTAL_HEALTH);
        elemental->SetHealth(BALINDA_ELEMENTAL_HEALTH);
        elemental->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, static_cast<float>(BALINDA_ELEMENTAL_MELEE_MIN_DAMAGE));
        elemental->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, static_cast<float>(BALINDA_ELEMENTAL_MELEE_MAX_DAMAGE));
        elemental->SetAttackTime(BASE_ATTACK, BALINDA_ELEMENTAL_ATTACK_TIME);
        elemental->UpdateDamagePhysical(BASE_ATTACK);
        elemental->SetReactState(REACT_AGGRESSIVE);
    }
}

struct boss_balinda : public ScriptedAI
{
    boss_balinda(Creature* creature) : ScriptedAI(creature), summons(me), _iceBlockCount(0)
    {
        ApplyCasterImmunities(me);
    }

    void Reset() override
    {
        summons.DespawnAll();
        _iceBlockCount = 0;
        ApplyCasterImmunities(me);
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _iceBlockCount = 0;

        Talk(SAY_AGGRO);
        SummonWaterElemental();

        ScheduleTimedEvent(35s, [&]
        {
            SummonWaterElemental();
        }, 35s, 45s);

        ScheduleTimedEvent(5s, 10s, [&]
        {
            CastIfReady([&] { DoCastAOE(SPELL_ARCANE_EXPLOSION); });
        }, 6s, 10s);

        ScheduleTimedEvent(8s, [&]
        {
            CastIfReady([&] { DoCastVictim(SPELL_CONE_OF_COLD); });
        }, 9s, 14s);

        ScheduleTimedEvent(1s, [&]
        {
            CastAtEnemy(SPELL_FIREBALL);
        }, 4s, 6s);

        ScheduleTimedEvent(4s, [&]
        {
            CastAtEnemy(SPELL_FROSTBOLT);
        }, 6s, 9s);

        ScheduleTimedEvent(5s, [&]
        {
            if (me->GetDistance2d(me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY()) > 50)
            {
                EnterEvadeMode();
                Talk(SAY_EVADE);
            }

            if (Creature* elemental = summons.GetCreatureWithEntry(NPC_WATER_ELEMENTAL))
            {
                if (elemental->GetDistance2d(me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY()) > 50)
                {
                    elemental->AI()->EnterEvadeMode();
                }
            }

        }, 5s, 5s);
    }

    void JustSummoned(Creature* summoned) override
    {
        if (summoned->GetEntry() == NPC_WATER_ELEMENTAL)
        {
            ConfigureWaterElemental(summoned);
            summons.Summon(summoned);
        }
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*type*/, SpellSchoolMask /*school*/) override
    {
        if (_iceBlockCount == 0 && me->HealthBelowPctDamaged(65, damage))
        {
            DoCastSelf(SPELL_ICEBLOCK);
            damage = 0;
            ++_iceBlockCount;
        }
        else if (_iceBlockCount == 1 && me->HealthBelowPctDamaged(35, damage))
        {
            DoCastSelf(SPELL_ICEBLOCK);
            damage = 0;
            ++_iceBlockCount;
        }
    }

    void DamageDealt(Unit* /*victim*/, uint32& damage, DamageEffectType damageType, SpellSchoolMask damageSchoolMask) override
    {
        if (damageType != DIRECT_DAMAGE && (damageSchoolMask & SPELL_SCHOOL_MASK_MAGIC))
            damage *= 2;
    }

    void JustDied(Unit* /*killer*/) override
    {
        summons.DespawnAll();
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        scheduler.Update(diff,
            std::bind(&ScriptedAI::DoMeleeAttackIfReady, this));
    }

private:
    static void ApplyCasterImmunities(Creature* creature)
    {
        creature->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_INTERRUPT_CAST, true);
        creature->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_INTERRUPT, true);
        creature->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SILENCE, true);
        creature->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_SILENCE, true);
        creature->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_PACIFY_SILENCE, true);
        creature->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK, true);
    }

    template <typename Action>
    void CastIfReady(Action&& action)
    {
        if (!me->HasUnitState(UNIT_STATE_CASTING))
            action();
    }

    void CastAtEnemy(uint32 spellId)
    {
        CastIfReady([&]
        {
            if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 40.0f, true))
                DoCast(target, spellId);
            else
                DoCastVictim(spellId);
        });
    }

    void SummonWaterElemental()
    {
        if (!summons.empty())
            return;

        SpellCastResult result = DoCastSelf(SPELL_SUMMON_WATER_ELEMENTAL);
        if (result == SPELL_CAST_OK && !summons.empty())
            return;

        if (Creature* elemental = DoSummon(NPC_WATER_ELEMENTAL, me, 4.0f, 45 * IN_MILLISECONDS, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN))
        {
            if (summons.empty())
            {
                ConfigureWaterElemental(elemental);
                summons.Summon(elemental);
            }
        }
    }

    void ConfigureWaterElemental(Creature* elemental)
    {
        elemental->SetLevel(me->GetLevel());
        elemental->SetFaction(me->GetFaction());
        ConfigureWaterElementalCombatStats(elemental);
        ApplyCrowdControlImmunities(elemental);

        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 50, true))
            elemental->AI()->AttackStart(target);
    }

    SummonList summons;
    uint8 _iceBlockCount;
};

struct npc_balinda_greater_water_elemental : public ScriptedAI
{
    npc_balinda_greater_water_elemental(Creature* creature) : ScriptedAI(creature)
    {
        ConfigureWaterElemental();
    }

    void Reset() override
    {
        scheduler.CancelAll();
        ConfigureWaterElemental();
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        scheduler.CancelAll();

        ScheduleTimedEvent(1s, [&]
        {
            CastWaterBolt();
        }, 1s, 2s);
    }

    void DamageDealt(Unit* /*victim*/, uint32& damage, DamageEffectType damageType, SpellSchoolMask damageSchoolMask) override
    {
        if (damageType == SPELL_DIRECT_DAMAGE && (damageSchoolMask & SPELL_SCHOOL_MASK_FROST))
            damage = urand(BALINDA_ELEMENTAL_WATER_BOLT_MIN_DAMAGE, BALINDA_ELEMENTAL_WATER_BOLT_MAX_DAMAGE);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        scheduler.Update(diff,
            std::bind(&ScriptedAI::DoMeleeAttackIfReady, this));
    }

private:
    void ConfigureWaterElemental()
    {
        ConfigureWaterElementalCombatStats(me);
        ApplyCrowdControlImmunities(me);
    }

    void CastWaterBolt()
    {
        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        if (Unit* target = SelectWaterBoltTarget())
            DoCast(target, SPELL_WATER_BOLT);
    }

    Unit* SelectWaterBoltTarget()
    {
        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, [&](Unit* target)
        {
            return target && target->IsPlayer() && target->getPowerType() == POWER_MANA &&
                me->IsWithinCombatRange(target, BALINDA_ELEMENTAL_WATER_BOLT_RANGE) &&
                me->IsWithinLOSInMap(target);
        }))
            return target;

        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, RangeSelector(me, BALINDA_ELEMENTAL_WATER_BOLT_RANGE, true, true, 8.0f)))
            return target;

        return SelectTarget(SelectTargetMethod::Random, 0, BALINDA_ELEMENTAL_WATER_BOLT_RANGE, true);
    }
};

void AddSC_boss_balinda()
{
    RegisterCreatureAI(boss_balinda);
    RegisterCreatureAI(npc_balinda_greater_water_elemental);
}
