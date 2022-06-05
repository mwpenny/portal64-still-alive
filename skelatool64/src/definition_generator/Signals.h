#ifndef __DEFINION_SIGANLS_H__
#define __DEFINION_SIGANLS_H__

#include "./DefinitionGenerator.h"

#include <map>
#include <string>
#include <memory>
#include <assimp/scene.h>

enum class SignalOperationType {
    And,
    Or,
    Not,
    Timer
};

struct SignalOperation {
    SignalOperationType type;
    std::string outputName;
    std::vector<std::string> inputNames;

    union {
        float duration;
    } operand;
};

class Signals {
public: 
    int SignalCount();
    int SignalIndexForName(const std::string& name);
private:
    std::map<std::string, int> signalNameToIndex;
};

struct SignalsOutput {
    std::vector<SignalOperation> signalOperations;    
};

std::shared_ptr<SignalsOutput> generateSignals(NodeGroups& nodeGroups);

std::vector<SignalOperation> orderSignals(const std::vector<SignalOperation>& signals);

void generateSignalsDefinition(CFileDefinition& fileDefinition, StructureDataChunk& levelDef, const SignalsOutput& signalsOutput, Signals& signals);

#endif