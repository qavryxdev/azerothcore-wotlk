-- Use a dedicated turret AI for Alterac Valley tower bowmen.
-- SmartAI could open with one Shoot cast, then fall back into melee-looking combat.
UPDATE `creature_template`
SET `AIName` = '', `ScriptName` = 'npc_av_tower_bowman'
WHERE `entry` IN (13358, 13359);

INSERT INTO `creature_template_addon` (`entry`, `path_id`, `mount`, `bytes1`, `bytes2`, `emote`, `visibilityDistanceType`, `auras`) VALUES
(13358, 0, 0, 0, 2, 0, 0, ''),
(13359, 0, 0, 0, 2, 0, 0, '')
ON DUPLICATE KEY UPDATE `bytes2` = VALUES(`bytes2`);

INSERT INTO `creature_equip_template` (`CreatureID`, `ID`, `ItemID1`, `ItemID2`, `ItemID3`, `VerifiedBuild`) VALUES
(13358, 1, 0, 0, 5262, 18019),
(13359, 1, 0, 0, 5261, 18019)
ON DUPLICATE KEY UPDATE
`ItemID1` = VALUES(`ItemID1`),
`ItemID2` = VALUES(`ItemID2`),
`ItemID3` = VALUES(`ItemID3`);
