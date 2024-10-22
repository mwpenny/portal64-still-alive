#ifndef _STRING_UTILS_H
#define _STRING_UTILS_H

#include <string>
#include <vector>

std::string FindAndReplace(const std::string& source, const std::string& searchString, const std::string& replaceString, bool wholeWord = false);

std::string Indent(const std::string& input, const std::string& whitespace);

std::string Trim(const std::string& input);

void makeCCompatible(std::string& target);

bool StartsWith(const std::string& input, const std::string& prefix);

bool EndsWith(const std::string& input, const std::string& suffix);

void SplitString(const std::string& input, char delimeter, std::vector<std::string>& output);

#endif