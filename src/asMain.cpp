#include <iostream>
#include <fstream>
#include <regex>
#include "/home/ss/Desktop/Projekat/inc/assembler.hpp"
using namespace std;

int main(int argc, const char *argv[]) {
    string inputFile;
    string outputFile;
    if (argc <= 1) {
      cout << "Need arguments!" << endl;
      return -1;
    }

    // cout << argv[1] << endl;
    //cout << argv[2] << endl;
    // cout << argc << endl;
    //cout << argv[3] << endl;
    // cout << ((string)argv[1] == "-o") << endl;
    if ((string)argv[1] == "-o") {
        inputFile = argv[3];
        outputFile = argv[2];
    }
    else {
        cout << "Output file doesn't exists!" << endl;
        return -2;
    }

    Assembler as(inputFile, outputFile);

    if (!as.compile()) return -3;
    
    return 0;
}