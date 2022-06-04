#include "./DisplayList.h"

#include "./CFileDefinition.h"

#include <sstream>

DisplayListCommand::DisplayListCommand(DisplayListCommandType type): mType(type) {}

DisplayListCommand::~DisplayListCommand() {}

CommentCommand::CommentCommand(std::string comment):
    DisplayListCommand(DisplayListCommandType::COMMENT),
    mComment(comment) {

}

std::unique_ptr<DataChunk> CommentCommand::GenerateCommand() {
    return std::unique_ptr<DataChunk>(new CommentDataChunk(mComment));
}

RawContentCommand::RawContentCommand(std::string content):
    DisplayListCommand(DisplayListCommandType::RAW),
    mContent(content) {

}

std::unique_ptr<DataChunk> RawContentCommand::GenerateCommand() {
    return std::unique_ptr<DataChunk>(new PrimitiveDataChunk<std::string>(mContent));
}

VTXCommand::VTXCommand(
        int numVerts, 
        int indexBufferStart, 
        std::string vertexBuffer,
        int vertexBufferOffset
    ) :
    DisplayListCommand(DisplayListCommandType::G_VTX),
    mNumVerts(numVerts),
    mIndexBufferStart(indexBufferStart),
    mVertexBuffer(vertexBuffer),
    mVertexBufferOffset(vertexBufferOffset) {

    }

std::unique_ptr<DataChunk> VTXCommand::GenerateCommand() {
    std::unique_ptr<MacroDataChunk> result(new MacroDataChunk("gsSPVertex"));

    std::string vertexIndex = std::string("&") + 
        mVertexBuffer +
        "[" + std::to_string(mVertexBufferOffset) + "]";

    result->AddPrimitive(vertexIndex);
    result->AddPrimitive(mNumVerts);
    result->AddPrimitive(mIndexBufferStart);
    
    return std::move(result);
}

TRI1Command::TRI1Command(int a, int b, int c) :
    DisplayListCommand(DisplayListCommandType::G_TRI1),
    mA(a),
    mB(b),
    mC(c) {

    }

std::unique_ptr<DataChunk> TRI1Command::GenerateCommand() {
    std::unique_ptr<MacroDataChunk> result(new MacroDataChunk("gsSP1Triangle"));

    result->AddPrimitive(mA);
    result->AddPrimitive(mB);
    result->AddPrimitive(mC);
    result->AddPrimitive(0);
    
    return std::move(result);
}

TRI2Command::TRI2Command(int a0, int b0, int c0, int a1, int b1, int c1) :
    DisplayListCommand(DisplayListCommandType::G_TRI2),
    mA0(a0),
    mB0(b0),
    mC0(c0),
    mA1(a1),
    mB1(b1),
    mC1(c1) {

    }

std::unique_ptr<DataChunk> TRI2Command::GenerateCommand() {    
    std::unique_ptr<MacroDataChunk> result(new MacroDataChunk("gsSP2Triangles"));

    result->AddPrimitive(mA0);
    result->AddPrimitive(mB0);
    result->AddPrimitive(mC0);
    result->AddPrimitive(0);

    result->AddPrimitive(mA1);
    result->AddPrimitive(mB1);
    result->AddPrimitive(mC1);
    result->AddPrimitive(0);
    
    return std::move(result);
}

CallDisplayListByNameCommand::CallDisplayListByNameCommand(const std::string& dlName): 
    DisplayListCommand(DisplayListCommandType::G_DL),
    mDLName(dlName) {

}

std::unique_ptr<DataChunk> CallDisplayListByNameCommand::GenerateCommand() {
    std::unique_ptr<MacroDataChunk> result(new MacroDataChunk("gsSPDisplayList"));

    result->AddPrimitive(mDLName);
    
    return std::move(result);
}

PushMatrixCommand::PushMatrixCommand(unsigned int matrixOffset, bool replace): 
    DisplayListCommand(DisplayListCommandType::G_MTX),
    mMatrixOffset(matrixOffset),
    mReplace(replace) {

}

std::unique_ptr<DataChunk> PushMatrixCommand::GenerateCommand() {
    std::unique_ptr<MacroDataChunk> result(new MacroDataChunk("gsSPMatrix"));

    std::string matrixSegment = std::string("(Mtx*)MATRIX_TRANSFORM_SEGMENT_ADDRESS + ") + std::to_string(mMatrixOffset);

    result->AddPrimitive(matrixSegment);

    std::string flags = "G_MTX_MODELVIEW | G_MTX_MUL | ";

    if (mReplace) {
        flags += "G_MTX_NOPUSH";
    } else {
        flags += "G_MTX_PUSH";
    }

    result->AddPrimitive(flags);
    
    return std::move(result);
}

PopMatrixCommand::PopMatrixCommand(unsigned int popCount): 
    DisplayListCommand(DisplayListCommandType::G_POPMTX),
    mPopCount(popCount) {
        
}

std::unique_ptr<DataChunk> PopMatrixCommand::GenerateCommand() {
    if (mPopCount == 0) {
        return std::unique_ptr<DataChunk>(new DataChunkNop());
    }

    if (mPopCount > 1) {
        std::unique_ptr<MacroDataChunk> result(new MacroDataChunk("gsSPPopMatrixN"));

        result->AddPrimitive<const char*>("G_MTX_MODELVIEW");
        result->AddPrimitive(mPopCount);

        return std::move(result);
    }
    
    std::unique_ptr<MacroDataChunk> result(new MacroDataChunk("gsSPPopMatrix"));

    result->AddPrimitive<const char*>("G_MTX_MODELVIEW");

    return std::move(result);
}

std::string generateGeometryMode(GeometryMode mode) {
    std::string result = "";

    return result;
}

ChangeGeometryMode::ChangeGeometryMode(GeometryMode clear, GeometryMode set): 
    DisplayListCommand(DisplayListCommandType::G_GEOMETRYMODE), 
    mClear(clear),
    mSet(set) {
}

std::unique_ptr<DataChunk> ChangeGeometryMode::GenerateCommand() {
    std::unique_ptr<MacroDataChunk> result(new MacroDataChunk("gsSPGeometryMode"));

    result->AddPrimitive(generateGeometryMode(mClear));
    result->AddPrimitive(generateGeometryMode(mSet));

    return std::move(result);
}

CullDisplayList::CullDisplayList(unsigned int vertexCount): 
    DisplayListCommand(DisplayListCommandType::G_CULLDL), 
    mVertexCount(vertexCount) {
    
}

std::unique_ptr<DataChunk> CullDisplayList::GenerateCommand() {
    std::unique_ptr<MacroDataChunk> result(new MacroDataChunk("gsSPCullDisplayList"));

    result->AddPrimitive(mVertexCount - 1);

    return std::move(result);
}

DisplayList::DisplayList(std::string name):
    mName(name),
    mDataChunk(new StructureDataChunk()) {
    
}

void DisplayList::AddCommand(std::unique_ptr<DisplayListCommand> command) {
    mDataChunk->Add(std::move(command->GenerateCommand()));
}

StructureDataChunk& DisplayList::GetDataChunk() {
    return *mDataChunk;
}

const std::string& DisplayList::GetName() {
    return mName;
}

std::unique_ptr<FileDefinition> DisplayList::Generate(const std::string& fileSuffix) {
    mDataChunk->Add(std::unique_ptr<DataChunk>(new MacroDataChunk("gsSPEndDisplayList")));

    std::unique_ptr<FileDefinition> result(new DataFileDefinition(
        std::string("Gfx"), 
        mName, 
        true, 
        fileSuffix, 
        std::move(mDataChunk)
    ));

    result->AddTypeHeader("<ultra64.h>");

    return result;
}