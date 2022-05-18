
#include "FileUtils.h"
#include <math.h>
#include <vector>
#include <sstream>
#include <iostream>
#include <unistd.h>
#include <fstream>

std::string gCwd;

#define MAX_PATH    256

const std::string& GetCwd() {
    if (!gCwd.length()) {
        char buffer[MAX_PATH];
        if (getcwd(buffer, MAX_PATH)) {
            gCwd = buffer;
        }
    }

    return gCwd;
}

bool isPathCharacter(char chr) {
    return chr == '\\' || chr == '/';
}

std::string replaceExtension(const std::string& input, const std::string& newExt) {
    std::size_t extPos = input.rfind('.');

    if (extPos == std::string::npos) {
        return input + newExt;
    } else {
        return input.substr(0, extPos) + newExt;
    }
}

std::string getBaseName(const std::string& input) {
    std::size_t pathPos = input.rfind('/');
    std::size_t wrongPathPos = input.rfind('\\');

    if (wrongPathPos != std::string::npos && pathPos != std::string::npos) {
        pathPos = std::max(pathPos, wrongPathPos);
    } else if (wrongPathPos != std::string::npos) {
        pathPos = wrongPathPos;
    }

    if (pathPos == std::string::npos) {
        return input;
    } else {
        return input.substr(pathPos + 1);
    }
}

std::string DirectoryName(const std::string& filename) {
    std::size_t correctSlash = filename.rfind('/');
    std::size_t wrongSlash = filename.rfind('\\');

    if (correctSlash != std::string::npos && wrongSlash != std::string::npos) {
        correctSlash = std::max(correctSlash, wrongSlash);
    }

    if (correctSlash == std::string::npos) {
        return "";
    }

    return filename.substr(0, correctSlash);
}

std::size_t nextPathCharacter(const std::string& input, std::size_t curr) {
    std::size_t correctPath = input.find('/', curr);
    std::size_t wrongPath = input.find('\\', curr);

    if (correctPath != std::string::npos && wrongPath != std::string::npos) {
        return std::min(correctPath, wrongPath);
    } else if (correctPath != std::string::npos) {
        return correctPath;
    } else {
        return wrongPath;
    }
}

void separatePath(const std::string& input, std::vector<std::string>& output) {
    std::size_t curr = 0;

    do {
        std::size_t next = nextPathCharacter(input, curr);

        if (next == std::string::npos) {
            output.push_back(input.substr(curr));
            curr = std::string::npos;
        } else {
            output.push_back(input.substr(curr, next - curr));
            curr = next + 1;
        }

    } while (curr != std::string::npos);
}

std::string Join(const std::string& a, const std::string& b) {
    if (b.length() == 0) {
        return a;
    }

    if (isPathCharacter(b[0])) {
        return b;
    }

    std::vector<std::string> parts;

    separatePath(a, parts);
    separatePath(b, parts);

    std::vector<std::string> normalizedParts;

    int skipCount = 0;

    for (int i = parts.size() - 1; i >= 0; --i) {
        if (parts[i] == "..") {
            ++skipCount;
        } else if (parts[i] == ".") {
            continue;
        } else if (skipCount) {
            --skipCount;
        } else if (parts[i] != ".") {
            normalizedParts.push_back(parts[i]);
        }
    }

    std::ostringstream result;

    for (int i = normalizedParts.size() - 1; i >= 0; --i) {
        if (i != (int)normalizedParts.size() - 1) {
            result << "/";
        }

        result << normalizedParts[i];
    }

    return result.str();
}

std::vector<std::string> SplitOnFirstPath(const std::string& path) {
    std::size_t lastStart = 0;

    std::vector<std::string> result;

    bool hasMore = true;

    while (hasMore) {
        std::size_t pathPos = path.find('/', lastStart);
        std::size_t wrongPathPos = path.find('\\', lastStart);

        if (pathPos != std::string::npos && wrongPathPos != std::string::npos) {
            pathPos = std::min(pathPos, wrongPathPos);
        }
        
        if (pathPos == std::string::npos) {
            hasMore = false;
            result.push_back(path.substr(lastStart));
        } else {
            if (pathPos != lastStart) {
                result.push_back(path.substr(lastStart, pathPos - lastStart));
            }
            lastStart = pathPos + 1;
        }
    }

    return result;
}

std::string Relative(const std::string& from, const std::string& to) {
    std::vector<std::string> fromPathSplit = SplitOnFirstPath(DirectoryName(from));
    std::vector<std::string> toPathSplit = SplitOnFirstPath(to);

    unsigned commonStart = 0;

    while (commonStart < fromPathSplit.size() && commonStart < toPathSplit.size() && fromPathSplit[commonStart] == toPathSplit[commonStart]) {
        ++commonStart;
    }

    std::ostringstream result;

    for (unsigned i = commonStart; i < fromPathSplit.size(); ++i) {
        result << "../";
    }

    for (unsigned i = commonStart; i < toPathSplit.size(); ++i) {
        result << toPathSplit[i];

        if (i+1 != toPathSplit.size()) {
            result << '/';
        }
    }

    return result.str();
}

std::string NormalizePath(const std::string& path) {
    return Join(GetCwd(), path);
}

bool FileExists(const std::string& path) {
    std::ifstream tmp;
    tmp.open(path);

    bool result = (bool)tmp;

    if (result) {
        tmp.close();
    }

    return result;
}