#ifndef ASSEMBLER_H
#define ASSEMBLER_H
#include <string>
#include <map>
#include <vector>
using namespace std;

// equ ne radi za izraze
// ascii ne radi sa zagradama
// check places of sections?

class Assembler {
private:

  int locationCounter;
  string inputFilePath;
  string outputFilePath;
  int currLineNumber;
  string currSection;

  struct SectionTable {
    int sectionId;
    int size;
    vector<int> offset;
    vector<char> info;
    string sectionName;
  };
  
  struct SymbolTable {
    int symbolId;
    int value;
    bool isDefined;
    bool isGlobal;
    bool isExtern;
    string section;
    string symbolName;
  };
  
  struct RelocationTable {
    int offset;
    bool isInfo;
    string sectionName;
    string symbolName;
    string type;
    int addend;
  };

  struct Flink {
    char sign;  // either + or -
    int address;
    string sectionName;
    string symbolName;
  };
  
  bool pass();
  map<int, string> errorMessages;
  map<string, SymbolTable> symbolTable;
  map<string, SectionTable> sectionTable;
  vector<RelocationTable> relocationTable;
  vector<Flink> flink;

  bool processLabel(string label);
  void processGlobalSymbol(string symbol);
  bool processExternSymbol(string symbol);
  bool processSection(string section);
  bool processWordDirective(string word);
  bool processSkipDirective(string value);
  bool processAsciiDirective(string str);
  bool processEquDirective(string name, string value);



  bool processInstruction(string instr);

  int processAbsoluteAddressingSymbol(string symbol);
  int processPCRelativeAddressingSymbol(string symbol);

  int castLiteralToDecimalValue(string literal);
  string decToHex(int decValue);

  void makeTextFile();
  void makeBinaryFile();

public:
  Assembler(string, string);
  bool compile();
};

#endif // ASSEMBLER_H