#ifndef RECORDING_PARSER_H
#define RECORDING_PARSER_H

#include "structs.h"

struct playerInfo;

/*ACTION CODES

0x0 = move/gather/attack,
0x1 = research from building
0x2 = train unit from a building
0x3 = build,
0x4 = set rally point of a building
0x7 = kill
0xa = stop unit
0x10 = use god power
0x11 = market action
0x12 = ungarrison
0x14 = resign
0x16 = garrison units
0x17 = tribute
0x1c = convert
0x21 = set stance (aggressive/defensive/stand ground)
0x2a = town bell
0x2e = repair building
0x2f = empower
0x33 = set formation
0x34 = cheat
0x35 = kronos time shift building
0x36 = continuous training on/off

0x50 = viewlock
0x51 = select
0x52 = add to group

*/


class RecParser{
    public:
    RecParser();
    void ParseFile(const char *filename);

    private:
    playerInfo* findPlayer(int playerId);
    void printPlayerSummary(int player, std::ofstream &file);
    void decompressFile(const char *filename);
    void load_rec_file(const char *filename);
    void load_game_info();
    void load_game_data();
    void updateGameRecord(int actionCode);
    void fillCommonRecord(int actionCode, unsigned char **ptr);
    int parseRecord(int headerCode, unsigned char *ptr);
    int findNextValidRecord(unsigned char *ptr);
    int parseHeader(unsigned char *ptr);
    void load_game_actions();
    int isThereAWinner();
    void print_results(const char* filename);
    void create_gameInfo_file(const char* name);
    void create_scripts_file(const char* name);
    gameData data;

};

#endif // RECORDING_PARSER_H
