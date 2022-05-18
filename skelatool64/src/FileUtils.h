#ifndef _FILE_UTILS_H
#define _FILE_UTILS_H

#include <string>

bool isPathCharacter(char chr);

std::string replaceExtension(const std::string& input, const std::string& newExt);
std::string getBaseName(const std::string& input);

std::string DirectoryName(const std::string& filename);
std::string Join(const std::string& a, const std::string& b);
std::string Relative(const std::string& from, const std::string& to);

std::string NormalizePath(const std::string& path);

bool FileExists(const std::string& path);

#endif