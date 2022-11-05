
#include "recordingParser.h"

#include <algorithm>
#include <string>

int main(int argc, char *argv[]){
    RecParser parser;
    std::string filename;

    if(argc > 1){
        parser.ParseFile(argv[1]);
        return 0;
    }
    printf("\n Enter .rcx file: ");
    std::getline (std::cin, filename);
    filename.erase(std::remove(filename.begin(), filename.end(), '"'), filename.end()); //remove " from string

    parser.ParseFile(filename.c_str());

    return 0;
}
