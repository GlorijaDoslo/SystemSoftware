#ifndef LINKER_H
#define LINKER_H
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace std;

class Linker {
private:
  // identical as in asembler
  struct SymbolTable {
    int symbolId;
    int value;
    bool isDefined;
    bool isGlobal;
    bool isExtern;
    string section;
    string symbolName;
  };

  // everything is the same as in asembler except for virtual memory address field
  // which stores virtual address where section is placed in memory
  struct SectionTable {
    int sectionId;
    int size;
    vector<int> offset;
    vector<char> info;
    string sectionName;
    int virtualMemAddr;
  };

  struct SectionData {
    string sectionName;
    int size;
    string filename;
    int currAddrMergedSection;
  };
  // same as in asembler except for filename which represent in which file is particular
  // relocation data
  struct RelocationTable {
    int offset;
    bool isInfo;
    string sectionName;
    string symbolName;
    string type;
    string filename;
  };

  // first key is filename and second is symbol or section name
  map<string, map<string, SymbolTable>> fileSymbolTable;
  map<string, map<string, SectionTable>> fileSectionTable;
  map<string, vector<RelocationTable>> fileRelocationTable;

  // after merging all symbols, sections and relocation data
  map<string, SymbolTable> mergedSymbolTable;
  map<string, SectionTable> mergedSectionTable;
  vector<RelocationTable> mergedRelocationTable;


  string outputFileName;
  vector<string> linkingFiles;

  map<string, map<string, SectionData>> sectionData;

  // this is option relocateable
  bool isRelocateable;
  // read this from input and place in this map
  map<string, int> placeSectionAddr;

  bool search(string name, string option);
  bool moveSectionsToVirtualAddress();
  bool relocationDataHex();
  bool relocationDataRelocateable();
 
  // used in link method
  bool readBinaryFilesFromAsembler();
  bool makeMergedSymbolTable();
  bool makeMergedSections();
  bool makeMergedSectionData();
  void makeMergedRelocations();

  void makeTextFile();
  void makeBinaryFile();


public:
  static int START_STACK_ADDRESS;

  Linker(string, vector<string>, bool, map<string, int>);
  bool link();

};
#endif //LINKER_H