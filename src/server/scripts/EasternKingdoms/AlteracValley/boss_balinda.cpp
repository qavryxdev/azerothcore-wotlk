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

    static void ApplyCrowdControlImmunities(Creature* creature)
    {
        for (uint32 mechanic = MECHANIC_CHARM; mechanic < MAX_MECHANIC; ++mechanic)
            if (IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK & (1ULL << mechanic))
                creature->ApplySpellImmune(0, IMMUNITY_MECHANIC, mechanic, true);
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
        ApplyCrowdControlImmunities(elemental);

        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 50, true))
            elemental->AI()->AttackStart(target);
    }

    SummonList summons;
    uint8 _iceBlockCount;
};

void AddSC_boss_balinda()
{
    RegisterCreatureAI(boss_balinda);
}
