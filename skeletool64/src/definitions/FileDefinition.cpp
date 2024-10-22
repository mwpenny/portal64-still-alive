#include "FileDefinition.h"
#include "../StringUtils.h"

FileDefinition::FileDefinition(const std::string& type, const std::string& name, bool isArray, std::string location) :
    mType(type),
    mName(name),
    mIsArray(isArray),
    mLocation(location),
    mForResource(NULL) {

}

FileDefinition::FileDefinition(const std::string& type, const std::string& name, bool isArray, std::string location, const void* forResource) :
    mType(type),
    mName(name),
    mIsArray(isArray),
    mLocation(location),
    mForResource(forResource) {

}

FileDefinition::~FileDefinition() {

}

void FileDefinition::GenerateDeclaration(std::ostream& output) {
    output << "extern " << mType << " " << mName;

    if (mIsArray) {
        output << "[]";
    }
}

std::string FileDefinition::GetLocation() {
    return mLocation;
}

void FileDefinition::AddTypeHeader(const std::string& typeHeader) {
    mTypeHeaders.insert(typeHeader);
}

const std::set<std::string>& FileDefinition::GetTypeHeaders() {
    return mTypeHeaders;
}

const void* FileDefinition::ForResource() const {
    return mForResource;
}

const std::string& FileDefinition::GetName() const {
    return mName;
}

DataFileDefinition::DataFileDefinition(const std::string& type, const std::string& name, bool isArray, std::string location, std::unique_ptr<DataChunk> data):
    FileDefinition(type, name, isArray, location),
    mData(std::move(data)) {

}

DataFileDefinition::DataFileDefinition(const std::string& type, const std::string& name, bool isArray, std::string location, std::unique_ptr<DataChunk> data, const void* forResource):
    FileDefinition(type, name, isArray, location, forResource),
    mData(std::move(data)) {

}

void DataFileDefinition::Generate(std::ostream& output) {
    int start = (int)output.tellp();

    output << mType << " " << mName;

    if (mIsArray) {
        output << "[]";
    }

    output << " = ";

    mData->Output(output, 0, (int)output.tellp() - start);
}

RawFileDefinition::RawFileDefinition(const std::string& type, const std::string& name, bool isArray, std::string location, const std::string& content) : FileDefinition(type, name, isArray, location), mContent(content) {}

void RawFileDefinition::Generate(std::ostream& output) {
    output << mType << " " << mName;

    if (mIsArray) {
        output << "[] = {" << std::endl;
        output << Indent(mContent, "    ");
        output << "};";
    } else {
        output << " = " << mContent << ";";
    }
}