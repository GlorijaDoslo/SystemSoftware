#ifndef EMULATOR_H
#define EMULATOR_H


#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>
#include <map>

using namespace std;

class Emulator {

  enum Register {
    r0 = 0, r1 = 1, r2 = 2, r3 = 3, r4 = 4, r5 = 5, r6 = 6, sp = r6, r7 = 7, pc = r7, psw = 8,
    unimportantReg = 0xF
  };

  enum Flag {
    Z = 1 << 0, O = 1 << 1, C = 1 << 2, N = 1 << 3, Tr = 1 << 13, Tl = 1 << 14, I = 1 << 15
  };

  enum InstructionMnemonic {
    halt, intInstr, iret, call, ret, jmp, jeq, jne, jgt, xchg, add, sub, mul, div, cmp, notInstr,
    andInstr, orInstr, xorInstr, test, shl, shr, ldr, str
  };

  enum AddressType {
    imm, regDir, regInd, regIndDispl, memDir, regIndAdd
  };

  enum UpdateType {
    noUpdate, preDec, preInc, postDec, postInc
  };

  string inputFileName;

  bool running; // program status
  ofstream outputFile;

  // Instruction variables
  InstructionMnemonic instrMnemonic;
  int instrSize;
  int destRegNumber;
  int sourceRegNumber;
  int updateType;
  int addressType;
  short instrPayload; // This is the 2B of data, ordered by big endian
  int prevPC;

  // Registers variables
  vector<short> registers; 
  short &rpc = registers[pc];
  short &rsp = registers[sp];
  short &rpsw = registers[psw];

  // Memory variables
  vector<char> memory;

  // Interrupts variables

  vector<bool> interruptsRequests;

  // Timer variables

  short timerPeriodId;
  long long int timerPeriod;

  bool timerActive;

  long long int previousTime;
  long long int currentTime;

  // ***** Instruction *****

  short fetchOperandByAddressingType();
  bool setOperandByAddressingType(short);
  void updateBeforeAddressFetchSourceRegister();
  void updateAfterAddressFetchSourceRegister();
  bool instrRecognizer();
  bool instrExecuter();

  // ***** Registers *****

  void pushOnStack(short value);
  short popFromStack();
  void resetFlag(short flag);
  void setFlag(short flag);
  bool getFlag(short flag);
  bool jumpPSWCalculator(short instr);


  // ***** Memory *****

  short readFromMemory(int address, int size, bool littleEndian = true);                                                                        
  void writeToMemory(short value, int address, int size, bool littleEndian = true);

  void writeMemoryInFile(string filename);
  // ***** Interrupts *****
  
  void setInterruptRequestOnLine(int line);
  void interruptRequestsHandler();
  void jumpOnInterruptRoutine(short entry);


  // ***** Terminal *****

  bool configureTerminal();
  void resetTerminal();
  void readCharFromInput();

  // ***** Timer functions *****

  long long int fetchDurationById(short id);
  void resetTimer();
  void timerTick();

public:
  Emulator(string);
  bool readBinaryFileFromLinker();

  bool emulate();

  static int MEMORY_SIZE;
  static int START_STACK_ADDR;
  static int REGISTER_NUM;

  static int START_PROGRAM_ADDR;
  static int ERROR;
  static int TIMER;
  static int TERMINAL;

  static int NUM_OF_PERIFERIES;
  static int TERMINAL_INDEX_NUM;
  static int TIMER_INDEX_NUM;

  static short TERM_OUT;
  static short TERM_IN;
  static short TIM_CFG;
};

#endif //EMULATOR_H