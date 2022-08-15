#include <iostream>
// #include <vector>
// #include <regex>
#include <string>
#include <fstream>
#include <iomanip>
#include <map>

#include "/home/ss/Desktop/Projekat/inc/emulator.hpp"

using namespace std;

int main(int argc, const char *argv[]) {
    if (argc != 2) cout << "Emulator warning: Only first file will be considered!" << endl;
    //cout << argc << ":" << argv[0] << "#" << argv[1] << endl;
    Emulator emulator(argv[1]);
    // cout << "POCETAK EMULACIJE!" << endl;
    if (!emulator.readBinaryFileFromLinker()) return -1;
    // cout << "PROCITAO FAJL!" << endl;
    if (!emulator.emulate()) return -1;
    // cout << "KRAJ EMULACIJE!" << endl;
    return 0;
}