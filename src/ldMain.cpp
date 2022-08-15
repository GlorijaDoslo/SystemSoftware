#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <regex>
#include <iomanip>
#include "/home/ss/Desktop/Projekat/inc/linker.hpp"

using namespace std;

int main(int argc, const char *argv[]) {

  bool outputFile = false;
  string outFileName = "linker.o";
  bool placeFile = false;
  regex place("^-place=([a-zA-Z_][a-zA-Z_0-9]*)@(0[xX][0-9a-fA-F]+)$");
  smatch addrOfSection;
  map<string, int> placeSectionAddr;
  bool isHex = false;
  bool isRelocateable = false;
  vector<string> linkingFiles;
  for (int i = 1; i < argc; i++) {
    string arg = argv[i];
    if (arg == "-o") outputFile = true; 
    else if (arg == "-hex") isHex = true;
    else if (arg == "-relocateable") isRelocateable = true;
    else if (regex_search(arg, addrOfSection, place)) {
      placeFile = true;
      string section = addrOfSection.str(1);
      int address = stoi(addrOfSection.str(2), nullptr, 16);
      placeSectionAddr[section] = address;
    }
    else if (outputFile) {
      outFileName = (string)argv[i];
      outputFile = false;
    }
    else linkingFiles.push_back((string)argv[i]);
        
  }

  if (isHex == true && isRelocateable == true) {
      cout << "Linker error: Only one option is permitted: -hex OR -relocateable!" << endl;
      return -1;
  }
  if (isHex == false && isRelocateable == false) {
      cout << "Linker error: Need -hex or -relocateable option!" << endl;
      return -1;
  }

  Linker linker(outFileName, linkingFiles, isRelocateable, placeSectionAddr);
  linker.link();

  return 0;
}