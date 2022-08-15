// #include <iostream>
// #include <fstream>
#include <iomanip>
#include <chrono>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <termios.h>

#include "/home/ss/Desktop/Projekat/inc/emulator.hpp"

using namespace std;

int Emulator::MEMORY_SIZE = 1 << 16;
int Emulator::START_STACK_ADDR = 0xFF00;
int Emulator::REGISTER_NUM = 9;

int Emulator::START_PROGRAM_ADDR = 0;
int Emulator::ERROR = 1;
int Emulator::TIMER = 2;
int Emulator::TERMINAL = 3;

// this is number of periferies (2 periferies, terminal and timer)
int Emulator::NUM_OF_PERIFERIES = 2;
int Emulator::TERMINAL_INDEX_NUM = 0;
int Emulator::TIMER_INDEX_NUM = 1;

// for displaying on terminal
short Emulator::TERM_OUT = 0xFF00;
// reading from terminal
short Emulator::TERM_IN = 0xFF02;
// configuring timer
short Emulator::TIM_CFG = 0xFF10;

struct termios stdinBackupSettings;

// constructor
Emulator::Emulator(string inputFile) : memory(MEMORY_SIZE, 0), registers(REGISTER_NUM, 0),
  outputFile("emulator.txt"), interruptsRequests(NUM_OF_PERIFERIES, 0) {
  this->inputFileName = inputFile;
  running = false;
  timerActive = false;
  previousTime = chrono::duration_cast<chrono::milliseconds>(
      chrono::system_clock::now().time_since_epoch()).count();
}

// ***** Instructions *****

// recognize the addressing type and fetch operand
short Emulator::fetchOperandByAddressingType() {
  short ret;

  if (addressType == imm) {
    // cout << "IMM" << endl;
    ret = instrPayload;
  }
  else if (addressType == regDir) {
    // cout << "REGDIR " << hex << registers[sourceRegNumber] << " and destReg " 
      // << hex << registers[destRegNumber] << endl;
    ret = registers[sourceRegNumber];

  }
  else if (addressType == regInd) {
    // cout << "REGIND " << hex << registers[sourceRegNumber] << " and destReg " 
      // << hex << registers[destRegNumber] << endl;
      ret = readFromMemory(registers[sourceRegNumber], 2);
  }
  else if (addressType == regIndDispl) {
    // cout << "REG IND DISP " << hex << registers[sourceRegNumber] << " INSTR PAY " << hex << instrPayload << endl;
    
    ret = readFromMemory(registers[sourceRegNumber] + instrPayload, 2);
    // cout << "RETURN " << hex << (int)ret << endl;
  }
  else if (addressType == memDir) {
  //  cout << "MEMDIR " << hex << registers[sourceRegNumber] << " and destReg " 
      // << hex << registers[destRegNumber] << endl; 
      ret = readFromMemory(instrPayload, 2);
  }
  else if (addressType == regIndAdd) {
    // cout << "regIndAdd " << hex << registers[sourceRegNumber] << " and destReg " 
      // << hex << registers[destRegNumber] << endl;
      ret = registers[sourceRegNumber] + instrPayload;
  } 
  else {
    cout << "Emulator error: Addressing type is not supported!" << endl;
  }
  return ret;
}

// recognize addressing type and store operand in register or memory
bool Emulator::setOperandByAddressingType(short value) {
  if (addressType == imm) {
        // cout << "IMM " << hex << registers[sourceRegNumber] << " and destina Reg " << hex << registers[destRegNumber] << endl;
    cout << "Emulator error: Cannot store in immediate value!" << endl;
    return false;
  }
  else if (addressType == regDir) {
    // cout << "REGDIR " << hex << registers[sourceRegNumber] << " and destina Reg " << hex << registers[destRegNumber] << endl;
    registers[sourceRegNumber] = value;
    return true;
  }
  else if (addressType == regInd) { // register value as address
    // cout << "ADDRESS TYPE REGIND " << hex <<registers[sourceRegNumber] << endl;
    // cout << "REGIND " << hex << registers[sourceRegNumber] << " and destina Reg " << hex << registers[destRegNumber] << endl;

    writeToMemory(value, registers[sourceRegNumber], 2);
    // cout << "REG IND " << hex << readFromMemory(registers[sourceRegNumber], 2) << endl;
    return true;
  }
  else if (addressType == regIndDispl) {
    // cout << "ADDRESS TYPE REGINDDISPL " << hex << registers[sourceRegNumber] + instrPayload << endl;
    
    writeToMemory(value, registers[sourceRegNumber] + instrPayload, 2);
    // cout << "REG IND DISPL " << hex << readFromMemory(registers[sourceRegNumber] + instrPayload, 2) << endl;
    return true;
  }
  else if (addressType == memDir) {
    // cout << "ADDRESS TYPE MEMDIR " << hex << instrPayload << endl;
    writeToMemory(value, instrPayload, 2);
    // cout << "MEMDIR " << hex << readFromMemory(instrPayload, 2);
    return true;
  }
  else if (addressType == regIndAdd) {
    // cout << "REGIND ADD " << hex << registers[sourceRegNumber] << " and destina Reg " << hex << registers[destRegNumber] << endl;
    cout << "Emulator error: Cannot store in immediate value!" << endl;
    return false;
  }
  else {
    cout << "Emulator error: Addressing type is not supported!" << endl;
    return false;
  }
}

// pre increment and pre decrement addressing types
void Emulator::updateBeforeAddressFetchSourceRegister() {
  if (updateType == noUpdate) {
    return;
  }
  else if (updateType == preDec) {
    registers[sourceRegNumber] -= 2;
  }
  else if (updateType == preInc) {
    registers[sourceRegNumber] += 2;
  }
}

// post increment and post decrement addressing types
void Emulator::updateAfterAddressFetchSourceRegister() {
  if (updateType == noUpdate) {
    return;
  }
  else if (updateType == postDec) {
    registers[sourceRegNumber] -= 2;
  }
  else if (updateType == postInc) {
    registers[sourceRegNumber] += 2;
  }
}

// recognizes instruction on which pc register is pointing
bool Emulator::instrRecognizer() {
  // cout << "INSTR FETCH AND DECODE" << endl;
  short instr = readFromMemory(rpc, 1);
  rpc += 1;
  // instruction has operation code and modificator, both are 4 bits

  char mod = instr & 0xf;
  char opcode = (instr >> 4) & 0xf;

  // if (opcode == 9) {
  //   cout << "INSTR " << hex << instr << endl;
  //   cout << "RPC " << hex << (rpc - 1) << endl;
  //   cout << "MODDDDD " << hex << (int)mod << endl;
  // }

  if (opcode == 0) {
    instrMnemonic = halt;

    if (mod != 0) {
      cout << "Emulator error: Error in halt instruction!" << endl;
      return false;
    }
    return true;
  }
  else if (opcode == 1) {
    instrMnemonic = intInstr;
    if (mod != 0) {
      cout << "Emulator error: Error in int instruction!" << endl;
      return false;
    }
    short reg = readFromMemory(rpc, 1);
    rpc += 1;
    destRegNumber = (reg >> 4) & 0xf;
    sourceRegNumber = reg & 0xf;

    if (sourceRegNumber != 0xf) {
      cout << "Emulator error: Error in int instruction, bad register value!" << endl;
      return false;
    }
    return true;
  }
  else if (opcode == 2) {
    instrMnemonic = iret;
    if (mod != 0) {
      cout << "Emulator error: Error in iret instruction!" << endl;
      return false;
    }
    return true;
  }
  else if (opcode == 3) {
    instrMnemonic = call;
    if (mod != 0) {
      cout << "Emulator error: Error in call instruction!" << endl;
      return false;
    }
    short reg = readFromMemory(rpc, 1);
    rpc += 1;
    destRegNumber = (reg >> 4) & 0xf;
    sourceRegNumber = reg & 0xf;

    if (destRegNumber != 0xf) {
      cout << "Emulator error: Error in call instruction, bad register value!" << endl;
      return false;
    }
    short addrType = readFromMemory(rpc, 1);
    rpc += 1;

    updateType = (addrType >> 4) & 0xf;
    addressType = addrType & 0xf;

    instrSize = 3;

    if (addressType == imm || addressType == memDir || addressType == regIndDispl
      || addressType == regIndAdd) {
      instrSize += 2;
      //cout << "RPC " << hex << rpc << endl;
      instrPayload = readFromMemory(rpc, 2, false);
      //cout << "INSTRPAYLOAD " << hex << instrPayload << endl;
      rpc += 2;
    }
    return true;
  }
  else if (opcode == 4) {
    instrMnemonic = ret;
    if (mod != 0) {
      cout << "Emulator error: Error in ret instruction!" << endl;
      return false;
    }
    return true;
  }
  else if (opcode == 5) { // jump instructions
    if (mod == 0) instrMnemonic = jmp;
    else if (mod == 1) instrMnemonic = jeq;
    else if (mod == 2) instrMnemonic = jne;
    else if (mod == 3) instrMnemonic = jgt;
    else {
      cout << "Emulator error: Error in jump instructions, bad modificator!" << endl;
      return false;
    }
    short reg = readFromMemory(rpc, 1);
    rpc += 1;
    destRegNumber = (reg >> 4) & 0xf;
    sourceRegNumber = reg & 0xf;

    if (destRegNumber != 0xf) {
      cout << "Emulator error: Error in jump instruction, bad register value!" << endl;
      return false;
    }
    
    short addrType = readFromMemory(rpc, 1);
    rpc += 1;

    updateType = (addrType >> 4) & 0xf;
    addressType = addrType & 0xf;
    instrSize = 3;

    if (addressType == imm || addressType == memDir || addressType == regIndDispl 
      || addressType == regIndAdd) {
        instrSize += 2;
        //cout << "RPC " << hex << rpc << endl;
        instrPayload = readFromMemory(rpc, 2, false);
        //cout << "INSTRPAYLOAD " << hex << instrPayload << endl;
        rpc += 2;
    }
    return true;
  }
  else if (opcode == 6) {
    instrMnemonic = xchg;
    if (mod != 0) {
      cout << "Emulator error: Error in xchg instruction!" << endl;
      return false;
    }
    short reg = readFromMemory(rpc, 1);
    rpc += 1;
    destRegNumber = (reg >> 4) & 0xf;
    sourceRegNumber = reg & 0xf;
    return true;
  }
  else if (opcode == 7) { // add, sub, mul, div, cmp
    if (mod == 0) instrMnemonic = add;
    else if (mod == 1) instrMnemonic = sub;
    else if (mod == 2) instrMnemonic = mul;
    else if (mod == 3) instrMnemonic = div;
    else if (mod == 4) instrMnemonic = cmp;
    else {
      cout << "Emulator error: Error in aritmetic instructions, bad modificator!" << endl;
      return false;
    }
    short reg = readFromMemory(rpc, 1); 
    rpc += 1;
    destRegNumber = (reg >> 4) & 0xf;
    sourceRegNumber = reg & 0xf;
    return true;
  }
  else if (opcode == 8) { // not, and, or, xor, test
    if (mod == 0) instrMnemonic = notInstr;
    else if (mod == 1) instrMnemonic = andInstr;
    else if (mod == 2) instrMnemonic = orInstr;
    else if (mod == 3) instrMnemonic = xorInstr;
    else if (mod == 4) instrMnemonic = test;
    else {
      cout << "Emulator error: Error in logic instructions, bad modificator!" << endl;
      return false;
    }
    short reg = readFromMemory(rpc, 1); 
    rpc += 1;
    destRegNumber = (reg >> 4) & 0xf;
    sourceRegNumber = reg & 0xf;
    return true;
  }
  else if (opcode == 9) { // shl, shr
    if (mod == 0) instrMnemonic = shl;
    else if (mod == 1) instrMnemonic = shr;
    else {
      // cout << "MOD " << hex << (int)mod << endl;
      cout << "Emulator error: Error in shift instructions, bad modificator!" << endl;
      return false;
    }
    short reg = readFromMemory(rpc, 1); 
    rpc += 1;
    destRegNumber = (reg >> 4) & 0xf;
    sourceRegNumber = reg & 0xf;
    return true;
  }
  else if (opcode == 10) {
    instrMnemonic = ldr;
    if (mod != 0) {
      cout << "Emulator error: Error in ldr instruction!" << endl;
      return false;
    }
    short reg = readFromMemory(rpc, 1);
    rpc += 1;
    destRegNumber = (reg >> 4) & 0xf;
    sourceRegNumber = reg & 0xf;

    short addrType = readFromMemory(rpc, 1);
    rpc += 1;

    updateType = (addrType >> 4) & 0xf;
    addressType = addrType & 0xf;

    instrSize = 3;

    if (addressType == imm || addressType == memDir || addressType == regIndDispl
      || addressType == regIndAdd) {
      instrSize += 2;
      //cout << "RPC " << hex << rpc << endl;
      instrPayload = readFromMemory(rpc, 2, false);
      //cout << "INSTRPAYLOAD " << hex << instrPayload << endl;
      rpc += 2;
    }
    return true;
  }
  else if (opcode == 11) {
    instrMnemonic = str;
    if (mod != 0) {
      cout << "Emulator error: Error in str instruction!" << endl;
      return false;
    }
    short reg = readFromMemory(rpc, 1);
    rpc += 1;
    destRegNumber = (reg >> 4) & 0xf;
    sourceRegNumber = reg & 0xf;

    short addrType = readFromMemory(rpc, 1);
    rpc += 1;

    updateType = (addrType >> 4) & 0xf;
    addressType = addrType & 0xf;

    instrSize = 3;

    if (addressType == imm || addressType == memDir || addressType == regIndDispl
      || addressType == regIndAdd) {
      instrSize += 2;
      //cout << "RPC " << hex << rpc << endl;     
      instrPayload = readFromMemory(rpc, 2, false);
      //cout << "INSTRPAYLOAD " << hex << instrPayload << endl;
      rpc += 2;
    }
    return true;
  }
  else {
    cout << "Emulator error: Instruction is not supported!" << endl;
    return false;
  }
}

// executes instruction on which pc register is pointing
bool Emulator::instrExecuter() {
  // cout << "INSTR EXECUTE" << endl;
  // cout << "INSTR MNEMONIC " << instrMnemonic << " ADDRESS TYPE " << addressType << endl;
  if (instrMnemonic == halt) {
    // cout << "halt " << endl;
    running = false;
  }
  else if (instrMnemonic == intInstr) {
    // cout << "int " << endl;
    jumpOnInterruptRoutine(registers[destRegNumber]);
  }
  else if (instrMnemonic == iret) {
    // cout << "iret " << endl;
    rpsw = popFromStack();
    rpc = popFromStack();
  }
  else if (instrMnemonic == call) {
    // cout << "call " << endl;
    updateBeforeAddressFetchSourceRegister();
    pushOnStack(rpc);
    short op = fetchOperandByAddressingType();
    rpc = op;
    updateAfterAddressFetchSourceRegister();
  }
  else if (instrMnemonic == ret) {
    // cout << "ret " << endl;
    rpc = popFromStack();
  }
  else if (instrMnemonic == jmp) {
    // cout << "jmp " << endl;
    updateBeforeAddressFetchSourceRegister();
    short op = fetchOperandByAddressingType();
    rpc = op;
    updateAfterAddressFetchSourceRegister();
  }
  else if (instrMnemonic == jeq) {
    // cout << "jeq " << endl;
    updateBeforeAddressFetchSourceRegister();
    short op = fetchOperandByAddressingType();
    bool cond = jumpPSWCalculator(jeq);

    if (cond) rpc = op;
    updateAfterAddressFetchSourceRegister();
  }
  else if (instrMnemonic == jne) {
    // cout << "jne " << endl;
    updateBeforeAddressFetchSourceRegister();
    short op = fetchOperandByAddressingType();
    bool cond = jumpPSWCalculator(jne);

    if (cond) rpc = op;
    updateAfterAddressFetchSourceRegister();
  }
  else if (instrMnemonic == jgt) {
    // cout << "jgt " << endl;
    updateBeforeAddressFetchSourceRegister();
    short op = fetchOperandByAddressingType();
    bool cond = jumpPSWCalculator(jgt);

    if (cond) rpc = op;
    updateAfterAddressFetchSourceRegister();
  }
  else if (instrMnemonic == xchg) {
    // cout << "xchg " << endl;
    short tmp = registers[destRegNumber];
    registers[destRegNumber] = registers[sourceRegNumber];
    registers[sourceRegNumber] = tmp;
  }
  else if (instrMnemonic == add) {
    // cout << "add " << endl;
    registers[destRegNumber] = registers[destRegNumber] + registers[sourceRegNumber];
  }
  else if (instrMnemonic == sub) {
    // cout << "sub " << endl;
    registers[destRegNumber] = registers[destRegNumber] - registers[sourceRegNumber];
  }
  else if (instrMnemonic == mul) {
    // cout << "mul " << endl;
    registers[destRegNumber] = registers[destRegNumber] * registers[sourceRegNumber];
  }
  else if (instrMnemonic == div) {
    // cout << "div " << endl;
    if (!registers[sourceRegNumber]) {
      cout << "Emulator error: Cannot divide with zero!" << endl;
      return false;
    }
    registers[destRegNumber] = registers[destRegNumber] / registers[sourceRegNumber];
  }
  else if (instrMnemonic == cmp) {  // ZNCO
  // cout << "cmp " << endl;
    short v = registers[destRegNumber] - registers[sourceRegNumber];

    if (v == 0) setFlag(Z);
    else resetFlag(Z);

    if (v < 0) setFlag(N);
    else resetFlag(N);

    if (registers[destRegNumber] < registers[sourceRegNumber]) setFlag(C);
    else resetFlag(C);

    short x = registers[destRegNumber];
    short y = registers[sourceRegNumber];

    if ((x > 0 && y < 0 && (x - y) < 0) || (x < 0 && y > 0 && (x - y) > 0)) setFlag(O);
    else resetFlag(O);
  }
  else if (instrMnemonic == notInstr) {
    // cout << "not " << endl;
    registers[destRegNumber] = !registers[destRegNumber];
  }
  else if (instrMnemonic == andInstr) {
    // cout << "and " << endl;
    registers[destRegNumber] = registers[destRegNumber] & registers[sourceRegNumber];
  }
  else if (instrMnemonic == orInstr) {
    // cout << "or " << endl;
    registers[destRegNumber] = registers[destRegNumber] | registers[sourceRegNumber];

  }
  else if (instrMnemonic == xorInstr) {
    // cout << "xor " << endl;
    registers[destRegNumber] = registers[destRegNumber] ^ registers[sourceRegNumber];
  }
  else if (instrMnemonic == test) { // ZN
    // cout << "test " << endl;
    short v = registers[destRegNumber] & registers[sourceRegNumber];

    if (v == 0) setFlag(Z);
    else resetFlag(Z);

    if (v < 0) setFlag(N);
    else resetFlag(N);
  }
  else if (instrMnemonic == shl) {  // NZC
    // cout << "shl" << endl;
    short prevDest = registers[destRegNumber];
    short prevSource = registers[sourceRegNumber];
    registers[destRegNumber] <<= registers[sourceRegNumber];

    if (!registers[destRegNumber]) setFlag(Z);
    else resetFlag(Z);

    if (registers[destRegNumber] < 0) setFlag(N);
    else resetFlag(N);

    if (prevSource < 16 && ((prevDest >> (16 - prevSource)) & 1)) setFlag(C);
    else resetFlag(C);
  }
  else if (instrMnemonic == shr) {  // NZC bits
    // cout << "shr " << endl;
    short prevDest = registers[destRegNumber];
    short prevSource = registers[sourceRegNumber];
    registers[destRegNumber] >>= registers[sourceRegNumber];

    if (!registers[destRegNumber]) setFlag(Z);
    else resetFlag(Z);

    if (registers[destRegNumber] < 0) setFlag(N);
    else resetFlag(N);

    if ((prevDest >> (prevSource - 1)) & 1) setFlag(C);
    else resetFlag(C);
  }
  else if (instrMnemonic == ldr) {
    // cout << "ldr " << endl;
    updateBeforeAddressFetchSourceRegister();
    short op = fetchOperandByAddressingType();
    registers[destRegNumber] = op;
    updateAfterAddressFetchSourceRegister();
  }
  else if (instrMnemonic == str) {
    // cout << "str " << endl;
    updateBeforeAddressFetchSourceRegister();
    if (!setOperandByAddressingType(registers[destRegNumber])) return false;
    updateAfterAddressFetchSourceRegister();
  }
  else {
    cout << "Emulator error: Instruction isn't supported!" << endl;
    return false;
  }
  return true;
}

// ***** Registers *****

// puts 2B on stack, stack pointer is pointing on last taken, grows toward lower addresses
void Emulator::pushOnStack(short value) {
  rsp -= 2;
  //cout << "PUSH ON STACK " << hex << rsp << endl;
  writeToMemory(value, rsp, 2);
  // cout << "PUSH " << hex << readFromMemory(rsp, 2) << " and rsp " << hex << rsp << endl;
}

short Emulator::popFromStack() {
  short value = readFromMemory(rsp, 2);
  // cout << "POP " << hex << value << " and rsp " << hex << rsp << endl;
  rsp += 2;
  return value;
}

// reset flag in psw register
void Emulator::resetFlag(short flag) {
  rpsw &= ~flag;
}

// set flag in psw register
void Emulator::setFlag(short flag) {
  rpsw |= flag;
}

// get flag in psw register
bool Emulator::getFlag(short flag) {
  return rpsw & flag;
}

// calculate if jump instruction should jump or not
bool Emulator::jumpPSWCalculator(short instr) {
  if (instr == jeq) return getFlag(Z);
  else if (instr == jne) return !getFlag(Z);
  else if (instr == jgt) return !(getFlag(N) ^ getFlag(O)) & !getFlag(Z);
  return false;
}

// ***** Memory *****

// get data from memory on given address and size (1B or 2B) and send information
// to function about endianity
short Emulator::readFromMemory(int addr, int size, bool littleEndian) {
  // instruction has one byte
  // cout << "LITTLE ENDIAN " << littleEndian << endl;
  // cout << "RPC " << rpc << endl;
  // if (rpc == 0x20) {
  //   cout << "LITTLE ENDIAN " << littleEndian << endl;
  //   cout << "MEMORY ADDRESS " << hex << (int)memory[addr] << endl;
  //   cout << "MEMORY ADDRESS " << hex << (int)memory[addr + 1] << endl;
  //   short value = (short)((memory[addr] << 8) + (0xff & memory[addr + 1]));
  //   cout << "VALUE " << value << endl;
  // }
  if (size == 1) return memory[addr];
  else {
    if (littleEndian) { // lower address, lower byte
      char lowerValue = memory[addr];
      char higherValue = memory[addr + 1];
      short value = (short)((higherValue << 8) + (0xff & lowerValue));
      return value;
    }
    else {  // big endian
      char lowerValue = memory[addr + 1];
      char higherValue = memory[addr];
      short value = (short)((higherValue << 8) + (0xff & lowerValue));
      return value;
    }
  }
}
// put data to memory on given address and size (1B or 2B) and send information
// to function about endianity                                                   
void Emulator::writeToMemory(short value, int addr, int size, bool littleEndian) {
  if (size == 1) memory[addr] = value;
  else {
    char lowerValue = 0xff & value;
    char higherValue = 0xff & (value >> 8);
    if (littleEndian) {
      memory[addr] = lowerValue;
      memory[addr + 1] = higherValue;
    }
    else {
      memory[addr + 1] = lowerValue;
      memory[addr] = higherValue;
    }
  }
  // cout << "ADDR " << hex << addr << endl;
  if (addr == TERM_OUT) cout << (char)value << flush << endl;
}

// very helpfull function for debugging which puts memory of whole program in file
void Emulator::writeMemoryInFile(string filename) {
  ofstream out(filename);
  int counter = 0;
  for (int i = 0; i < memory.size(); i++) {
    char c = memory[i];
    if (counter % 8 == 0) out << hex << setfill('0') << setw(4) << (0xffff & counter) << "   ";
    out << hex << setfill('0') << setw(2) << (0xff & c) << " ";
    counter++;
    if (counter % 8 == 0) out << endl;
  }
  out.close();
}

// ***** Interrupts *****

// makes interrupt request
void Emulator::setInterruptRequestOnLine(int line) {
  if (line >= 0 && line < NUM_OF_PERIFERIES) {
    interruptsRequests[line] = true;
  }
}

// handles interrupt requests send by terminal and timer
void Emulator::interruptRequestsHandler() {
  if (!getFlag(I)) {
    for (int intLineNum = 0; intLineNum < NUM_OF_PERIFERIES; intLineNum++) {
      if (interruptsRequests[intLineNum]) {
        if (intLineNum == TIMER_INDEX_NUM) {
          if (!getFlag(Tr)) {
            interruptsRequests[intLineNum] = false;
            jumpOnInterruptRoutine(TIMER);
            break;
          }
        }
        else if (intLineNum == TERMINAL_INDEX_NUM) {
          if (!getFlag(Tl)) {
            interruptsRequests[intLineNum] = false;
            jumpOnInterruptRoutine(TERMINAL);
            break;
          }
        }
      }
    }
  }
}

void Emulator::jumpOnInterruptRoutine(short entry) {
  short address = readFromMemory((entry % 8) * 2, 2);
  // This is proper enter to interrupt
  pushOnStack(rpc);
  pushOnStack(rpsw);

  rpc = readFromMemory((entry % 8) * 2, 2);

  setFlag(I);
  setFlag(Tr);
  setFlag(Tl);
}

// ***** Terminal *****

void backupStdinSettings() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &stdinBackupSettings);
}

bool Emulator::configureTerminal() {
  if (tcgetattr(STDIN_FILENO, &stdinBackupSettings) < 0) {
    cout << "Emulator error: Cannot fetch setings from STDIN_FILENO!" << endl;
    return false;
  }

  static struct termios changedSettings = stdinBackupSettings;
  // ECHO - echoes the character
  // ECHONL - echoes the '\n' even if ECHO is turned off
  // ICANON - enables canonical input processing
  // IEXTEN - if EOF is preceded by a backslash character, the special character is placed
  // in the input queue without doing the "special character" 
  // processing and the backslash is discarded
  // VTIME - gives the TIME value which is connected with MIN
  // VMIN - gives the MIN value which is connected with TIME
  changedSettings.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN);
  changedSettings.c_cc[VMIN] = 0;
  changedSettings.c_cc[VTIME] = 0;

  // CSIZE - a collection of bits indicating the number of bits per byte
  // CS8 - 8 bites per byte
  // PARENB - Enables parity generation and detection. A parity bit is added to each character on output,
  // and expected from each character on input. 
  changedSettings.c_cflag &= ~(CSIZE | PARENB);
  changedSettings.c_cflag |= CS8;

  // This function will restore settings from STDIN which were set before changes
  if (atexit(backupStdinSettings) != 0) {
    cout << "Cannot backup settings to STDIN_FILENO!" << endl;
    return false;
  }

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &changedSettings)) {
      cout << "Emulator error: Cannot set setings!" << endl;
      return false;
  }
  return true;
}

void Emulator::resetTerminal() {
  backupStdinSettings();
}

void Emulator::readCharFromInput() {
  char c;

  if (read(STDIN_FILENO, &c, 1)) {
    writeToMemory(c, TERM_IN, 2);
    // cout << c << endl;
    setInterruptRequestOnLine(TERMINAL_INDEX_NUM);
  }
}

// ***** Timer *****

// i don't think this works correctly
long long int Emulator::fetchDurationById(short id) {
  //cout << "TIMER PERIOD " << timerPeriod << " ID " << id << endl;
  if (id == 0) return 500000; // 500ms
  else if (id == 1) return 1000000; // 1000ms
  else if (id == 2) return 1500000; // 1500ms
  else if (id == 3) return 2000000; // 2000ms
  else if (id == 4) return 5000000; // 5000ms
  else if (id == 5) return 7000000; // 10s
  else if (id == 6) return 30000000;  // 30s
  else if (id == 7) return 60000000;  // 60s
  return 60000000;
}

void Emulator::resetTimer() {
  timerPeriodId = 0;
  timerPeriod = fetchDurationById(timerPeriodId);
  //cout << "TIMER PERIOD " << timerPeriod << endl;
  timerActive = true;
}

void Emulator::timerTick() {
  currentTime = chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now().time_since_epoch()).count();
  if (timerActive) {
    if (currentTime - previousTime > timerPeriod) {
      //cout << "USAO U IF " << endl;
      timerActive = false;
      setInterruptRequestOnLine(TIMER_INDEX_NUM);
    }
  }
  else {
    timerPeriodId = readFromMemory(TIM_CFG, 2);
    //cout << "TIMER PERIOD ID " << timerPeriodId << endl;
    timerPeriod = fetchDurationById(timerPeriodId);
    //cout << "TIMER PERIOD " << (int)timerPeriod << endl;
    //cout << "FETCH TIME " << fetchDurationById(timerPeriodId) << endl;


    timerActive = true;
    previousTime = currentTime;
  }
}

// ***** Public functions *****

// collect data from linker
bool Emulator::readBinaryFileFromLinker() {
  ifstream inputFile(inputFileName, ios::binary);
  if (inputFile.fail()) {
    cout << "Emulator error: Error while opening file!" << endl;
    return false;
  }
  // get the lines
  int numOfLines = 0;
  inputFile.read((char*)&numOfLines, sizeof(numOfLines));
  //cout << "NUM OF SECTIONS " << numOfLines << endl;
  // iterate through lines
  for (int i = 0; i < numOfLines; i++) {
    int numberOfBytes;
    int va;
    inputFile.read((char*)&va, sizeof(va));
    inputFile.read((char*)&numberOfBytes, sizeof(numberOfBytes));
    //cout << "VA " << hex << (int)va << " NUMOFBYTES " << hex << (int)numberOfBytes << endl;
    // read from linker file and load to memory
    for (int j = 0; j < numberOfBytes; j++) {
      char c;
      inputFile.read((char*)&c, sizeof(c));
      //cout << "DATA " << hex << (int)c << endl;
      if ((j + va) > START_STACK_ADDR) {
        cout << "Emulator error: Virtual address is tried to overwrite reserved places!" << endl;
        inputFile.close();
        return false;
      }
      //cout << "J + VA " << hex << (j + va) << endl;
      memory[j + va] = c;
    }
  }
  // writeMemoryInFile("snapshot1.txt");
  inputFile.close();
  return true;
}

// start emulation
bool Emulator::emulate() {
  rpc = readFromMemory(START_PROGRAM_ADDR * 2, 2);
  rsp = START_STACK_ADDR;
  // cout << "beginning rpc and rsp " << hex << rpc << hex << rsp << endl;
  resetFlag(Tr);
  resetFlag(Tl);
  resetFlag(I);

  resetTimer();
  timerActive = false;

  if (!configureTerminal()) return false;

  running = true;
  int count = 0;
  while(running) {
    prevPC = rpc;
    // if (count++ == 90) break;
    // cout << "-----------------------------" << endl;
    // cout << "RPC " << hex << rpc << endl;
    // cout << "RSP " << hex << rsp << endl;


    if (!instrRecognizer()) {
      cout << "Emulator error: Failed in fetching and decoding instruction!" << endl;
      rpc = prevPC;
      jumpOnInterruptRoutine(ERROR);
    }
    if (!instrExecuter()) {
      cout << "Emulator error: Error in executing instruction!" << endl;
      rpc = prevPC;
      jumpOnInterruptRoutine(ERROR);
    }
    // cout << "r0=0x" << hex << setfill('0') << setw(4) << registers[r0] << "\t" 
    // << "r1=0x" << hex << setfill('0') << setw(4) << registers[r1] << "\t" 
    // << "r2=0x" << hex << setfill('0') << setw(4) << registers[r2] << "\t"
    // << "r3=0x" << hex << setfill('0') << setw(4) << registers[r3] << endl 
    // << "r4=0x" << hex << setfill('0') << setw(4) << registers[r4] << "\t" 
    // << "r5=0x" << hex << setfill('0') << setw(4) << registers[r5] << "\t"
    // << "r6=0x" << hex << setfill('0') << setw(4) << registers[r6] << "\t" 
    // << "r7=0x" << hex << setfill('0') << setw(4) << registers[r7] << endl;
    timerTick();

    readCharFromInput();

    interruptRequestsHandler();
  }
  // setFlag(Tr);
  cout << "-------------------------------------------------" << endl;
  cout << "Emulated processor executed halt instruction" << endl;
  cout << "Emulated processor state: psw=0b";
  short i;
    //cout << "0";
    i = 1 << 15;
    if (rpsw & i) cout << "1";
    else cout << "0";
    for (i = 1 << 14; i > 0; i = i / 2) {
      if((rpsw & i) != 0) cout << "1";
      else cout << "0";
    }
  // int a[16], n = rpsw, i;
  // for(i=0; n>0; i++) {    
  //   a[i]=n%2;    
  //   n= n/2;  
  // }    
  // for(i=i-1 ;i>=0 ;i--) {    
  //   cout<<a[i];    
  // }  
    cout << endl;
  cout << "r0=0x" << hex << registers[r0] << "\t" 
    << "r1=0x" << hex << setfill('0') << setw(4) << registers[r1] << "\t" 
    << "r2=0x" << hex << setfill('0') << setw(4) << registers[r2] << "\t"
    << "r3=0x" << hex << setfill('0') << setw(4) << registers[r3] << endl 
    << "r4=0x" << hex << setfill('0') << setw(4) << registers[r4] << "\t" 
    << "r5=0x" << hex << setfill('0') << setw(4) << registers[r5] << "\t"
    << "r6=0x" << hex << setfill('0') << setw(4) << registers[r6] << "\t" 
    << "r7=0x" << hex << setfill('0') << setw(4) << registers[r7] << endl;

  resetTerminal();
  return true;
}
