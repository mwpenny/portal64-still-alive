#ifndef _DISPLAY_LIST_H
#define _DISPLAY_LIST_H

#include <memory>
#include <vector>
#include <string>
#include <iostream>

#include "./definitions/FileDefinition.h"
#include "./materials/MaterialEnums.h"

enum class DisplayListCommandType {
    COMMENT,
    RAW,
    G_VTX,
    G_TRI1,
    G_TRI2,
    G_MTX,
    G_POPMTX,
    G_DL,
    G_GEOMETRYMODE,
    G_CULLDL,
};

class DisplayList;
class CFileDefinition;

struct DisplayListCommand {
    DisplayListCommand(DisplayListCommandType type);
    virtual ~DisplayListCommand();

    DisplayListCommandType mType;

    virtual std::unique_ptr<DataChunk> GenerateCommand() = 0;
};


struct CommentCommand : DisplayListCommand {
    CommentCommand(std::string comment);

    std::string mComment;

    std::unique_ptr<DataChunk> GenerateCommand();
};

struct RawContentCommand : DisplayListCommand {
    RawContentCommand(std::string content);

    std::string mContent;

    std::unique_ptr<DataChunk> GenerateCommand();
};

struct VTXCommand : DisplayListCommand {
    VTXCommand(
        int numVerts, 
        int indexBufferStart, 
        std::string vertexBuffer,
        int vertexBufferOffset
    );

    int mNumVerts;
    int mIndexBufferStart;
    std::string mVertexBuffer;
    int mVertexBufferOffset;

    std::unique_ptr<DataChunk> GenerateCommand();
};

struct TRI1Command : DisplayListCommand {
    TRI1Command(int a, int b, int c);

    int mA;
    int mB;
    int mC;

    std::unique_ptr<DataChunk> GenerateCommand();
};

struct TRI2Command : DisplayListCommand {
    TRI2Command(int a0, int b0, int c0, int a1, int b1, int c1);

    int mA0;
    int mB0;
    int mC0;

    int mA1;
    int mB1;
    int mC1;

    std::unique_ptr<DataChunk> GenerateCommand();
};

struct CallDisplayListCommand {
    CallDisplayListCommand(int targetDL, int offset);

    int mTargetDL;
    int mOffset;

    std::unique_ptr<DataChunk> GenerateCommand();
};

struct CallDisplayListByNameCommand : DisplayListCommand {
    CallDisplayListByNameCommand(const std::string& dlName);

    std::string mDLName;

    std::unique_ptr<DataChunk> GenerateCommand();
};

struct PushMatrixCommand : DisplayListCommand {
    PushMatrixCommand(unsigned int matrixOffset, bool replace);
    std::unique_ptr<DataChunk> GenerateCommand();

    unsigned int mMatrixOffset;
    bool mReplace;
};

struct PopMatrixCommand : DisplayListCommand {
    PopMatrixCommand(unsigned int popCount);
    std::unique_ptr<DataChunk> GenerateCommand();

    unsigned int mPopCount;
};

struct ChangeGeometryMode : DisplayListCommand {
    ChangeGeometryMode(GeometryMode clear, GeometryMode set);
    std::unique_ptr<DataChunk> GenerateCommand();

    GeometryMode mClear;
    GeometryMode mSet;
};

struct CullDisplayList : DisplayListCommand {
    CullDisplayList(unsigned int vertexCount);
    std::unique_ptr<DataChunk> GenerateCommand();
    unsigned int mVertexCount;
};

class DisplayList {
public:
    DisplayList(std::string name);
    void AddCommand(std::unique_ptr<DisplayListCommand> command);
    StructureDataChunk& GetDataChunk(); 

    const std::string& GetName();
    std::unique_ptr<FileDefinition> Generate(const std::string& fileSuffix);
private:
    std::string mName;
    std::unique_ptr<StructureDataChunk> mDataChunk;
};

#endif