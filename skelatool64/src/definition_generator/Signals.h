#ifndef __DEFINION_SIGANLS_H__
#define __DEFINION_SIGANLS_H__

#include "./DefinitionGenerator.h"

#include <map>
#include <string>
#include <memory>
#include <assimp/scene.h>

struct Signals {
    std::map<std::string, int> signalNameToIndex;
};

std::unique_ptr<Signals> findSignals(const aiScene* scene);

#endif