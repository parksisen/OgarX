/** @type {typeof import("../../physics/engine").DefaultSettings} */
module.exports = {
    VIRUS_COUNT: 20,
    PLAYER_MAX_CELLS: 64,
    PLAYER_NO_MERGE_DELAY: 22,
    PLAYER_NO_COLLI_DELAY: 12,
    PLAYER_MERGE_NEW_VER: false,
    // PLAYER_AUTOSPLIT_SIZE: 0,
    PLAYER_SPLIT_CAP: 4,
    PLAYER_MERGE_TIME: 0,
    // VIRUS_MONOTONE_POP: true,
    EJECT_SIZE: 38,
    EJECT_LOSS: 41,
    EJECT_DELAY: 80,
    PELLET_COUNT: 1000,
    PLAYER_SPAWN_SIZE: 1500,
    MAP_HW: 32767 >> 2, // MAX signed short
    MAP_HH: 32767 >> 2, // MAX signed short,
};