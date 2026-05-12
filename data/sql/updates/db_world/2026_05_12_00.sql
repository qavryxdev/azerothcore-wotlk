-- Keep Alterac Valley tower bowmen visibly armed and able to keep shooting.
UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` IN (13358, 13359);

INSERT INTO `creature_template_addon` (`entry`, `path_id`, `mount`, `bytes1`, `bytes2`, `emote`, `visibilityDistanceType`, `auras`) VALUES
(13358, 0, 0, 0, 2, 0, 0, ''),
(13359, 0, 0, 0, 2, 0, 0, '')
ON DUPLICATE KEY UPDATE `bytes2` = VALUES(`bytes2`);

INSERT INTO `creature_equip_template` (`CreatureID`, `ID`, `ItemID1`, `ItemID2`, `ItemID3`, `VerifiedBuild`) VALUES
(13358, 1, 0, 0, 5262, 18019),
(13359, 1, 0, 0, 5261, 18019)
ON DUPLICATE KEY UPDATE `ItemID1` = VALUES(`ItemID1`), `ItemID2` = VALUES(`ItemID2`), `ItemID3` = VALUES(`ItemID3`);

UPDATE `smart_scripts` SET `event_phase_mask` = 0 WHERE `entryorguid` IN (13358, 13359) AND `source_type` = 0 AND `id` = 3;

DELETE FROM `smart_scripts` WHERE `entryorguid` IN (13358, 13359) AND `source_type` = 0 AND `id` IN (6, 7);

INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `event_param6`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_param4`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
(13358, 0, 6, 7, 10, 0, 100, 0, 0, 80, 2000, 3000, 1, 0, 49, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 'Stormpike Bowman - Within 0-80 Range Out of Combat LoS - Attack Start'),
(13358, 0, 7, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 0, 11, 22121, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 'Stormpike Bowman - Out of Combat LoS - Cast Shoot'),
(13359, 0, 6, 7, 10, 0, 100, 0, 0, 80, 2000, 3000, 1, 0, 49, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 'Frostwolf Bowman - Within 0-80 Range Out of Combat LoS - Attack Start'),
(13359, 0, 7, 0, 61, 0, 100, 0, 0, 0, 0, 0, 0, 0, 11, 22121, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 'Frostwolf Bowman - Out of Combat LoS - Cast Shoot');
