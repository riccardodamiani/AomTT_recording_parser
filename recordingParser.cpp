#include "recordingParser.h"
#include "structs.h"
#include "vector.h"

#include <string.h>
#include <malloc.h>
#include <fstream>
#include <iostream>

//#define UNKNOWN_RECORD_DEBUG
#ifdef UNKNOWN_RECORD_DEBUG
#include "debug.h"
#endif

const char* majorGodNames[] = {"Zeus", "Poseidon", "Hades", "Isis", "Ra", "Set", "Odin", "Thor", "Loki", "Kronos", "Oranos", "Gaia"};

RecParser::RecParser(){
    data = {};

#ifdef UNKNOWN_RECORD_DEBUG
    memset(unknownArray, 0, sizeof(unknownArray));
    unknownRecordCount = 0;
#endif // DEBUG

}

void RecParser::ParseFile(const char *filename){
    load_rec_file(filename);
    load_game_info();
    load_game_data();
    load_game_actions();
    print_results(filename);
}

playerInfo* RecParser::findPlayer(int playerId){
    int j = 0, k = 0;
    for(k = 0; k < data.teamCount; k++){
        for(j = 0; j < data.teams[k]->playerCount; j++){
            if(data.teams[k]->players[j]->playerId == playerId){
                return data.teams[k]->players[j];
            }
        }
    }
    return 0;
};

void RecParser::printPlayerSummary(int player, std::ofstream &file){
    file << " ## Player " << player << ": ";

    struct playerInfo *p = findPlayer(player);
    if(p == 0){
        return;
    }

    int i;
    char tempName[256];
    for(i = 0; i <= p->nameLen; i++){   //convert to ascii
        tempName[i] = (char)p->name[i];
    }
    file << tempName;

    file << " (" << (p->godId < 12 ? majorGodNames[p->godId] : "Invalid") << ")";
    file << "\n Total actions: " << data.playersSummary[player].totalActions;
    file << "\n Move commmands: " << data.playersSummary[player].movements;
    file << "\n Units trained: " << data.playersSummary[player].unitsTrained;
    file << "\n Researches completed: " << data.playersSummary[player].researches;
    file << "\n Building contructed: " << data.playersSummary[player].buildingsBuilt;
    file << "\n Units group changes: " << data.playersSummary[player].groupChanges;
    file << "\n Units garrisoned: " << data.playersSummary[player].unitsGarrisoned;
    file << "\n Unit stance changes: " << data.playersSummary[player].stanceChanges;
    file << "\n Unit formation changes: " << data.playersSummary[player].formationChanges;
    file << "\n Town bell rang: " << data.playersSummary[player].TCBellRang;
    if(p->godId == 9) file << "\n Time shifted buildings: " << data.playersSummary[player].kronosTimeShifts;
    if(data.playersSummary[player].cheats > 0) file << "\n Cheats: " << data.playersSummary[player].cheats;
    file << "\n Resources sold: " << (int)data.playersSummary[player].foodSold << " food, " << (int)data.playersSummary[player].woodSold << " wood";
    file << "\n Resources bought: " << (int)data.playersSummary[player].foodBought << " food, " << (int)data.playersSummary[player].woodBought << " wood";
    file << "\n Resources sent to ally: " << (int)data.playersSummary[player].foodSent << " food, " << (int)data.playersSummary[player].woodSent << " wood, " <<
                (int)data.playersSummary[player].goldSent << " gold";
    file << "\n Resources received from ally: " << (int)data.playersSummary[player].foodReceived << " food, " << (int)data.playersSummary[player].woodReceived << " wood, " <<
                (int)data.playersSummary[player].goldReceived << " gold";
    file << "\n Player resigned: " << (data.playersSummary[player].resign == 1 ? "Yes":"No");
}


void RecParser::decompressFile(const char *filename){
    char buffer[200];

    sprintf(buffer, "python zlib.py -d \"%s\"", filename);
    system(buffer);
}


void RecParser::load_rec_file(const char *filename){

    //load game event count
    std::ifstream file(filename, std::ios::binary);
    //FILE *f = fopen(filename, "rb");
    if(!file){
        std::cout << "\n !! No file with name " << filename << " found";
        exit(EXIT_FAILURE);
    }
    int dat;
    file.seekg(-4, std::ios::end);
    file.read((char*)&dat, 4);
    file.close();
    data.realGameEventCount = dat;

    //decompress the file
    decompressFile(filename);

    //load the decompressed file
    file.open("decompressed", std::ios::binary);
    if(!file){
        std::cout << "\n !! Error loading the decompressed recording file.";
        exit(EXIT_FAILURE);
    }
    file.seekg(0, std::ios::end);
    int f_len = file.tellg();
    data.fileMem = (unsigned char *)malloc(f_len);
    file.seekg(0, std::ios::beg);
    file.read((char*)data.fileMem, f_len);
    file.close();
    data.fileLen = f_len;
}

void RecParser::load_game_info(){
    int i;

    data.intro = data.fileMem;

    for(i = 0; i < data.fileLen / 4; i++){
        if(((unsigned int*)data.intro)[i] == 0xfeff0000){
            data.gameInfo_xml = (unsigned short *)( &(((unsigned int*)data.intro)[i+1]) );
            break;
        }
    }

    data.intro_len = (unsigned char *)data.gameInfo_xml - data.intro;

    unsigned char *p = (unsigned char *)data.gameInfo_xml;
    do{
        p++;
    }while(*p != '{');
    do{
        p--;
    }while(*((unsigned int *)p) != 0x00000400);
    data.scripts = p + 4;

    data.gameInfo_xml_len = (data.scripts - (unsigned char *)data.gameInfo_xml - 8);

    //find the end of the script section
    int count = 0;
    unsigned char *ptr = data.scripts;
    for(ptr; ptr < data.fileMem + data.fileLen; ptr++){
        if(*ptr == '{'){
            ++ptr;
            ++count;
            break;
        }
    }
    for(ptr; ptr < data.fileMem + data.fileLen; ptr++){
        if(*ptr == '{'){
            ++count;
        }else if(*ptr == '}'){
            --count;
        }
        if(count == 0){
            break;
        }
    }
    do{
        ++ptr;
    }while(*ptr == 0x20 || *ptr == 0xa || *ptr == 0xd);
    data.data = ptr;

    data.scripts_len = data.data - data.scripts;
    data.data_len = data.fileLen - (data.data - data.fileMem);
}


void RecParser::load_game_data(){
    unsigned char* ptr = data.data;

    do{
        ++ptr;
    }while(!(((*ptr&0xf0) == 0x30) && ((*(ptr+5) & 0xf0) == 0x30)));
    ptr -= 4;
    data.teamCount = *(int*)ptr;
    ptr += 31;

    //alloc teams array
    data.teams = (teamInfo **)malloc(data.teamCount * sizeof(struct teamInfo *));

    int i = 0;
    int playerCount = 0;
    for(i = 0; i < data.teamCount; i++){
        //alloc each team memory
        data.teams[i] = (teamInfo *)malloc(sizeof(struct teamInfo));

        //get the team id
        data.teams[i]->teamId = *(int*)ptr;
        ptr += 4;

        //get the team name length
        data.teams[i]->nameLen = *(int*)ptr;
        ptr += 4;

        //alloc memory for the team name (null terminated)
        data.teams[i]->name = (char *)malloc(data.teams[i]->nameLen + 1);

        //copy the team name
        int j = 0;
        for(j = 0; j < data.teams[i]->nameLen; j++)
            data.teams[i]->name[j] = *(ptr++);
        data.teams[i]->name[data.teams[i]->nameLen] = '\0';

        //get the numebr of players in this team
        data.teams[i]->playerCount = *(int*)ptr;
        ptr += 4;

        //allocate the array of players
        data.teams[i]->players = (playerInfo **)malloc(data.teams[i]->playerCount * sizeof(struct playerInfo *));

        for(j = 0; j < data.teams[i]->playerCount; j++){
            //allocate each player memory
            data.teams[i]->players[j] = (playerInfo *)malloc(sizeof(struct playerInfo));

            //set its player id
            data.teams[i]->players[j]->playerId = *(int*)ptr;
            ptr += 4;

            //sets its team id
            data.teams[i]->players[j]->teamId = data.teams[i]->teamId;

            //count the total number of players
            ++playerCount;
        }
        //go to the next team mem
        ptr += 5;
    }

    //have to search the value like this due to
    //erratic offsets
    ptr-=5;
    do{
        ++ptr;
    }while(*(short*)ptr != 0x4b01);
    do{
        --ptr;
    }while(*ptr == 0);

    playerCount++;
    if(*(int*)ptr != playerCount){
        std::cout << "!! ERROR: Player counts do not match: " << (int*)ptr << " != " << playerCount;
        exit(EXIT_FAILURE);
    }

    data.playerCount = playerCount;
    ptr += 4;
    //get the player informations
    for(i = 0; i < playerCount; i++){
        ptr += 11;

        if(i == 0){ //mother nature
            data.motherNature = (playerInfo *)malloc(sizeof(struct playerInfo));

            //get the mother nature name length
            data.motherNature->nameLen = *(int*)ptr;
            ptr += 4;

            //allocate name memory
            data.motherNature->name = (unsigned short *)malloc((data.motherNature->nameLen + 1) * sizeof(unsigned short));

            int j;
            //copy the name
            for(j = 0; j < data.motherNature->nameLen; j++){
                data.motherNature->name[j] = *(unsigned short *)ptr;
                ptr += 2;
            }
            data.motherNature->name[data.motherNature->nameLen] = 0x0;

            //go to the next record
            ptr += 97;

            continue;
        }

        //find the player in the teams
        struct playerInfo *p = findPlayer(i);
        if(p == 0){
            std::cout << " !! ERROR: Cannot find player: " << i;
            exit(EXIT_FAILURE);
        }

        //get the player name length
        p->nameLen = *(int*)ptr;
        ptr += 4;

        //allocate name memory
        p->name = (unsigned short *)malloc((p->nameLen + 1) * sizeof(unsigned short));

        int j;
        //copy the name
        for(j = 0; j < p->nameLen; j++){
            p->name[j] = *(unsigned short *)ptr;
            ptr += 2;
        }
        p->name[p->nameLen] = 0x0;

        ptr += 9;
        p->godId = *(unsigned int *)ptr;

        //go to the next record
        ptr += 88;
    }

    ptr += 51;
    data.game_records_mem = ptr;
    data.game_records_mem_len = data.data_len - (data.game_records_mem - data.data);
}

void RecParser::updateGameRecord(int actionCode){
    switch(actionCode){
    case 0: //movements
        {
            Action &lastAction = data.gameActions.back();
            int player = lastAction.player;
            data.playersSummary[player].movements++;
            data.playersSummary[player].totalActions++;
        }
        break;

    case 1: //researches
        {
            Action &lastAction = data.gameActions.back();
            int player = lastAction.player;
            if(lastAction.data2){ //remove from queue
                data.playersSummary[player].researches--;
            }else
                data.playersSummary[player].researches++;
            data.playersSummary[player].totalActions++;
        }
        break;

    case 2: //units trained
        {
            Action &lastAction = data.gameActions.back();
            int player = lastAction.player;
            if(lastAction.data2){ //remove from queue
                data.playersSummary[player].unitsTrained -= lastAction.entityCount;
            }else{
                data.playersSummary[player].unitsTrained += lastAction.entityCount;
            }

            data.playersSummary[player].totalActions++;
        }
        break;

    case 3:     //build
        {
            Action &lastAction = data.gameActions.back();
            int player = lastAction.player;
            data.playersSummary[player].buildingsBuilt++;
            data.playersSummary[player].totalActions++;
        }
        break;

    case 4: //set rally point
        {
            Action &lastAction = data.gameActions.back();
            int player = lastAction.player;
            data.playersSummary[player].totalActions++;
        }
        break;

    case 7: //kill
        {
            Action &lastAction = data.gameActions.back();
            int player = lastAction.player;
            data.playersSummary[player].totalActions++;
        }
        break;

    case 0xa:       //stop
        {
            Action &lastAction = data.gameActions.back();
            int player = lastAction.player;
            data.playersSummary[player].totalActions++;
        }
        break;

    case 0x10:  //god powers
        {
            Action &lastAction = data.gameActions.back();
            int player = lastAction.player;
            data.playersSummary[player].totalActions++;
        }
        break;

    case 0x11:  //market actions
        {
            Action &lastAction = data.gameActions.back();
            int player = lastAction.player;
            int resourceType = lastAction.data1;
            if(resourceType == 0){
                if(lastAction.float_data.x > 0)
                    data.playersSummary[player].goldBought += lastAction.float_data.x;
                else
                    data.playersSummary[player].goldSold -= lastAction.float_data.x;
            }else if(resourceType == 1){
                if(lastAction.float_data.x > 0)
                    data.playersSummary[player].woodBought += lastAction.float_data.x;
                else
                    data.playersSummary[player].woodSold -= lastAction.float_data.x;
                //data.playersSummary[player].marketWoodTrans += lastAction.float_data.x;
            }else if(resourceType == 2){
                if(lastAction.float_data.x > 0)
                    data.playersSummary[player].foodBought += lastAction.float_data.x;
                else
                    data.playersSummary[player].foodSold -= lastAction.float_data.x;
                //data.playersSummary[player].marketFoodTrans += lastAction.float_data.x;
            }

            data.playersSummary[player].totalActions++;
        }
        break;

    case 0x12:  //ungarrison
        {
            Action &lastAction = data.gameActions.back();
            int player = lastAction.player;
            data.playersSummary[player].totalActions++;
        }
        break;

    case 0x14:      //resign
        {
            Action &lastAction = data.gameActions.back();
            int player = lastAction.player;
            data.playersSummary[player].resign = 1;
            data.playersSummary[player].totalActions++;
        }
        break;

    case 0x16:  //garrison
        {
            Action &lastAction = data.gameActions.back();
            int player = lastAction.player;
            data.playersSummary[player].unitsGarrisoned += lastAction.entityCount;
            data.playersSummary[player].totalActions++;
        }
        break;

    case 0x17:  //tribute
        {
            Action &lastAction = data.gameActions.back();
            int player = lastAction.player;
            int playerRecv = lastAction.data2;
            int resourceType = lastAction.data1;
             if(resourceType == 0){
                data.playersSummary[player].goldSent += lastAction.float_data.x;
                data.playersSummary[playerRecv].goldReceived += lastAction.float_data.x;
            }else if(resourceType == 1){
                data.playersSummary[player].woodSent += lastAction.float_data.x;
                data.playersSummary[playerRecv].woodReceived += lastAction.float_data.x;
            }else if(resourceType == 2){
                data.playersSummary[player].foodSent += lastAction.float_data.x;
                data.playersSummary[playerRecv].foodReceived += lastAction.float_data.x;
            }
            data.playersSummary[player].totalActions++;
        }
        break;

    case 0x1c:  //convert
        {
            Action &lastAction = data.gameActions.back();
            int player = lastAction.player;
            data.playersSummary[player].totalActions++;
        }
        break;

    case 0x21:  //set stance
        {
            Action &lastAction = data.gameActions.back();
            int player = lastAction.player;
            data.playersSummary[player].stanceChanges++;
            data.playersSummary[player].totalActions++;
        }
        break;

    case 0x2a:  //town bell
        {
            Action &lastAction = data.gameActions.back();
            int player = lastAction.player;
            data.playersSummary[player].TCBellRang++;
            data.playersSummary[player].totalActions++;
        }
        break;

    case 0x2e:  //repair
        {
            Action &lastAction = data.gameActions.back();
            int player = lastAction.player;
            data.playersSummary[player].totalActions++;
        }
        break;

    case 0x2f:  //empower
        {
            Action &lastAction = data.gameActions.back();
            int player = lastAction.player;
            data.playersSummary[player].totalActions++;
        }
        break;

    case 0x33:  //formation changes
        {
            Action &lastAction = data.gameActions.back();
            int player = lastAction.player;
            data.playersSummary[player].formationChanges++;
            data.playersSummary[player].totalActions++;
        }
        break;

    case 0x34:  //cheats
        {
            Action &lastAction = data.gameActions.back();
            int player = lastAction.player;
            data.playersSummary[player].cheats++;
            data.playersSummary[player].totalActions++;
        }
        break;

    case 0x35:  //kronos time shift building
        {
            Action &lastAction = data.gameActions.back();
            int player = lastAction.player;
            data.playersSummary[player].kronosTimeShifts++;
            data.playersSummary[player].totalActions++;
        }
        break;

    case 0x36:      //countinous training
        {
            Action &lastAction = data.gameActions.back();
            int player = lastAction.player;
            data.playersSummary[player].totalActions++;
        }
        break;

    case 0x51:  //select
        {
            Action &lastAction = data.gameActions.back();
            int player = lastAction.player;
            data.playersSummary[player].totalActions++;
        }
        break;

    case 0x52:  //add group
        {
            Action &lastAction = data.gameActions.back();
            int player = lastAction.player;
            data.playersSummary[player].groupChanges++;
            data.playersSummary[player].totalActions++;
        }
        break;
    }
}


void RecParser::fillCommonRecord(int actionCode, unsigned char **ptr){
    int i = 0;

    Action &lastAction = data.gameActions.back();
    lastAction.action = actionCode;
    lastAction.player = *(int *)(*ptr + 5);
    if(lastAction.player > 12){
        std::cout << "\n Invalid player id: " << lastAction.player;
        exit(EXIT_FAILURE);
    }
    *ptr += 33;
    int entityCount = *(unsigned int *)(*ptr);
    lastAction.entityCount = entityCount;
    lastAction.entities = (unsigned int*)malloc(entityCount * sizeof(unsigned int));
    *ptr += 4;
    for(i = 0; i < entityCount; i++){
        lastAction.entities[i] = *(unsigned int *)(*ptr);
        *ptr += 4;
    }
}

int RecParser::parseRecord(int headerCode, unsigned char *ptr){
    int entityCount = 0;
    unsigned char *starting_ptr = ptr;

    if(headerCode == 0xa1){
        if(*(int *)ptr == 0x2d){
            data.gameActions.push_back();
            Action &lastAction = data.gameActions.back();
            fillCommonRecord(0x52, &ptr);
            ptr += 4;
            int zeroArrayLen = *(unsigned int *)(ptr);
            ptr += 25 + zeroArrayLen;
            lastAction.data1 = *ptr;    //0x1 = remove from group
            ptr++;
            lastAction.data2 = *(unsigned int *)ptr;    //group
            ptr += 5;
            //data.totalEventCount++;
        }
    }else if(headerCode == 0x21){
        if(*(int *)ptr != *(char *)(ptr+4)){    //not a valid record
            return ptr - starting_ptr;
        }

        switch(*(int *)ptr ){
        case 0: //move
            {
                data.gameActions.push_back();
                Action &lastAction = data.gameActions.back();
                fillCommonRecord(*(int *)ptr, &ptr);
                unsigned int immidiate_position = *(unsigned int *)(ptr);
                if(immidiate_position > 0){
                    lastAction.float_data.x = *(float *)(ptr+4);
                    lastAction.float_data.y = *(float *)(ptr+8);
                    lastAction.float_data.z = *(float *)(ptr+12);
                    ptr += 4;
                    ptr += immidiate_position * 12;
                    int zeroArrayLen = *(unsigned int *)(ptr);
                    ptr += 46 + zeroArrayLen;
                }else{
                    ptr += 4;
                    int zeroArrayLen = *(unsigned int *)(ptr);
                    if(zeroArrayLen > 4){
                        zeroArrayLen = 4;
                    }
                    ptr += 25 + zeroArrayLen;
                    lastAction.targetEntity = *(unsigned int *)ptr;
                    ptr += 8;
                    lastAction.float_data.x = *(float *)(ptr);
                    lastAction.float_data.y = *(float *)(ptr+4);
                    lastAction.float_data.z = *(float *)(ptr+8);
                    ptr += 13;
                }
                updateGameRecord(0);
            }
            break;

        case 1: //research
            {
                data.gameActions.push_back();
                Action &lastAction = data.gameActions.back();
                fillCommonRecord(*(int *)ptr, &ptr);
                ptr += 4;
                int zeroArrayLen = *(unsigned int *)(ptr);
                ptr += 25 + zeroArrayLen;
                lastAction.data1 = *(unsigned int *)(ptr);  //research id
                lastAction.data2 = *(unsigned int *)(ptr+4);    //1 = delete from queue
                ptr += 9;
                updateGameRecord(0x1);
            }
            break;

        case 2: //train unit
            {
                data.gameActions.push_back();
                Action &lastAction = data.gameActions.back();
                fillCommonRecord(*(int *)ptr, &ptr);
                ptr += 4;
                int zeroArrayLen = *(unsigned int *)(ptr);
                ptr += 25 + zeroArrayLen;
                //unit type id to train or index of queue to remove
                lastAction.data1 = *(unsigned int *)(ptr);
                lastAction.data2 = *(unsigned int *)(ptr+4);    //1 = delete from queue
                ptr += 13;
                updateGameRecord(0x2);
            }
            break;

        case 3: //build
            {
                data.gameActions.push_back();
                Action &lastAction = data.gameActions.back();
                fillCommonRecord(*(int *)ptr, &ptr);
                unsigned int immidiate_position = *(unsigned int *)(ptr);
                ptr += 4;
                //sometimes in ai building actions there is a list immidiate_position positions here.
                //I don't know the purpose of this yet
                ptr += immidiate_position * 12;
                int zeroArrayLen = *(unsigned int *)(ptr);
                ptr += 25 + zeroArrayLen;
                lastAction.data1 = *(unsigned int *)(ptr);  //building type id
                lastAction.float_data.x = *(float *)(ptr+4);    //coords
                lastAction.float_data.y = *(float *)(ptr+8);
                lastAction.float_data.z = *(float *)(ptr+12);
                ptr += 37;
                updateGameRecord(0x3);
            }
            break;

        case 4: //set rally point
            {
                data.gameActions.push_back();
                Action &lastAction = data.gameActions.back();
                fillCommonRecord(*(int *)ptr, &ptr);
                unsigned int immidiate_position = *(unsigned int *)(ptr);
                if(immidiate_position == 1){
                    lastAction.float_data.x = *(float *)(ptr+4);
                    lastAction.float_data.y = *(float *)(ptr+8);
                    lastAction.float_data.z = *(float *)(ptr+12);
                    lastAction.targetEntity = 0xffffffff;
                    ptr += 16;
                    int zeroArrayLen = *(unsigned int *)(ptr);
                    ptr += 46 + zeroArrayLen;
                }else{
                    ptr += 4;
                    int zeroArrayLen = *(unsigned int *)(ptr);
                    ptr += 25 + zeroArrayLen;
                    lastAction.float_data.x = *(float *)(ptr);
                    lastAction.float_data.y = *(float *)(ptr+4);
                    lastAction.float_data.z = *(float *)(ptr+8);
                    ptr += 16;
                    lastAction.targetEntity = *(unsigned int *)ptr;
                    ptr += 5;
                }
                updateGameRecord(0x4);
            }
            break;

        case 7: //delete entity
            {
                data.gameActions.push_back();
                fillCommonRecord(*(int *)ptr, &ptr);
                ptr += 4;
                int zeroArrayLen = *(unsigned int *)(ptr);
                ptr += 25 + zeroArrayLen;
                ptr += 2;
                updateGameRecord(0x7);
            }
            break;

        case 0xa:   //stop unit
            {
                data.gameActions.push_back();
                Action &lastAction = data.gameActions.back();
                int i = 0;
                //the player id is in a different position than normal
                lastAction.action = *(int *)ptr;
                ptr += 25;
                lastAction.player = *(int *)ptr;
                ptr += 8;
                int entityCount = *(unsigned int *)(ptr);
                lastAction.entityCount = entityCount;
                lastAction.entities = (unsigned int*)malloc(entityCount * sizeof(unsigned int));
                ptr += 4;
                for(i = 0; i < entityCount; i++){
                    lastAction.entities[i] = *(unsigned int *)(ptr);
                    ptr += 4;
                }
                ptr += 4;
                int zeroArrayLen = *(unsigned int *)(ptr);
                ptr += 25 + zeroArrayLen;
                ptr++;
                updateGameRecord(0xa);
            }
            break;

        case 0x10:  //use god power
            {
                data.gameActions.push_back();
                Action &lastAction = data.gameActions.back();
                fillCommonRecord(*(int *)ptr, &ptr);
                unsigned int immidiate_position = *(unsigned int *)(ptr);
                if(immidiate_position == 1){
                    lastAction.float_data.x = *(float *)(ptr+4);
                    lastAction.float_data.y = *(float *)(ptr+8);
                    lastAction.float_data.z = *(float *)(ptr+12);
                    ptr += 16;
                    int zeroArrayLen = *(unsigned int *)(ptr);
                    ptr += 25 + zeroArrayLen;
                    lastAction.data1 = *(unsigned int *)(ptr);  //god power id
                    ptr += 28;
                    lastAction.targetEntity = *(unsigned int *)(ptr);   //target entity id
                    ptr += 5;
                }else{
                    ptr += 4;
                    int zeroArrayLen = *(unsigned int *)(ptr);
                    ptr += 25 + zeroArrayLen;
                    lastAction.data1 = *(unsigned int *)(ptr);  //god power id
                    lastAction.float_data.x = *(float *)(ptr+4);
                    lastAction.float_data.y = *(float *)(ptr+8);
                    lastAction.float_data.z = *(float *)(ptr+12);
                    ptr += 28;
                    lastAction.targetEntity = *(unsigned int *)ptr;  //target entity id
                    ptr += 5;
                }
                updateGameRecord(0x10);
            }
            break;

        case 0x11:  //market action
            {
                data.gameActions.push_back();
                Action &lastAction = data.gameActions.back();
                lastAction.action = *(int *)ptr;
                lastAction.player = *(int *)(ptr + 5);
                ptr += 33;
                ptr += 12;
                int zeroArrayLen = *(unsigned int *)(ptr);
                ptr += 25 + zeroArrayLen;
                lastAction.data1 = *(unsigned int *)(ptr);  //resources type (0 = gold, 1 = wood, 2 = food)
                ptr += 8;
                lastAction.float_data.x = *(float *)(ptr);      //amount
                ptr += 5;
                updateGameRecord(0x11);
            }
            break;

        case 0x12:      //ungarrison
            {
                data.gameActions.push_back();
                Action &lastAction = data.gameActions.back();
                fillCommonRecord(*(int *)ptr, &ptr);
                ptr += (*(unsigned int *)ptr) * 12 + 4;
                int zeroArrayLen = *(unsigned int *)(ptr);
                ptr += 6;
                //ungarrison subject (0 = command units to ungarrison from a building, 1 = command building to ungarrison all units)
                lastAction.data1 = *(unsigned short *)(ptr);
                ptr += 19 + zeroArrayLen;
                //if ungarrison subject == 0, this is the id of the building to ungarrison from
                lastAction.data2 = *(unsigned int *)(ptr);
                ptr += 5;
                updateGameRecord(0x12);
            }
            break;

        case 0x14:      //resign
            {
                data.gameActions.push_back();
                Action &lastAction = data.gameActions.back();
                lastAction.action = *(int *)ptr;
                ptr += 41;
                int zeroArrayLen = *(unsigned int *)(ptr);
                ptr += 25 + zeroArrayLen;
                lastAction.player = *(unsigned int *)(ptr);  //id of the player that resign
                ptr += 13;
                updateGameRecord(0x14);
            }
            break;

        case 0x16:  //garrison
            {
                data.gameActions.push_back();
                Action &lastAction = data.gameActions.back();
                fillCommonRecord(*(int *)ptr, &ptr);
                ptr += 4;
                int zeroArrayLen = *(unsigned int *)(ptr);
                ptr += 25 + zeroArrayLen;
                lastAction.targetEntity = *(unsigned int *)(ptr);  //id of the building to garrison to
                ptr += 5;
                updateGameRecord(0x16);
            }
            break;

        case 0x17:  //tribute
            {
                data.gameActions.push_back();
                Action &lastAction = data.gameActions.back();
                fillCommonRecord(*(int *)ptr, &ptr);
                int zeroArrayLen = *(unsigned int *)(ptr);
                ptr += 25 + zeroArrayLen;
                lastAction.data1 = *(unsigned int *)(ptr);  //resourse type (0 = gold, 1 = wood, 2 = food)
                lastAction.data2 = *(unsigned int *)(ptr + 4);  //player receiving the resources
                lastAction.float_data.x = *(float *)(ptr + 8);  //amount
                lastAction.float_data.y = *(float *)(ptr + 12);  //taxes
                ptr += 17;
                updateGameRecord(0x17);
            }
            break;
        case 0x1c:      //convert
            {
                data.gameActions.push_back();
                Action &lastAction = data.gameActions.back();
                fillCommonRecord(*(int *)ptr, &ptr);
                ptr += 4;
                int zeroArrayLen = *(unsigned int *)(ptr);
                ptr += 25 + zeroArrayLen;
                lastAction.data1 = *(unsigned int *)ptr;    //type id to convert to
                ptr += 4;
                lastAction.data2 = *ptr;
                ptr += 2;
                updateGameRecord(0x1c);
            }
            break;

        case 0x21:  //set stance
            {
                data.gameActions.push_back();
                Action &lastAction = data.gameActions.back();
                fillCommonRecord(*(int *)ptr, &ptr);
                ptr += 4;
                int zeroArrayLen = *(unsigned int *)(ptr);
                ptr += 25 + zeroArrayLen;
                lastAction.data1 = *ptr;    //stance id (0 = aggressive, 1 = defensive, 2 = stand ground)
                ptr += 2;
                updateGameRecord(0x21);
            }
            break;

        case 0x2a:  //town bell
            {
                data.gameActions.push_back();
                fillCommonRecord(*(int *)ptr, &ptr);
                ptr += 4;
                int zeroArrayLen = *(unsigned int *)(ptr);
                ptr += 25 + zeroArrayLen;
                ptr++;
                updateGameRecord(0x2a);
            }
            break;

        case 0x2e:  //repair
            {
                data.gameActions.push_back();
                Action &lastAction = data.gameActions.back();
                fillCommonRecord(*(int *)ptr, &ptr);
                ptr += 4;
                int zeroArrayLen = *(unsigned int *)(ptr);
                ptr += 25 + zeroArrayLen;
                lastAction.targetEntity = *(unsigned int *)(ptr);
                ptr += 5;
                updateGameRecord(0x2e);
            }
            break;

        case 0x2f:  //empower
            {
                data.gameActions.push_back();
                Action &lastAction = data.gameActions.back();
                fillCommonRecord(*(int *)ptr, &ptr);
                ptr += 4;
                int zeroArrayLen = *(unsigned int *)(ptr);
                ptr += 25 + zeroArrayLen;
                lastAction.targetEntity = *(unsigned int *)(ptr);
                ptr += 5;
                updateGameRecord(0x2f);
            }
            break;

        case 0x33:      //set formation
            {
                data.gameActions.push_back();
                Action &lastAction = data.gameActions.back();
                fillCommonRecord(*(int *)ptr, &ptr);
                ptr += 4;
                int zeroArrayLen = *(unsigned int *)(ptr);
                ptr += 25 + zeroArrayLen;
                //formation id (6 = line formation, 2 = box formation, 5 = mixed formation, 3 = spread formation)
                lastAction.data1 = *ptr;
                ptr += 2;
                updateGameRecord(0x33);
            }
            break;

        case 0x34:  //cheat
            {
                data.gameActions.push_back();
                Action &lastAction = data.gameActions.back();
                lastAction.action = *(int *)ptr;
                lastAction.player = *(int *)(ptr + 5);
                ptr += 45;
                int zeroArrayLen = *(unsigned int *)(ptr);
                ptr += 25 + zeroArrayLen;
                lastAction.data1 = *(unsigned int *)ptr;
                lastAction.data2 = *(unsigned int *)(ptr+4);
                ptr += 9;
                updateGameRecord(0x34);
            }
            break;

        case 0x35:      //kronos time shift
            {
                data.gameActions.push_back();
                Action &lastAction = data.gameActions.back();
                fillCommonRecord(*(int *)ptr, &ptr);
                lastAction.float_data.x = *(float *)(ptr+4);
                lastAction.float_data.y = *(float *)(ptr+8);
                lastAction.float_data.z = *(float *)(ptr+12);
                ptr += 16;
                int zeroArrayLen = *(unsigned int *)(ptr);
                ptr += 25 + zeroArrayLen;
                ptr += 25;
                updateGameRecord(0x35);
            }
            break;
        case 0x36:      //continuous training on/off
            {
                data.gameActions.push_back();
                fillCommonRecord(*(int *)ptr, &ptr);
                ptr += 4;
                int zeroArrayLen = *(unsigned int *)(ptr);
                ptr += 25 + zeroArrayLen;
                ptr++;
                updateGameRecord(0x36);
            }
            break;
        default:
            #ifdef UNKNOWN_RECORD_DEBUG
                recordUnknownEvent(headerCode, *ptr);
            #endif // DEBUG
            return ptr - starting_ptr;
            break;
        }

    }

    return ptr - starting_ptr;
}

int RecParser::findNextValidRecord(unsigned char *ptr){
    int i = 0;
    for(ptr; ptr < data.fileMem + data.fileLen; ptr++){
        if((*ptr == 0x21 && *(ptr+1) != 0 && *(ptr+2) != 0 && *(ptr+3) == 1) ||
            ((*ptr == 0x81 || *ptr == 0xb || *ptr == 0xa1 || *ptr == 0x3 || *ptr == 0x9) && *(ptr+1) != 0 && *(ptr+2) != 0 && *(ptr+3) != 0) ||
            (*ptr == 1 && *(ptr+1) != 0 && *(ptr + 2) == 0 && *(ptr+3) != 0)){
            //it probably found the next record
            return i;
        }
        ++i;
    }
    return i;
}

int RecParser::parseHeader(unsigned char *ptr){
    int i, count;

    switch(*ptr){
    case 0x1:   //waste of time
        data.gameTicks++;
        return 3;
        break;

    case 0x3:
        {
            float t = *(float *)(ptr+1);
            if(t > 0.0001 && t < 1000000){  //is a float
                data.totalEventCount++;
                return 7;
            }
            return 1 + findNextValidRecord(ptr+1);

        }
        break;

    case 0x9:
        {
            float t = *(float *)(ptr+1);
            if(t > 0.0001 && t < 1000000){  //is a float
                data.totalEventCount++;
                return 7;
            }
            return 1 + findNextValidRecord(ptr+1);
        }
        break;

    case 0xb:   //viewlock
        {
            data.gameActions.push_back();
            Action &lastAction = data.gameActions.back();
            lastAction.action = 0x50;
            lastAction.float_data.x = *(float *)(ptr + 1);
            lastAction.float_data.y = *(float *)(ptr + 5);
            data.totalEventCount++;
            return 11;
        }
        break;

    case 0x21:
        {
            int records = *(ptr+2);
            int totalLen = 4;
            for(i = 0; i < records; i++){
                int ret = parseRecord(0x21, ptr + totalLen);
                if(ret == 0) return totalLen + findNextValidRecord(ptr+totalLen);
                totalLen += ret;
            }
            data.totalEventCount++;
            return totalLen;
        }
        break;

    case 0x81:  //select
        {
            data.gameActions.push_back();
            Action &lastAction = data.gameActions.back();
            lastAction.action = 0x51;
            count = *(unsigned char *)(ptr + 2);
            lastAction.entityCount = count;
            lastAction.entities = (unsigned int*)malloc(count * sizeof(unsigned int));
            ptr += 3;
            for(i = 0; i < count; i++){
                lastAction.entities[i] = *(unsigned int *)(ptr);
                ptr += 4;
                //data.totalEventCount++;
            }
            data.totalEventCount++;
            updateGameRecord(0x51);
            return 4 + 4*count;
        }
        break;

    case 0xa1:      //add units to group
        {
            int records = *(ptr+2);
            int totalLen = 4;
            for(i = 0; i < records; i++){
                int ret = parseRecord(0xa1, ptr + totalLen);
                if(ret == 0) return totalLen + findNextValidRecord(ptr+totalLen);
                totalLen += ret;
            }
            data.totalEventCount++;
            updateGameRecord(0x52);
            return totalLen;
        }
        break;

    default:
        #ifdef UNKNOWN_RECORD_DEBUG
        recordUnknownEvent(*ptr, -1);
        #endif // DEBUG
        //search the next valid record
        return findNextValidRecord(ptr);
        break;

    }
    return 0;
}


void RecParser::load_game_actions(){

    data.gameTicks = 0;

    unsigned char *ptr = data.game_records_mem;
    do{
        ptr += parseHeader(ptr);
    }while(ptr < data.fileMem + data.fileLen);
}

//finds out if there is a winning team
int RecParser::isThereAWinner(){
    int i, j;
    int resigned = 0;
    for(i = 0; i < data.teamCount; i++){
        int teamResign = 1;
        for(j = 0; j < data.teams[i]->playerCount; j++){
            teamResign &= data.playersSummary[data.teams[i]->players[j]->playerId].resign;
        }
        if(teamResign == 1) resigned++;
    }
    if(data.teamCount - resigned == 1){
        return 1;
    }
    return 0;
}

void RecParser::create_gameInfo_file(const char* name){
    char nameBuffer[256];


    sprintf(nameBuffer, "%s_%s", name, "gameSettings.xml");
    std::ofstream file(nameBuffer, std::ios::binary);

    for(int i = 0; i < data.gameInfo_xml_len/2; i++){
        unsigned short val = ((unsigned short*)data.gameInfo_xml)[i];
        if(val == 0 || val == 0x400){
            continue;
        }
        file.write((char*)&val, 2);
    }
    //fwrite(data.gameInfo_xml, data.scripts_len, 1, f);
    file.close();
}

void RecParser::create_scripts_file(const char* name){
    char nameBuffer[256];

    sprintf(nameBuffer, "%s_%s", name, "scripts.txt");
    std::ofstream file(nameBuffer, std::ios::binary);

    for(int i = 0; i < data.scripts_len; i++){
        unsigned char val = data.scripts[i];
        if(val == 0 || val == 0x4){
            continue;
        }
        file.write((char*)&val, 1);
    }
    file.close();
}

void RecParser::print_results(const char* name){
    int i, j, k;
    char nameBuffer[256];

    create_gameInfo_file(name);
    create_scripts_file(name);

    int winner = isThereAWinner();

    sprintf(nameBuffer, "%s_%s", name, "gameReport.txt");
    std::ofstream file(nameBuffer);

    file << " Total game events: " << data.realGameEventCount;
    file << "\n Recognized game events: " << data.totalEventCount + data.gameTicks << " (" << ((float)((data.totalEventCount + data.gameTicks)*100)/data.realGameEventCount) << " %)";

    #ifdef UNKNOWN_RECORD_DEBUG
    file << "\n\n#### UNKNOWN EVENTS ####";
    for(i = 0; i < unknownRecordCount; i++){
        file << "\n HC: " << unknownArray[i].headerCode << ", AC: " << unknownArray[i].actionCode << ", Count: " << unknownArray[i].count;
    }
    #endif // UNKNOWN_RECORD_DEBUG

    file << "\n\n######## Teams ########";
    for(i = 0; i < data.teamCount; i++){
        file << "\n " << data.teams[i]->teamId << "] " << data.teams[i]->name;
        for(j = 0; j < data.teams[i]->playerCount; j++){
            file << "\n\t " << data.teams[i]->players[j]->playerId << ") ";
            for(k = 0; k <= data.teams[i]->players[j]->nameLen; k++){
                nameBuffer[k] =  data.teams[i]->players[j]->name[k];
            }
            file << nameBuffer;
            file << " (" << (data.teams[i]->players[j]->godId < 12 ? majorGodNames[data.teams[i]->players[j]->godId] : "Invalid") << ")";
            if(winner && !data.playersSummary[data.teams[i]->players[j]->playerId].resign){
                file << "    [Winner]";
            }
        }
    }

    for(i = 1; i < data.playerCount; i++){
        file << "\n\n";
        printPlayerSummary(i, file);
    }
    file.close();

}
