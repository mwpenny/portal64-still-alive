#include "Signals.h"

std::map<std::string, int> gSignalNameIndexMapping = {
    {"@button", 0},
    {"@door", 0},
};

std::unique_ptr<Signals> findSignals(const aiScene* scene) {
    return nullptr;
}