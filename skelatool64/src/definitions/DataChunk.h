#ifndef __DATA_CHUNK_H__
#define __DATA_CHUNK_H__

#include <ostream>
#include <sstream>
#include <memory>
#include <vector>

#include <assimp/vector3.h>
#include <assimp/quaternion.h>

class DataChunk {
public:
    DataChunk();
    virtual ~DataChunk();

    virtual bool Output(std::ostream& output, int indentLevel, int linePrefix) = 0;

    int GetEstimatedLength();
protected:
    virtual int CalculateEstimatedLength() = 0;
private:
    int mCachedLength;
};

class DataChunkNop : public DataChunk {
public:
    DataChunkNop();

    virtual bool Output(std::ostream& output, int indentLevel, int linePrefix);
protected:
    virtual int CalculateEstimatedLength();
};

template <typename T>
class PrimitiveDataChunk : public DataChunk {
public:
    PrimitiveDataChunk(const T& value): DataChunk(), mValue(value) {}

    virtual bool Output(std::ostream& output, int indentLevel, int linePrefix) {
        output << mValue;
        return true;
    }
protected:
    virtual int CalculateEstimatedLength() {
        std::ostringstream tmp;
        Output(tmp, 0, 0);
        return tmp.tellp();
    }
private:
    T mValue;
};

class StringDataChunk : public PrimitiveDataChunk<std::string> {
public:
    StringDataChunk(const std::string& value);
private:
    static char EscapeCharacter(char input);
    static std::string EscapeAndWrapString(const std::string& string);
};

class StructureEntryDataChunk : public DataChunk {
public:
    StructureEntryDataChunk(const std::string& name, std::unique_ptr<DataChunk> entry);

    virtual bool Output(std::ostream& output, int indentLevel, int linePrefix);
protected:
    virtual int CalculateEstimatedLength();
private:
    std::string mName;
    std::unique_ptr<DataChunk> mEntry;
};

class StructureDataChunk : public DataChunk {
public:
    StructureDataChunk();
    StructureDataChunk(const aiVector3D& vector);
    StructureDataChunk(const aiQuaternion& quat);

    void Add(std::unique_ptr<DataChunk> entry);

    template <typename T>
    void AddPrimitive(const T& primitive) {
        Add(std::unique_ptr<DataChunk>(new PrimitiveDataChunk<T>(primitive)));
    }

    void Add(const std::string& name, std::unique_ptr<DataChunk> entry);

    template <typename T>
    void AddPrimitive(const std::string& name, const T& primitive) {
        Add(std::unique_ptr<DataChunk>(new StructureEntryDataChunk(
            name,
            std::unique_ptr<DataChunk>(new PrimitiveDataChunk<T>(primitive))
        )));
    }

    virtual bool Output(std::ostream& output, int indentLevel, int linePrefix);
    
    static void OutputIndent(std::ostream& output, int indentLevel);
    static void OutputChildren(std::vector<std::unique_ptr<DataChunk>>& children, std::ostream& output, int indentLevel, int totalLength, bool trailingComma);
protected:
    virtual int CalculateEstimatedLength();
private:
    std::vector<std::unique_ptr<DataChunk>> mChildren;
};

class MacroDataChunk : public DataChunk {
public:
    MacroDataChunk(const std::string& macroName);
    MacroDataChunk(const std::string& macroName, bool singleLine);

    void Add(std::unique_ptr<DataChunk> entry);

    template <typename T>
    void AddPrimitive(const T& primitive) {
        Add(std::unique_ptr<DataChunk>(new PrimitiveDataChunk<T>(primitive)));
    }

    virtual bool Output(std::ostream& output, int indentLevel, int linePrefix);
protected:
    virtual int CalculateEstimatedLength();

private:
    std::string mMacroName;
    std::vector<std::unique_ptr<DataChunk>> mParameters;
    bool mSingleLine;
};

class CommentDataChunk : public DataChunk {
public:
    CommentDataChunk(const std::string& comment);

    virtual bool Output(std::ostream& output, int indentLevel, int linePrefix);
protected:
    virtual int CalculateEstimatedLength();
private:
    std::string mComment;
};

#endif