#ifndef STRUCTS_H
#define STRUCTS_H

#include "vector.h"

struct playerInfo{
    unsigned godId;
    int playerId;
    int nameLen;
    unsigned short *name;
    int teamId;
    char *data;
};

struct teamInfo{
    int teamId;
    int nameLen;
    char *name;
    int playerCount;
    struct playerInfo **players;
};

struct ActionSummary{
    int totalActions;
    int groupChanges;
    int movements;
    int unitsTrained;
    int buildingsBuilt;
    int researches;
    int stanceChanges;
    int formationChanges;
    int TCBellRang;
    int unitsGarrisoned;
    int resign;
    float foodSent, goldSent, woodSent;
    float foodReceived, goldReceived, woodReceived;
    float foodSold, goldSold, woodSold;
    float foodBought, goldBought, woodBought;
    float estimFoodGathered, estimGoldGathered, estimWoodGathered;
    int kronosTimeShifts;
    int cheats;
};


struct gameData{
    unsigned char *fileMem;
    int fileLen;
    int playerCount;
    int teamCount;
    struct teamInfo **teams;
    struct playerInfo *motherNature;
    unsigned short *gameInfo_xml;
    int gameInfo_xml_len;
    unsigned char *scripts;
    int scripts_len;
    unsigned char *intro;
    int intro_len;
    unsigned char *data;
    int data_len;
    unsigned char *game_records_mem;
    int game_records_mem_len;
    vector <Action> gameActions;
    //int actionLastIndex;
    int gameTicks;
    int realGameEventCount;
    ActionSummary playersSummary[10];
    int totalEventCount;
};

#endif // STRUCTS_H
