-- DB update 2026_05_14_00 -> 2026_05_16_00
--
UPDATE `creature_template`
SET `AIName` = '',
    `ScriptName` = 'npc_balinda_greater_water_elemental'
WHERE `entry` = 25040;

DELETE FROM `smart_scripts`
WHERE `entryorguid` = 25040
  AND `source_type` = 0;
