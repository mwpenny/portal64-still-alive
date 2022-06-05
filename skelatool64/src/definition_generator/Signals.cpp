#include "Signals.h"

#include <set>
#include <map>

int Signals::SignalCount() {
    return (int)signalNameToIndex.size();
}

int Signals::SignalIndexForName(const std::string& name) {
    auto result = signalNameToIndex.find(name);

    if (result == signalNameToIndex.end()) {
        int result = (int)signalNameToIndex.size();
        signalNameToIndex[name] = result;
        return result;
    }

    return result->second;
}

void determineSignalOrder(const std::vector<SignalOperation>& input, std::vector<SignalOperation>& output, std::set<std::size_t>& usedSignals, std::map<std::string, std::vector<std::size_t>>& signalProducers, std::size_t current) {
    usedSignals.insert(current);

    const SignalOperation& inputSignal = input[current];

    for (auto& signalName : inputSignal.inputNames) {
        for (auto producerIndex : signalProducers[signalName]) {
            determineSignalOrder(input, output, usedSignals, signalProducers, producerIndex);
        }
    }

    output.push_back(inputSignal);
}

std::vector<SignalOperation> orderSignals(const std::vector<SignalOperation>& signals) {
    std::vector<SignalOperation> result;

    std::set<std::size_t> usedSignals;
    std::map<std::string, std::vector<std::size_t>> signalProducers;

    for (std::size_t i = 0; i < signals.size(); ++i) {
        signalProducers[signals[i].outputName].push_back(i);
    }

    for (std::size_t i = 0; i < signals.size(); ++i) {
        determineSignalOrder(signals, result, usedSignals, signalProducers, i);
    }

    return result;
}

const char* gNamesForSignalTypes[] = {
    "SignalOperatorTypeAnd",
    "SignalOperatorTypeOr",
    "SignalOperatorTypeNot",
    "SignalOperatorTypeTimer",
}; 

std::unique_ptr<DataChunk> generateChunkForSignal(const SignalOperation& signal, Signals& signals, int timerIndex) {
    std::unique_ptr<StructureDataChunk> result(new StructureDataChunk());

    result->AddPrimitive(gNamesForSignalTypes[(int)signal.type]);
    result->AddPrimitive(signals.SignalIndexForName(signal.outputName));

    std::unique_ptr<StructureDataChunk> inputs(new StructureDataChunk());

    inputs->AddPrimitive(signals.SignalIndexForName(signal.inputNames[0]));

    if (signal.type == SignalOperationType::Timer) {
        inputs->AddPrimitive(timerIndex);
    } else if (signal.inputNames.size() > 1) {
        inputs->AddPrimitive(signals.SignalIndexForName(signal.inputNames[1]));
    } else {
        inputs->AddPrimitive(-1);
    }

    result->Add(std::move(inputs));


    std::unique_ptr<StructureDataChunk> data(new StructureDataChunk());

    if (signal.type == SignalOperationType::Timer) {
        data->AddPrimitive("duration", (short)(signal.operand.duration * 16.0f));
    } else {
        std::unique_ptr<StructureDataChunk> additionalInputs(new StructureDataChunk());
        for (unsigned i = 2; i < 4; ++i) {
            if (i < signal.inputNames.size()) {
                additionalInputs->AddPrimitive(signals.SignalIndexForName(signal.inputNames[i]));
            } else {
                additionalInputs->AddPrimitive(-1);
            }
        }
        data->Add("additionalInputs", std::move(additionalInputs));
    }

    result->Add(std::move(data));

    return result;
};

std::unique_ptr<DataChunk> generateSignalData(const std::vector<SignalOperation>& operations, Signals& signals) {
    int timerCount = 0;

    std::unique_ptr<StructureDataChunk> result(new StructureDataChunk());

    for (auto& signal : operations) {
        result->Add(std::move(generateChunkForSignal(signal, signals, timerCount)));
        ++timerCount;
    }

    return result;
}


std::shared_ptr<SignalsOutput> generateSignals(NodeGroups& nodeGroups) {
    std::vector<SignalOperation> operations;
    
    for (auto& nodeInfo : nodeGroups.NodesForType("@and")) {
        if (nodeInfo.arguments.size() <= 1) {
            continue;
        }

        SignalOperation operation;
        operation.type = SignalOperationType::And;
        operation.outputName = nodeInfo.arguments[0];
        operation.inputNames.assign(nodeInfo.arguments.begin() + 1, nodeInfo.arguments.end());

        operations.push_back(operation);
    }

    for (auto& nodeInfo : nodeGroups.NodesForType("@or")) {
        if (nodeInfo.arguments.size() <= 1) {
            continue;
        }

        SignalOperation operation;
        operation.type = SignalOperationType::Or;
        operation.outputName = nodeInfo.arguments[0];
        operation.inputNames.assign(nodeInfo.arguments.begin() + 1, nodeInfo.arguments.end());

        operations.push_back(operation);
    }

    for (auto& nodeInfo : nodeGroups.NodesForType("@not")) {
        if (nodeInfo.arguments.size() <= 1) {
            continue;
        }

        SignalOperation operation;
        operation.type = SignalOperationType::Not;
        operation.outputName = nodeInfo.arguments[0];
        operation.inputNames.push_back(nodeInfo.arguments[1]);

        operations.push_back(operation);
    }

    for (auto& nodeInfo : nodeGroups.NodesForType("@timer")) {
        if (nodeInfo.arguments.size() <= 2) {
            continue;
        }

        SignalOperation operation;
        operation.type = SignalOperationType::Timer;
        operation.outputName = nodeInfo.arguments[0];
        operation.inputNames.push_back(nodeInfo.arguments[1]);

        operation.operand.duration = std::atof(nodeInfo.arguments[2].c_str());

        operations.push_back(operation);
    }

    std::shared_ptr<SignalsOutput> result(new SignalsOutput());

    result->signalOperations = orderSignals(operations);

    return result;      
}

void generateSignalsDefinition(CFileDefinition& fileDefinition, StructureDataChunk& levelDef, const SignalsOutput& signalsOutput, Signals& signals) {
    std::string signalsName = fileDefinition.AddDataDefinition(
        "signal_operations", 
        "struct SignalOperator", 
        true, 
        "_geo", 
        std::move(generateSignalData(signalsOutput.signalOperations, signals))
    );

    levelDef.AddPrimitive("signalOperators", signalsName);
    levelDef.AddPrimitive("signalOperatorCount", signalsOutput.signalOperations.size());
}