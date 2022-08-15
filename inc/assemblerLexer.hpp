#ifndef ASSEMBLERLEXER_H
#define ASSEMBLERLEXER_H
#include <regex>
#include <string>
using namespace std;

// ***** Basic details *****

regex any_symbol("(.*)");

// regexes for removing comments in input file
regex removeComment("([^#]*)#.*");
//regex comment("^#.*$");

regex removeTabs("[\\t]+");
regex removeExtraSpaces(" {2,}");

regex removeBoundarySpaces("^( *)([^ ].*[^ ])( *)$");
regex findCommaSpaces(" ?, ?");
regex findColumnsSpaces(" ?: ?");


string decLiteral = "-?[0-9]+";
string hexLiteral = "0x[0-9A-F]+";
// string octLiteral = "0[0-8]*";
string symbol = "[a-zA-Z][a-zA-Z0-9_]*";
string symbol2 = "[ a-zA-Z0-9_]+";
string strSymbol = "^('(.*)')$";
string strSymbol2 = "^(\"(.*)\")$";

string literal = decLiteral + "|" + hexLiteral;

// ***** Assembly directives *****

// if i need to escape . in C++ i need two \\ not one
regex globalDir("^\\.global (" + symbol + "(," + symbol + ")*)$");
regex externDir("^\\.extern (" + symbol + "(," + symbol + ")*)$");
regex sectionDir("^\\.section (" + symbol + ")$");
regex wordDir("^\\.word ((" + symbol + "|" + literal + ")(,(" + symbol + "|" + literal + "))*)$");
regex skipDir("^\\.skip (" + decLiteral + "|" + hexLiteral + ")$");
regex asciiDir("^\\.ascii ['\"](" + symbol2 + ")['\"]$");
regex equDir("^\\.equ (" + symbol + "),(" + decLiteral + "|" + hexLiteral + ")$");
regex endDir("^\\.end$");

regex sym("^(" + symbol + ")$");
regex literalDecimal("^(" + decLiteral + ")$");
regex literalHexadecimal("^(" + hexLiteral + ")$");

regex label("^(" + symbol + "):$");           
regex labelCommand("(" + symbol + "):(.*)");

// ***** Assembly instructions *****

// instructions with registers
regex noOperandInstr("^(halt|iret|ret)$");
regex oneOperandInstr("^(int|push|pop|not) (r[0-7]|psw)$");
regex twoOperandInstr("^(xchg|add|sub|mul|div|cmp|and|or|xor|test|shl|shr) (r[0-7]|psw),(r[0-7]|psw)$");

// jump operand
regex jumps("^(call|jmp|jeq|jne|jgt) (.*)$");
// load/store
regex loadStore("^(ldr|str) (r[0-7]|psw),(.*)$");


// syntax notation for operand in instruction for load/store
// value <literal> or <symbol>
regex loadStoreAbsolute("^\\$(" + symbol + "|" + literal + ")$");
// value from address which is in symbolLiteral [<literal>] or [<symbol>]
regex loadStoreMemdir("^(" + symbol + "|" + literal + ")$");
regex loadStorePCRelative("^%(" + symbol + ")$");
// value in register <reg>
regex loadStoreRegdir("^(r[0-7]|psw)$");
// value from memory which is in register [<reg>]
regex loadStoreRegind("^\\[(r[0-7]|psw)\\]$");
// value from memory which is in register + displacement [<reg> + <literal>]
regex loadStoreRegingWithDisplacement("^\\[(r[0-7]|psw) ?\\+ ?(" + symbol + "|" + literal + ")\\]$");

regex jmpAbsolute("^(" + symbol + "|" + literal + ")$");
regex jmpPCRelative("^%(" + symbol + ")$");
regex jmpMemdir("^\\*(" + symbol + "|" + literal + ")$");
// value in register *<reg>
regex jmpRegdir("^\\*(r[0-7]|psw)$");
// value from memory which is in register *[<reg>]
regex jmpRegind("^\\*\\[(r[0-7]|psw)\\]$");
// value from memory which is in register + displacement *[<reg> + <literal>]
regex jmpRegindWithDisplacement("^\\*\\[(r[0-7]|psw) ?\\+ ?(" + symbol + "|" + literal + ")\\]$");

#endif // ASSEMBLERLEXER_H