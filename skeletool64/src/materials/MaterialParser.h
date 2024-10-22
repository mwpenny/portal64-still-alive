#ifndef _MATERIAL_PARSER_H
#define _MATERIAL_PARSER_H

#include "Material.h"
#include <istream>
#include <map>
#include <string>
#include <vector>

struct MaterialFile {
public:
    std::map<std::string, std::shared_ptr<Material>> mMaterials;
};

struct ParseError {
    ParseError(const std::string& message);
    std::string mMessage;
};

struct ParseResult {
    ParseResult(const std::string& insideFolder);
    std::string mInsideFolder;
    std::string mForcePallete;
    bool mTargetCIBuffer;
    MaterialFile mMaterialFile;
    std::vector<ParseError> mErrors;
};

void parseMaterialFile(std::istream& input, ParseResult& output);

#endif