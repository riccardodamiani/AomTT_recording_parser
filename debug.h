
#ifndef DEBUG_H
#define DEBUG_H

#define UNKNOWN_ARRAY_LEN 1000
struct unknownRecord{
    int headerCode;
    int actionCode;
    int count;
}unknownArray[UNKNOWN_ARRAY_LEN];
int unknownRecordCount;

void recordUnknownEvent(int headerCode, int actionCode){
    int i;
    if(unknownRecordCount >= UNKNOWN_ARRAY_LEN){
        printf("\n Warning: Exceeded unknown action array length");
        return;
    }
    for(i = 0; i < unknownRecordCount; i++){
        if(headerCode == unknownArray[i].headerCode &&
           actionCode == unknownArray[i].actionCode){
            unknownArray[i].count++;
            return;
        }
    }

    unknownArray[unknownRecordCount].headerCode = headerCode;
    unknownArray[unknownRecordCount].actionCode = actionCode;
    unknownArray[unknownRecordCount].count = 1;
    unknownRecordCount++;
}

#endif // DEBUG_H
