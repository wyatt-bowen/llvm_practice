#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include <string>
#include <fstream>
#include <unordered_map>
#include <set>
#include <queue>

using namespace llvm;
using namespace std;

#define DEBUG_TYPE "CSElimination"

namespace
{
  llvm::IRBuilder<> builder(context);
  // llvm::Module module("MyModule", context);
  struct ValueNumberAndName
  {
    int value;
    Value *name;
    ValueNumberAndName(int value, Value *name)
    {
      value = value;
      name = name;
    }
    ValueNumberAndName(int value)
    {
      value = value;
    }
    ValueNumberAndName(Value *name)
    {
      name = name;
    }
    string getName()
    {
      return string(name->getName());
    }
  };
  string getOperandString(Instruction inst, int operandNum = 0)
  {
    return "WIP";
  }

  struct CSElimination : public FunctionPass
  {
    static char ID;
    CSElimination() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override
    {
      for (auto &basic_block : F)
      {
        int nextValueNum = 0;
        unordered_map<string, ValueNumberAndName> table;
        for (auto &instruction : basic_block)
        {
          // Operations
          // All Binary instructions, the differences btwn add sub mult and div are only in the key string built
          string keyString; // string that contains the key for this subexpression
          // operand 1 section
          string operand1 = string(instruction.getOperand(0)->getName());
          auto op1Location = table.find(operand1);
          int op1ValueNum;
          if (op1Location == table.end())
          {
            op1ValueNum = nextValueNum++;
            table[operand1].value = op1ValueNum;
          }
          else
          {
            op1ValueNum = op1Location->second.value;
          }
          // end operand 1 section
          // operand 2 section
          string operand2 = string(instruction.getOperand(1)->getName());
          auto op2Location = table.find(operand2);
          int op2ValueNum;
          if (op2Location == table.end())
          {
            op2ValueNum = nextValueNum++;
            table[operand2].value = op2ValueNum;
          }
          else
          {
            op2ValueNum = op2Location->second.value;
          }
          // end operand 2 section
          // build the key string
          keyString += "<" + to_string(op1ValueNum);
          // Instruction opcodes
          switch (instruction.getOpcode())
          {
          case Instruction::Add:
            keyString += "+";
            break;
          case Instruction::Sub:
            keyString += "-";
            break;
          case Instruction::Mul:
            keyString += "*";
            break;
          case Instruction::SDiv:
            keyString += "/";
            break;
          default:
            keyString += "uhh";
          }

          // string operationResult = string(instruction.getName());
          Value *operationResult = &instruction;
          // Now look for the keyString in the table
          auto keyStringLocation = table.find(keyString);
          // If not found: add 1 thing to the table
          // The key for the keyString will get the a new valueNum,
          // and the name of the operationResult variable.
          if (keyStringLocation == table.end() || (keyStringLocation != table.end() && table[keyString].value != table[table[keyString].getName()].value))
          { // (Not found) OR (Found AND keystring's value != the variables current value), add to table, don't change instruction
            table[keyString] = ValueNumberAndName(nextValueNum++, operationResult);
          }
          else
          { // Found an existing entry of
            // Value *val =
            //     Instruction *storeInst = builder.CreateAlignedStore()
            Instruction *storeInst = builder.CreateStore(table[keyString].name, operationResult);
            instruction.replaceAllUsesWith(storeInst);
          }

          // No matter what: The key for the variable being assigned to will have a new valueNum = keyStrings valueNum
          table[string(operationResult->getName())].value = table[keyString].value;
        }
      }
      return true; // Indicate this is a Transform pass
    }
  }; // end of struct CSElimination

} // end of anonymous namespace

char CSElimination::ID = 0;
static RegisterPass<CSElimination> X("CSElimination", "CSElimination Pass",
                                     false /* Only looks at CFG */,
                                     true /* Tranform Pass */);
