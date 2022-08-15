#include <iostream>
#include <fstream>
#include <iomanip>
#include <regex>
#include "/home/ss/Desktop/Projekat/inc/linker.hpp"

using namespace std;

int Linker::START_STACK_ADDRESS = 0xFF00;

// constructor
Linker::Linker(string outFileNam, vector<string> linkFiles, bool isReloc, map<string, int> mapSecAddr) {
  this->outputFileName = outFileNam;
  this->linkingFiles = linkFiles;
  this->isRelocateable = isReloc;
  this->placeSectionAddr = mapSecAddr;
}


// options -> placeSectionAddr, mergedSymbolTable, mergedSectionTable
bool Linker::search(string name, string option) {
  if (option == "placeSectionAddr") {
    for (map<string, int>::iterator place = placeSectionAddr.begin(); place != placeSectionAddr.end();
      place++) {
      if (name == place->first) {
        return true;
      }
    } // end of search
  }
  else if (option == "mergedSymbolTable") {
    for (map<string, SymbolTable>::iterator symbol = mergedSymbolTable.begin();
      symbol != mergedSymbolTable.end(); symbol++) {
      if (name == symbol->first) {
        return true;
      }
    }
  }
    else if (option == "mergedSectionTable") {
    for (map<string, SectionTable>::iterator sec = mergedSectionTable.begin();
      sec != mergedSectionTable.end(); sec++) {
      if (name == sec->first) {
        return true;
      }
    }
  }
  return false;
}

// very important method
bool Linker::moveSectionsToVirtualAddress() {
  int nextAddressNonPlaceSections = 0;
  // first the place options
  for (map<string, int>::iterator place = placeSectionAddr.begin(); place != placeSectionAddr.end(); place++) {
    //set the VA of section which is mentioned in place option
    for (map<string, SectionTable>::iterator it = mergedSectionTable.begin();
      it != mergedSectionTable.end(); it++) {
      if (place->first == it->first) {
        it->second.virtualMemAddr = place->second;
        break;
      }
    }
    // and data of that section
    for (map<string, SectionData>::iterator file = sectionData[place->first].begin();
      file != sectionData[place->first].end(); file++) {
      file->second.currAddrMergedSection += place->second;
      if (file->second.currAddrMergedSection + file->second.size > nextAddressNonPlaceSections)
        nextAddressNonPlaceSections = file->second.currAddrMergedSection + file->second.size;
    }

  } // place for loop

  // check for overlaps between two sections when using place option
  for (map<string, SectionTable>::iterator it1 = mergedSectionTable.begin(); 
    it1 != mergedSectionTable.end(); it1++) {
    if (it1->first == "UND" || it1->first == "ABS") continue; // skip these sections
    if (!search(it1->first, "placeSectionAddr")) continue;
    // start VA of section 
    int startAdd = it1->second.virtualMemAddr;
    // end VA of section
    int endAdd = it1->second.virtualMemAddr + it1->second.size;

    if (endAdd > START_STACK_ADDRESS) {
      cout << "Linker error: Section " + it1->first + " tried to overwrite reserved places of memory!" << endl;
      return false;
    }

    for (map<string, SectionTable>::iterator it2 = mergedSectionTable.begin();
      it2 != mergedSectionTable.end(); it2++) {
      if (it1->first == it2->first) continue; // this is the same section
      if (it2->first == "UND" || it2->first == "ABS") continue; // skip these sections
      if (!search(it2->first, "placeSectionAddr")) continue;
      int startAdd2 = it2->second.virtualMemAddr;
      int endAdd2 = it2->second.virtualMemAddr + it2->second.size;
      if (max(startAdd, startAdd2) < min(endAdd, endAdd2)) {
        cout << "Linker error: Sections " + it1->first + " and " + it2->first 
          + " have overlaped in memory!" << endl;
        return false;
      }

    }

  } // end of section table for loop

  // // moving sections to virtual memory
  // for (map<string, SectionTable>::iterator it = mergedSectionTable.begin();
  //   it != mergedSectionTable.end(); it++) {
  //   if (it->first == "UND" || it->first == "ABS") continue;


  //   if (!search(it->first, "placeSectionAddr")) {
  //     it->second.virtualMemAddr = nextAddressNonPlaceSections;

  //     for (map<string, SectionData>::iterator it2 = sectionData[it->first].begin();
  //       it2 != sectionData[it->first].end(); it2++) {
  //       it2->second.currAddrMergedSection += nextAddressNonPlaceSections;
  //     }
  //     nextAddressNonPlaceSections += it->second.size;
  //   }
  //   if (nextAddressNonPlaceSections > START_STACK_ADDRESS) {
  //     cout << "Linker error: Section " + it->first + " tried to overwrite reserved places of memory!" << endl;
  //     return false;
  //   }

  // } // end of mergedSectionTable for loop

  // moving sections to virtual memory
  vector<pair<string, SectionTable>> vector(mergedSectionTable.begin(), mergedSectionTable.end());

  // Sort the vector according to the word count in ascending order.
  sort(vector.begin(), vector.end(), 
           []( const auto & lhs, const auto & rhs ) 
           { return lhs.second.sectionId < rhs.second.sectionId; } );

  // for (auto& it: vector) {
  //   cout << it.second.sectionName << endl;
  // }
  //for (auto& it: vector) {
  //   for (map<string, SectionTable>::iterator it = mergedSectionTable.begin(); 
  //   it != mergedSectionTable.end(); it++ ) {
  //     cout << it->first << " " << it->second.size << endl;
  //   if (it->first == "UND" || it->first == "ABS") continue;


  //   if (!search(it->first, "placeSectionAddr")) {
  //     it->second.virtualMemAddr = nextAddressNonPlaceSections;
  //     cout << "Address " << hex << nextAddressNonPlaceSections << endl;
  //     cout << "VA " << hex << it->second.virtualMemAddr << endl;
  //     for (map<string, SectionData>::iterator it2 = sectionData[it->first].begin();
  //       it2 != sectionData[it->first].end(); it2++) {
  //       it2->second.currAddrMergedSection += nextAddressNonPlaceSections;
  //     }
  //     nextAddressNonPlaceSections += it->second.size;
  //   }
  //   if (nextAddressNonPlaceSections > START_STACK_ADDRESS) {
  //     cout << "Linker error: Section " + it->first + " tried to overwrite reserved places of memory!" 
  //       << endl;
  //     return false;
  //   }

  // } // end of mergedSectionTable for loop
  
  for (auto& it: vector) {
    if (search(it.first, "placeSectionAddr")) continue;
  // for (map<string, SectionTable>::iterator it = mergedSectionTable.begin(); 
  //  it != mergedSectionTable.end(); it++ ) {
      // cout << it.first << " " << it.second.size << endl;
    if (it.first == "UND" || it.first == "ABS") continue;


    if (!search(it.first, "placeSectionAddr")) {
      it.second.virtualMemAddr = nextAddressNonPlaceSections;
      // cout << "Address " << hex << nextAddressNonPlaceSections << endl;
      // cout << "VA " << hex << it.second.virtualMemAddr << endl;
      for (map<string, SectionData>::iterator it2 = sectionData[it.first].begin();
        it2 != sectionData[it.first].end(); it2++) {
        it2->second.currAddrMergedSection += nextAddressNonPlaceSections;
      }
      nextAddressNonPlaceSections += it.second.size;
    }
    if (nextAddressNonPlaceSections > START_STACK_ADDRESS) {
      cout << "Linker error: Section " + it.first + " tried to overwrite reserved places of memory!" 
        << endl;
      return false;
    }

  } // end of mergedSectionTable for loop
  for (auto& i: vector) {
    mergedSectionTable[i.first].virtualMemAddr = i.second.virtualMemAddr;
  }
  return true;
}

// option hex
bool Linker::relocationDataHex() {
  for (RelocationTable relData: mergedRelocationTable) {
    // cout << "REL DATA " << hex << (int)relData.offset << " " << relData.type << " " << relData.symbolName << endl;
    
    int val = 0;
    // if it is in section table then it's a section
    if (search(relData.symbolName, "mergedSectionTable")) {  
      // cout << "ITS A SECTION " << endl;
      bool found = false;
      for (map<string, map<string, SectionData>>::iterator it = sectionData.begin(); 
        it != sectionData.end(); it++) {

        if (relData.symbolName == it->first) {
          for (map<string, SectionData>::iterator it2 = it->second.begin(); it2 != it->second.end(); it2++) {
            if (relData.filename == it2->first) {
              // cout << "INSIDE DOUBLE IF " << hex << (int)it2->second.currAddrMergedSection << endl;
              val = it2->second.currAddrMergedSection;
              // cout << "VALUE IN DOUBLE IF " << hex << (int)val << endl;
              found = true;
              break;
            }
          }
        }
        if (found) break;
      }
    }
    else {  // if it's not in section table then it is a symbol
      // cout << "ITS A SYMBOL" << endl;
      for (map<string, SymbolTable>::iterator it = mergedSymbolTable.begin(); it != mergedSymbolTable.end();
        it++) {
        if (relData.symbolName == it->first) {
          val = it->second.value;
          // cout << "VALUE IN ELSE_IF " << hex << (int)val << endl;
        }
      }
      
    } // end of else
    // cout << "GOT THE VALUE " << endl;
    int offInfo = 0;
    if (relData.type == "R_PC16") {
      
      offInfo = relData.offset;
      // cout << "ENTERED IN IF FOR PC " << hex << (int)offInfo << endl;
      for (map<string, SectionTable>::iterator it = mergedSectionTable.begin();
        it != mergedSectionTable.end(); it++) {

        if (relData.sectionName == it->first) {
          offInfo += it->second.virtualMemAddr;
          // cout << "ADDING SECTION" + it->first + "OFFSET " << hex << (int)offInfo << endl;
          break;
        }
      }
      if (!relData.isInfo) {
        offInfo += 1; // not sure
        // cout << "NOT INFO OFFSET DECREMENT " << hex << (int)offInfo << endl;
      }
      
    } // end of if 
    int lower = 0, higher = 0;

    if (relData.isInfo) {
      // cout << "IT IS DATA " << endl;
      for (map<string, SectionTable>::iterator it = mergedSectionTable.begin(); 
        it != mergedSectionTable.end(); it++) {

        if (relData.sectionName == it->first) {
          lower = it->second.info[relData.offset];
          higher = it->second.info[relData.offset + 1];
        }
      }
      // cout << "HIGHER " << hex << higher << endl;
      // cout << "LOWER " << hex << lower << endl;
      int v = (int)((higher << 8) + (0xff & lower));
      // cout << "V bez offInfo " << hex << v << endl;
      // cout << "OFFINFO " << hex << offInfo << endl;
      // cout << "VAL " << hex << val << endl;
      v = (val - offInfo);  // !!!!!! watch out here 
      // cout << "V posle offInfo " << hex << v << endl;
      // cout << "OFFSET " << relData.offset << endl;
      // cout << "VALUE V " << v << endl; 
      mergedSectionTable[relData.sectionName].info[relData.offset] = (0xff & v);
      mergedSectionTable[relData.sectionName].info[relData.offset + 1] = ((v >> 8) & 0xff);
      // cout << "INFO " << hex << (int)mergedSectionTable[relData.sectionName].info[relData.offset] << endl;
      // cout << "INFO " << hex << (int)mergedSectionTable[relData.sectionName].info[relData.offset + 1] << endl;
    }
    else {  // isn't data
    // cout << "IT ISNT DATA " << endl;
      for (map<string, SectionTable>::iterator it = mergedSectionTable.begin(); 
        it != mergedSectionTable.end(); it++) {

        if (relData.sectionName == it->first) {
          lower = it->second.info[relData.offset];
          higher = it->second.info[relData.offset - 1];
        }
      }
      // cout << "HIGHER " << hex << higher << endl;
      // cout << "LOWER " << hex << lower << endl;
      int v = (int)((higher << 8) + (0xff & lower));
      v = val - offInfo;  //!!!!!!!!!!!!!!!!!!!
      // cout << "OFFSET " << relData.offset << endl;

      mergedSectionTable[relData.sectionName].info[relData.offset] = (0xff & v);
      mergedSectionTable[relData.sectionName].info[relData.offset - 1] = ((v >> 8) & 0xff);
      // cout << "INFO " << hex << (int)mergedSectionTable[relData.sectionName].info[relData.offset] << endl;
      // cout << "INFO " << hex << (int)mergedSectionTable[relData.sectionName].info[relData.offset - 1] << endl;
    }
  }
  return true;
}

// option relocateable
bool Linker::relocationDataRelocateable() {
  for (RelocationTable relData: mergedRelocationTable) {
    int val = 0;
    if (search(relData.symbolName, "mergedSectionTable")) {
      bool found = false;
      for (map<string, map<string, SectionData>>::iterator it = sectionData.begin(); it != sectionData.end(); it++) {
        if (relData.symbolName == it->first) {
          for (map<string, SectionData>::iterator it2 = it->second.begin(); it2 != it->second.end(); it2++) {
            if (relData.filename == it2->first) {
              val = it2->second.currAddrMergedSection;
              found = true;
              break;
            }
          }
        }
        if (found) break;
      }
    }
    else continue;
    int lower = 0;
    int higher = 0;

    if (relData.isInfo) { // little endian lower byte lower address
      for (map<string, SectionTable>::iterator it = mergedSectionTable.begin(); 
        it != mergedSectionTable.end(); it++) {

        if (relData.sectionName == it->first) {
          lower = it->second.info[relData.offset];
          higher = it->second.info[relData.offset + 1];
        }
      }

      int v = (int)((0xff & lower) + (higher << 8));
      v += val;
      mergedSectionTable[relData.sectionName].info[relData.offset] = (0xff & v);
      mergedSectionTable[relData.sectionName].info[relData.offset + 1] = ((v >> 8) & 0xff);
    }
    else {  // big endian, check this!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      for (map<string, SectionTable>::iterator it = mergedSectionTable.begin(); 
        it != mergedSectionTable.end(); it++) {

        if (relData.sectionName == it->first) {
          lower = it->second.info[relData.offset];
          higher = it->second.info[relData.offset + 1];
        }
      }

      int v = (int)((0xff & lower) + (higher << 8));
      v += val;
      mergedSectionTable[relData.sectionName].info[relData.offset] = (0xff & v);
      mergedSectionTable[relData.sectionName].info[relData.offset + 1] = ((v >> 8) & 0xff);
    }
  }
  return true;
}

// read from binary file which linker got from asembler
bool Linker::readBinaryFilesFromAsembler() {
  // cout << "READ BINARY" << endl;
  for (string file : linkingFiles) {  // iterate through files
    ifstream inFile(file, ios::binary);
    if (inFile.fail()) {
      cout << "Linker error: File " + file + " is invalid!" << endl;
      return false;
    }
    // Symbol table
    map<string, SymbolTable> symTab;
    int symNum = 0;
    inFile.read((char*)&symNum, sizeof(symNum));
    for (int i = 0; i < symNum; i++) {
      SymbolTable symTable;
      string symName;
      int len;
      inFile.read((char*)(&len), sizeof(len));
      symName.resize(len);
      inFile.read((char*)symName.c_str(), len);

      inFile.read((char*)(&symTable.symbolId), sizeof(symTable.symbolId));
      inFile.read((char*)(&symTable.value), sizeof(symTable.value));
      inFile.read((char*)(&symTable.isDefined), sizeof(symTable.isDefined));
      inFile.read((char*)(&symTable.isGlobal), sizeof(symTable.isGlobal));
      inFile.read((char*)(&symTable.isExtern), sizeof(symTable.isExtern));

      len;
      inFile.read((char*)(&len), sizeof(len));
      symTable.section.resize(len);
      inFile.read((char*)symTable.section.c_str(), len);

      len;
      inFile.read((char*)(&len), sizeof(len));
      symTable.symbolName.resize(len);
      inFile.read((char*)symTable.symbolName.c_str(), len);

      symTab[symName] = symTable;
    }

    fileSymbolTable[file] = symTab;
    // cout << "READ SECTIONS" << endl;

    // Section table
    map<string, SectionTable> sTable;

    int secNum = 0;
    inFile.read((char*)&secNum, sizeof(secNum));
    for (int i = 0; i < secNum; i++) {
      struct SectionTable secTable;
      string secName;
      unsigned int len;
      inFile.read((char*)(&len), sizeof(len));
      secName.resize(len);
      inFile.read((char*)secName.c_str(), len);
      // section id
      inFile.read((char*)(&secTable.sectionId), sizeof(secTable.sectionId));
      // section size
      inFile.read((char*)(&secTable.size), sizeof(secTable.size));

      // offset
      int offNum;
      inFile.read((char*)(&offNum), sizeof(offNum));

      for (int j = 0; j < offNum; j++) {
        int o;
        inFile.read((char*)(&o), sizeof(o));
        secTable.offset.push_back(o);
      }
      // info
      int charNum;
      inFile.read((char*)(&charNum), sizeof(charNum));
      for (int j = 0; j < charNum; j++) {
        char c;
        inFile.read((char*)(&c), sizeof(c));
        secTable.info.push_back(c);
      }
      // section name
      len;
      inFile.read((char*)(&len), sizeof(len));
      secTable.sectionName.resize(len);
      inFile.read((char*)secTable.sectionName.c_str(), len);

      secTable.virtualMemAddr = 0;

      sTable[secName] = secTable;
    }

    fileSectionTable[file] = sTable;
    // cout << "READ RELOCATIONS" << endl;

    // Relocation table
    int relNum = 0;
    inFile.read((char*)&relNum, sizeof(relNum));

    vector<RelocationTable> relTable;
    for (int i = 0; i < relNum; i++) {
      RelocationTable relData;
      inFile.read((char*)(&relData.offset), sizeof(relData.offset));
      inFile.read((char*)(&relData.isInfo), sizeof(relData.isInfo));

      int len;
      inFile.read((char*)(&len), sizeof(len));
      relData.sectionName.resize(len);
      inFile.read((char*)relData.sectionName.c_str(), len);

      len;
      inFile.read((char*)(&len), sizeof(len));
      relData.symbolName.resize(len);
      inFile.read((char*)relData.symbolName.c_str(), len);

      len;
      inFile.read((char*)(&len), sizeof(len));
      relData.type.resize(len);
      inFile.read((char*)relData.type.c_str(), len);

      relTable.push_back(relData);
    }

    fileRelocationTable[file] = relTable;

    inFile.close();
  } // end of files for

  // cout << "POCETAK RELOCATION TABLE" << endl;
  // for (const auto& iter : fileRelocationTable["interrupts.o"]) {
  //   cout << iter.offset << " " << iter.symbolName << endl;
  // }

  return true;
}

// make one big symbol table
bool Linker::makeMergedSymbolTable() {

  int nextSymbolId = 0;
  // first add sections as symbols in symbol table
  for (map<string, SectionTable>::iterator it = mergedSectionTable.begin();
    it != mergedSectionTable.end(); it++) {
    
    SymbolTable newSymbol;
    if (it->second.sectionId > nextSymbolId) nextSymbolId = newSymbol.symbolId;
    newSymbol.symbolId = it->second.sectionId;
    newSymbol.value = it->second.virtualMemAddr;
    newSymbol.isDefined = true;
    newSymbol.isGlobal = false;
    newSymbol.isExtern = false;
    newSymbol.section = it->second.sectionName;
    newSymbol.symbolName = it->second.sectionName;
    mergedSymbolTable[it->second.sectionName] = newSymbol;
  }
  nextSymbolId++;
  map<string, SymbolTable> rememberExternSymbols;
  // now solve symbols
  for (string filename: linkingFiles) {
    for (map<string, SymbolTable>::iterator it = fileSymbolTable[filename].begin();
      it != fileSymbolTable[filename].end(); it++) {
        if (it->second.isExtern) {
          rememberExternSymbols[it->first] = it->second;
          continue;
        }
        if (it->second.section == it->second.symbolName) continue;

        if (search(it->first, "mergedSymbolTable")) {
          if (it->second.isGlobal && !mergedSymbolTable[it->first].isGlobal) {

            mergedSymbolTable[it->first].symbolName = mergedSymbolTable[it->first].symbolName + "Local";
            mergedSymbolTable[mergedSymbolTable[it->first].symbolName] = mergedSymbolTable[it->first];

            it->second.symbolId = nextSymbolId++;
          if (it->second.section != "ABS") 
            it->second.value += sectionData[it->second.section][filename].currAddrMergedSection;
          mergedSymbolTable[it->second.symbolName] = it->second;
          }
          else if (!it->second.isGlobal && mergedSymbolTable[it->first].isGlobal) {
            it->second.symbolId = nextSymbolId++;
          if (it->second.section != "ABS") 
            it->second.value += sectionData[it->second.section][filename].currAddrMergedSection;
            it->second.symbolName = it->second.symbolName + "Local";
            mergedSymbolTable[it->second.symbolName] = it->second;
          }
          else {
            cout << "Linker error: Multiple definition of symbol " + it->first + "!" << endl;
            return false;
          }
          
        }
        else {  // symbol is not in final symbol table
          it->second.symbolId = nextSymbolId++;
          if (it->second.section != "ABS") 
            it->second.value += sectionData[it->second.section][filename].currAddrMergedSection;
          mergedSymbolTable[it->second.symbolName] = it->second;
        }

    }
  }

  // solve extern symbols
  for (map<string, SymbolTable>::iterator it = rememberExternSymbols.begin(); 
    it != rememberExternSymbols.end(); it++) {

    // let's check if symbol isn't in symbol table
    if (!search(it->first, "mergedSymbolTable")) {
      if (!isRelocateable) {
        cout << "Linker error: Symbol " + it->first + " isn't defined!" << endl;
        return false;
      }
      else {  // if it is -relocateable
        it->second.symbolId = nextSymbolId++;
        mergedSymbolTable[it->first] = it->second;
      }    
    }
  }
  return true;
}

// make one big section table
bool Linker::makeMergedSections() {
  map<string, int> sizesOfSections;
  // lets initialize it
  for (string filename: linkingFiles)
    for (map<string, SectionTable>::iterator it = fileSectionTable[filename].begin();
      it != fileSectionTable[filename].end(); it++)
        sizesOfSections[it->second.sectionName] = 0;
  // finished initialization

  vector<string> help;
  int id = 1;
  // let's find all sections with the same name in files and merge them together
  for (string filename: linkingFiles) {
    for (map<string, SectionTable>::iterator it = fileSectionTable[filename].begin();
      it != fileSectionTable[filename].end(); it++) {
        if (it->second.sectionName == "UND") continue;  // skip undefined section
        SectionData newData;
        newData.sectionName = it->second.sectionName;
        newData.size = it->second.size;
        newData.filename = filename;
        newData.currAddrMergedSection = sizesOfSections[it->second.sectionName];
        sizesOfSections[it->second.sectionName] += it->second.size;
        bool found = false;
        for (auto ite: help) {
          if (ite == it->first) {
            found = true; break;
          }
        }
        if (!found) help.push_back(it->first);

        sectionData[it->second.sectionName][filename] = newData;
    }
  }
  // int secId = 0;
  // // passing through sizesOfSection and making final section table
  // for (map<string, int>::iterator it = sizesOfSections.begin(); it != sizesOfSections.end(); it++) {
  //   SectionTable newSection;
  //   if (it->first == "ABS") newSection.sectionId = -1;
  //   else if (it->first == "UND") newSection.sectionId = 0;
  //   else newSection.sectionId = ++secId;
  //   newSection.size = it->second;
  //   newSection.sectionName = it->first;
  //   newSection.virtualMemAddr = 0;

  //   this->mergedSectionTable[it->first] = newSection;
  // }

  int secId = 0;
  // passing through sizesOfSection and making final section table
  for (auto i: help) {
    for (map<string, int>::iterator it = sizesOfSections.begin(); it != sizesOfSections.end(); it++) {
      if (i == it->first) {
        SectionTable newSection;
        if (it->first == "ABS") newSection.sectionId = -1;
        else if (it->first == "UND") newSection.sectionId = 0;
        else newSection.sectionId = ++secId;
        newSection.size = it->second;
        newSection.sectionName = it->first;
        newSection.virtualMemAddr = 0;

        this->mergedSectionTable[it->first] = newSection;
      }
    }
  }
  if (!isRelocateable && !moveSectionsToVirtualAddress()) return false;

  return true;
}

// merge all section data
bool Linker::makeMergedSectionData() {
  // cout << "UNUTAR SECTION DATA " << endl;
  for (map<string, SectionTable>::iterator it = mergedSectionTable.begin();
    it != mergedSectionTable.end(); it++) {
    // if section doesn't have any data then skip it
    if (it->second.size == 0) continue;
    // cout << "SECTION SIZE " << it->second.size << " FOR SECTION " << it->first << endl;
    // cout << "FIND ALL SECTIONS " << endl;

    // iterate through files and find all sections
    for (string filename: linkingFiles) {
      for (map<string, SectionData>::iterator it2 = sectionData[it->first].begin();
        it2 != sectionData[it->first].end(); it2++) {
        // if section doesn't exist in file then continue
        // cout << "SECTION DATA FILENAME " << it2->first << endl;
        if (it2->second.filename != filename) continue;
        else {  // section exists in file
        if (fileSectionTable[filename][it->first].size == 0) continue;
        // cout << "OTISAO U ELSE " << endl;
        // cout << "SEKCIJA " << it->first << endl;
        // cout << "VREDNOST " << fileSectionTable[filename][it->first].offset.size() << endl;
        //for (map<string, SectionTable>::iterator it3 = fileSectionTable[filename][it->first].offse)
          for (int i = 0; i < fileSectionTable[filename][it->first].offset.size() - 1; i++) {
            int currOff = fileSectionTable[filename][it->first].offset[i];
            int nextOff = fileSectionTable[filename][it->first].offset[i + 1];
            // cout << "CURROFF FIRST " << currOff << endl;
            // cout << "NEXTOFF FIRST " << nextOff << endl;
            it->second.offset.push_back(currOff + sectionData[it->first][filename].currAddrMergedSection);
            for (int j = currOff; j < nextOff; j++) {
              // cout << "UNUTAR FORA " << (int)fileSectionTable[filename][it->first].info[j] << endl;
              it->second.info.push_back(fileSectionTable[filename][it->first].info[j]);
              //cout << "MERGED SEC " << hex << (int)it->second.info[it->second.info.size() - 1] << endl;
            }

 
          }
          // cout << "PUT THE LAST ONE " << endl;
          // cout << "FILENAME " << filename << endl;
          // cout << "SEKCIJA " << it->first << endl;

          // last one
          int currOff = fileSectionTable[filename][it->first]
            .offset[fileSectionTable[filename][it->first].offset.size() - 1];
          int nextOff = fileSectionTable[filename][it->first].info.size();
          // cout << "CURROFF " << currOff << endl;
          // cout << "NEXTOFF " << nextOff << endl;
          it->second.offset.push_back(currOff + sectionData[it->first][filename].currAddrMergedSection);
          // cout << "PRE FORA" << endl;
          for (int j = currOff; j < nextOff; j++) {
            // cout << "UNUTAR FORA " << endl;
            // cout << "VREDNOST " << (int)fileSectionTable[filename][it->first].info[j] << endl;
            it->second.info.push_back(fileSectionTable[filename][it->first].info[j]);
          }
          // cout << "IZA FORA" << endl;
        }
      } // end of sectionData for
    } // end of linkingFiles for
  } // end of mergedSectionTable for

  // for (map<string, SectionTable>::iterator it = mergedSectionTable.begin(); it != mergedSectionTable.end();
  //   it++) {
  //     cout << "section " << it->first << endl;
  //     for (const auto& a : it->second.info) {
  //       cout << hex << (int)a << endl;
  //     }
  // }
  //       cout << "POCETAK ISPISA" << endl;
  // for (map<string, SectionTable>::iterator iter = mergedSectionTable.begin(); 
  //   iter != mergedSectionTable.end(); iter++) {
  //     cout << "Section " + iter->first + " info" << endl;
  //     for (int i = 0; i < iter->second.info.size(); i++) {
  //       cout << hex << (int)iter->second.info[i] << endl;
  //     }
  //   }
  // cout << "KRAJ ISPISA" << endl;
  // cout << "FINISHED ALL" << endl;
  return true;
}

// make one big relocation table
void Linker::makeMergedRelocations() {
  for (string filename: linkingFiles) { // iterate through all files
    for (RelocationTable it: fileRelocationTable[filename]) {
      RelocationTable newData;
      newData.offset = it.offset
       + sectionData[it.sectionName][filename].currAddrMergedSection
       - mergedSectionTable[it.sectionName].virtualMemAddr;
      newData.isInfo = it.isInfo;
      newData.sectionName = it.sectionName;
      newData.symbolName = it.symbolName;
      newData.type = it.type;
      newData.filename = filename;
      mergedRelocationTable.push_back(newData);
    }
  }
}

// to remove .hex from output file name and put .txt
regex file("(.*).hex");

// make text file for humans
void Linker::makeTextFile() {
  string outputPath = regex_replace(outputFileName, file, "$1");
  ofstream outputFile(outputPath + ".txt");
  // symbol table
  outputFile << "Symbol Table:" << endl;
  outputFile << "\tId\t\tValue\t\tBind\t\tSection\t\tName"
    << endl << "\t--\t\t-----\t\t----\t\t-------\t\t----" << endl;

  for (map<string, SymbolTable>::iterator it = mergedSymbolTable.begin(); it != mergedSymbolTable.end(); it++) {
    outputFile << hex << setfill('0') << setw(4) << (0xffff & it->second.symbolId) << "\t\t";
    outputFile << hex << setfill('0') << setw(4) << (0xffff & it->second.value) << "\t\t";

    if (!it->second.isGlobal) outputFile << "\tLOC\t\t\t";
    else {
        if (it->second.isDefined) outputFile << "\tGLOB\t\t\t";
        else {
          if (it->second.isExtern) outputFile << "\tEXT\t\t\t";
          else outputFile << "\tUN\t\t\t";
        }
    }
    outputFile << it->second.section << "\t\t\t\t" << it->second.symbolName << "\t\t\t" << endl;
  }

  outputFile << endl;

  // section table
  outputFile << "Section Table:" << endl;
  outputFile << "Id\t\tName\t\t\tSize\t\t\tVA" << endl;
  outputFile << "--\t\t----\t\t\t----\t\t\t--" << endl;
  for (map<string, SectionTable>::iterator it = mergedSectionTable.begin(); it != mergedSectionTable.end(); it++) {
    outputFile << it->second.sectionId << "\t" << it->second.sectionName << "\t";
    outputFile << hex << setfill('0') << setw(4) << (0xffff & it->second.size) << "\t";
    outputFile << hex << setfill('0') << setw(4) << (0xffff & it->second.virtualMemAddr) << endl;
  }

  outputFile << endl;
  // section data
  outputFile << "Section data:" << endl;


  for (map<string, map<string, SectionData>>::iterator sec = sectionData.begin();
        sec != sectionData.end(); sec++) {

    outputFile << "Section: " << sec->first << " :{" << endl;
    for (map<string, SectionData>::iterator file = sec->second.begin();
          file != sec->second.end(); file++) {

      outputFile << "\t" << "# " << file->first;
      SectionData secData = file->second;

      outputFile << "\t\t" << secData.sectionName << "\t" << secData.filename;
      outputFile << ":\t" << secData.size;
      outputFile << " [" << secData.currAddrMergedSection << ",";
      outputFile << secData.currAddrMergedSection + secData.size << ")";

      outputFile << endl;
    }
    outputFile << "\t}" << endl;
    outputFile << endl;
  }

  // relocation data
  outputFile << "Relocation data" << endl;
  outputFile << "Offset\tType\t\t\tSymbol\t\tSection name" << endl;

  for (RelocationTable rel_data : mergedRelocationTable) {
    outputFile << hex << setfill('0') << setw(4) << (0xffff & rel_data.offset);
    outputFile << "\t" << rel_data.type << "\t\t";
    outputFile << "\t" << rel_data.symbolName << "\t\t" << rel_data.sectionName << endl;
    outputFile << "\t\t" << rel_data.filename << endl;
  }
  outputFile << endl;
  // if -relocateable option is active
  if (isRelocateable) {
    outputFile << "Relocateable result:" << endl;
    for (map<string, SectionTable>::iterator it = mergedSectionTable.begin(); it != mergedSectionTable.end(); it++) {
 
      outputFile << "#." << it->first << endl;
      if (it->second.size == 0) continue;
      
      SectionTable sec = it->second;
      for (int i = 0; i < sec.offset.size() - 1; i++) {

        int currOffset = sec.offset[i];
        int nextOffset = sec.offset[i + 1];
        outputFile << hex << setfill('0') << setw(4) << (0xffff & currOffset) << ": ";
        for (int j = currOffset; j < nextOffset; j++) {
          char c = sec.info[j];
          outputFile << hex << setfill('0') << setw(2) << (0xff & c) << " ";
        }
        outputFile << endl;
      }
      // last one
      int currOffset = sec.offset[sec.offset.size() - 1];
      int nextOffset = sec.info.size();
      outputFile << hex << setfill('0') << setw(4) << (0xffff & currOffset) << ": ";
      for (int j = currOffset; j < nextOffset; j++) {
        char c = sec.info[j];
        outputFile << hex << setfill('0') << setw(2) << (0xff & c) << " ";
      }
      outputFile << endl << endl;
    }
  }
  else {  // -hex option

    outputFile << "Hex result:" << endl;
    for (map<string, SectionTable>::iterator it = mergedSectionTable.begin(); it != mergedSectionTable.end(); it++) {
        outputFile << "Section data " << it->first << ":" << endl;

        SectionTable sec = it->second;
        int counter = 0;

        for (int i = 0; i < sec.info.size(); i++) {
            char c = sec.info[i];
            if (counter % 8 == 0) 
              outputFile << hex << setfill('0') << setw(4) << (0xffff & counter + sec.virtualMemAddr) << "   ";
            
            outputFile << hex << setfill('0') << setw(2) << (0xff & c) << " ";
            counter++;
            if (counter % 8 == 0) outputFile << endl;
        }
        outputFile << endl << endl;
    }
  }
  outputFile.close();
}

// make binary file for emulator
void Linker::makeBinaryFile() {
  if (isRelocateable) {
    string binaryFile = outputFileName;
    ofstream binaryOut(binaryFile, ios::out | ios::binary);
    int len;

    // Symbol table
    int symNum = mergedSymbolTable.size();
    binaryOut.write((char *)&symNum, sizeof(symNum));

    for (map<string, SymbolTable>::iterator it = mergedSymbolTable.begin(); it != mergedSymbolTable.end(); it++) {
      string key = it->first;
      // key
      len = key.length();
      binaryOut.write((char*)(&len), sizeof(len));
      binaryOut.write(key.c_str(), key.length());
      // symbol id
      binaryOut.write((char*)(&it->second.symbolId), sizeof(it->second.symbolId));
      // value
      binaryOut.write((char*)(&it->second.value), sizeof(it->second.value));
      binaryOut.write((char*)(&it->second.isDefined), sizeof(it->second.isDefined));
      binaryOut.write((char*)(&it->second.isGlobal), sizeof(it->second.isGlobal));
      binaryOut.write((char*)(&it->second.isExtern), sizeof(it->second.isExtern));
      // section name
      len = it->second.section.length();
      binaryOut.write((char*)(&len), sizeof(len));
      binaryOut.write(it->second.section.c_str(), it->second.section.length());
      // symbol name
      len = it->second.symbolName.length();
      binaryOut.write((char*)(&len), sizeof(len));
      binaryOut.write(it->second.symbolName.c_str(), it->second.symbolName.length());

    }

    // Section table
    int secNum = mergedSectionTable.size();
    binaryOut.write((char*)&secNum, sizeof(secNum));

    for (map<string, SectionTable>::iterator it = mergedSectionTable.begin(); it != mergedSectionTable.end(); it++) {
      string key = it->first;
      len = key.length();
      binaryOut.write((char*)(&len), sizeof(len));
      binaryOut.write(key.c_str(), key.length());
      // section id
      binaryOut.write((char*)(&it->second.sectionId), sizeof(it->second.sectionId));
      // size
      binaryOut.write((char*)(&it->second.size), sizeof(it->second.size));
      len = it->second.sectionName.length();
      // offset
      int offNum = it->second.offset.size();
      binaryOut.write((char*)&offNum, sizeof(offNum));
      for (int o : it->second.offset) binaryOut.write((char *)&o, sizeof(o));
      // info
      int charNum = it->second.info.size();
      binaryOut.write((char*)&charNum, sizeof(charNum));
      for (char c : it->second.info) binaryOut.write((char *)&c, sizeof(c));
      
    
      // section name
      binaryOut.write((char*)(&len), sizeof(len));
      binaryOut.write(it->second.sectionName.c_str(), it->second.sectionName.length());
    }

    // Relocation
    
    int relNum = mergedRelocationTable.size();
    // int cnt = 0;
    // for (const auto& rel : relocationTable) {
    //  if (!symbolTable[rel.symbolName].isGlobal) cnt++;
    // }
    //relNum -= cnt;
    binaryOut.write((char*)&relNum, sizeof(relNum));
    
    for (RelocationTable relData : mergedRelocationTable) {
      //if (!symbolTable[relData.symbolName].isGlobal) continue;
      binaryOut.write((char*)(&relData.offset), sizeof(relData.offset));
      binaryOut.write((char*)(&relData.isInfo), sizeof(relData.isInfo));
      // section name
      len = relData.sectionName.length();
      binaryOut.write((char*)(&len), sizeof(len));
      binaryOut.write(relData.sectionName.c_str(), relData.sectionName.length());
      // symbol name
      len = relData.symbolName.length();
      binaryOut.write((char*)(&len), sizeof(len));
      binaryOut.write(relData.symbolName.c_str(), relData.symbolName.length());
      // type
      len = relData.type.length();
      binaryOut.write((char*)(&len), sizeof(len));
      binaryOut.write(relData.type.c_str(), relData.type.length());
    }
    binaryOut.close();
  }
  else {  // hex option
    ofstream binaryOut(outputFileName, ios::out | ios::binary);

    int numOfSections = 0;
    for (map<string, SectionTable>::iterator it = mergedSectionTable.begin();
      it != mergedSectionTable.end(); it++) {
        if (it->first == "UND" || it->first == "ABS") continue;
        else numOfSections++;
    }

    binaryOut.write((char*)&numOfSections, sizeof(numOfSections));

    for(map<string, SectionTable>::iterator it = mergedSectionTable.begin();
      it != mergedSectionTable.end(); it++) {
        if (it->first == "UND" || it->first == "ABS") continue;
        // cout << "NAME OF SECTION " << it->first << " and size " << it->second.size << "and size 2 "
          //  << it->second.info.size();
        int size = it->second.info.size();
        binaryOut.write((char*)(&it->second.virtualMemAddr), sizeof(it->second.virtualMemAddr));
        binaryOut.write((char*)&size, sizeof(size));

        for (char data: it->second.info) binaryOut.write((char*)&data, sizeof(data));
        
    }
  }
}

// link all files
bool Linker::link() {
  if (!readBinaryFilesFromAsembler()) return false;
  // cout << "PROCITAO" << endl;
  if ( !makeMergedSections()) return false;
  // cout << "SEKCIJE" << endl;
  if (!makeMergedSymbolTable()) return false;
  // cout << "SYMBOL" << endl;
  makeMergedRelocations();
  // cout << "RELOKACIJE" << endl;
  if ( !makeMergedSectionData()) return false;
  // cout << "SECTION DATA" << endl;
  if (isRelocateable && !relocationDataRelocateable()) return false;
  else if (!relocationDataHex()) return false;
  // cout << "KRAJ" << endl;
  makeTextFile();
  makeBinaryFile();
  return true;
}