#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/CFG.h"
#include <string>
#include <fstream>
#include <unordered_map>
#include <set>
#include <queue>

using namespace llvm;
using namespace std;

#define DEBUG_TYPE "LivenessAnalysis"

namespace
{
  template <typename T>
  std::set<T> getUnion(const std::set<T> &a, const std::set<T> &b)
  {
    std::set<T> result = a;
    result.insert(b.begin(), b.end());
    return result;
  }

  template <typename T>
  std::set<T> getIntersection(const std::set<T> &a, const std::set<T> &b)
  {
    std::set<T> result;
    for (const auto &element : a)
    {
      if (b.count(element) > 0)
      {
        result.insert(element);
      }
    }
    return result;
  }

  template <typename T>
  std::set<T> getSetDifference(const std::set<T> &a, const std::set<T> &b)
  {
    std::set<T> result;
    for (const auto &element : a)
    {
      if (b.count(element) == 0)
      {
        result.insert(element);
      }
    }
    return result;
  }

  std::string stringSetString(std::set<std::string> ss)
  {
    if (ss.size() == 0)
      return "{}";
    std::string sss = "{";
    for (const auto &element : ss)
    {
      sss += element + " ";
      // errs() << element;
    }
    sss += "\b}";

    return sss;
  }

  // std::set<std::string> vars;
  std::unordered_map<std::string, std::set<std::string>> LiveOut;
  std::unordered_map<std::string, std::set<std::string>> UEVar;
  std::unordered_map<std::string, std::set<std::string>> VarKill;

  struct LivenessAnalysis : public FunctionPass
  {
    static char ID;
    LivenessAnalysis() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override
    {
      errs() << "LivenessAnalysis: ";
      errs() << F.getName() << "\n";

      // reverse start
      // auto bbList = &(F.getBasicBlockList()); // fetch the pointer of the list

      // // errs() << "reverse  \n";
      // errs() << "Basic block names in reverse hopefully: ";

      // for (auto bb = bbList->rbegin(); bb != bbList->rend(); bb++)
      // {

      //   BasicBlock *b = &(*bb);

      //   errs() << b->getName() << ", ";
      // }
      // errs() << "\n";
      // reverse end

      // normal traversal
      // for (auto &basic_block : F)
      // {
      //   errs() << "Basic block name: " << basic_block.getName() << "\n";
      //   for (auto &instruction : basic_block)
      //   {
      //   }
      // }
      // normal traversal end

      // actual algo!
      // step 2 on paper
      for (auto &basic_block : F)
      {
        string bbName = string(basic_block.getName());
        std::set<std::string> _UEVar;
        std::set<std::string> _VarKill;
        for (auto &instruction : basic_block)
        {
          if (instruction.getOpcode() == Instruction::Load)
          {
            std::string ident = std::string(instruction.getOperand(0)->getName());
            if (_VarKill.find(ident) == _VarKill.end())
              _UEVar.insert(ident);
          }
          if (instruction.getOpcode() == Instruction::Store)
          {
            std::string ident = std::string(instruction.getOperand(1)->getName());
            _VarKill.insert(ident);
          }
        } // end for each instruction
        // errs() << basic_block.getName() << "'s UEVar: ";
        // for (const auto &element : _UEVar)
        // {
        //   errs() << element << " ";
        // }
        // errs() << "\n";

        // errs() << basic_block.getName() << "'s VarKill: ";
        // for (const auto &element : _VarKill)
        // {
        //   errs() << element << " ";
        // }
        // errs() << "\n";
        UEVar[bbName] = _UEVar; // one of the last lines
        VarKill[bbName] = _VarKill;
      } // end for basic block
      // end for step 2.

      // step 3: Solve liveout eqns
      for (auto &basic_block : F)
      {
        LiveOut[string(basic_block.getName())].clear();
      }
      bool changed = true;
      while (changed)
      {
        changed = false;
        for (auto &basic_block : F)
        {
          std::string bbName = std::string(basic_block.getName());
          std::set<std::string> before = LiveOut[string(basic_block.getName())]; // getting LiveOut set before changes
          std::set<std::string> after;
          for (auto succ : successors(&basic_block))
          {
            std::string succName = std::string(succ->getName());
            std::set<std::string> _UEVar = UEVar[succName];
            std::set<std::string> _VarKill = VarKill[succName];
            std::set<std::string> _LiveOut = LiveOut[succName];

            // errs() << "Set difference in " << bbName << "'s successor " << succName << "\n";
            std::set<std::string> intersectionSet = getSetDifference(_LiveOut, _VarKill);
            // errs() << stringSetString(intersectionSet) << "} = {" << stringSetString(_LiveOut) << " - " << stringSetString(_VarKill) << "\n";

            // errs() << "Union in " << bbName << "'s successor " << succName << "\n";
            std::set<std::string> unionSet = getUnion(_UEVar, intersectionSet);
            // errs() << stringSetString(unionSet) << " = " << stringSetString(_UEVar) << " U " << stringSetString(intersectionSet) << "\n";

            // Union with after
            // errs() << "Union with after in " << bbName << "'s successor " << succName << "\n";
            std::set<std::string> beforeaftergetschanged = after;
            after = getUnion(beforeaftergetschanged, unionSet);
            // errs() << stringSetString(after) << " = " << stringSetString(beforeaftergetschanged) << " U " << stringSetString(unionSet) << "\n\n";
          } // end for successors
          if (before != after)
            changed = true;
          // errs() << "LiveOut of " << bbName << ": " << stringSetString(after) << "\n\n\n";
          LiveOut[bbName] = after;
        } // end for each block
      }   // end while
      // end step 3

      // ONE LAST LOOP FOR EACH BLOCK TO PRINT INFO
      for (auto &basic_block : F)
      {
        string bbName = string(basic_block.getName());
        errs() << "BasicBlock " << bbName << ":\n";
        //       << "UEVar: " << stringSetString(UEVar[bbName]) << "\n";

        // errs() << "\nVarKill: " << stringSetString(VarKill[bbName]) << "\n";

        errs() << "LiveOut: " << stringSetString(LiveOut[bbName]) << "\n\n";

        // errs() << "\n";
      }

      return false;
    }
  }; // end of struct LivenessAnalysis
} // end of anonymous namespace

char LivenessAnalysis::ID = 0;
static RegisterPass<LivenessAnalysis> X("LivenessAnalysis", "LivenessAnalysis Pass",
                                        false /* Only looks at CFG */,
                                        false /* Analysis Pass */);
