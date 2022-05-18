#include "DataChunk.h"

DataChunk::DataChunk(): mCachedLength(0) {}
DataChunk::~DataChunk() {}

int DataChunk::GetEstimatedLength() {
    if (!mCachedLength) {
        mCachedLength = CalculateEstimatedLength();
    }

    return mCachedLength;
}

DataChunkNop::DataChunkNop() : DataChunk() {}

bool DataChunkNop::Output(std::ostream& output, int indentLevel, int linePrefix) {
    return false;
}

int DataChunkNop::CalculateEstimatedLength() {
    return 0;
}

StringDataChunk::StringDataChunk(const std::string& value): PrimitiveDataChunk(EscapeAndWrapString(value)) {}

char StringDataChunk::EscapeCharacter(char input) {
    switch (input) {
        case '"': return '"';
        case '\t': return 't';
        case '\n': return 'n';
        case '\r': return 'r';
    }

    return 0;
}

std::string StringDataChunk::EscapeAndWrapString(const std::string& string) {
    std::ostringstream result;

    result << '"';

    for (auto currChar : string) {
        char escapeChar = EscapeCharacter(currChar);

        if (escapeChar) {
            result << '\\' << escapeChar;
        } else {
            result << escapeChar;
        }
    }

    result << '"';

    return result.str();
}


StructureEntryDataChunk::StructureEntryDataChunk(const std::string& name, std::unique_ptr<DataChunk> entry) :
    DataChunk(),
    mName(name),
    mEntry(std::move(entry)) {

}

bool StructureEntryDataChunk::Output(std::ostream& output, int indentLevel, int linePrefix) {
    output << "." << mName << " = ";
    mEntry->Output(output, indentLevel, linePrefix);
    return true;
}

int StructureEntryDataChunk::CalculateEstimatedLength() {
    return mName.length() + 4 + mEntry->GetEstimatedLength();
}

StructureDataChunk::StructureDataChunk(): DataChunk() {}

StructureDataChunk::StructureDataChunk(const aiVector3D& vector) : StructureDataChunk() {
    AddPrimitive(vector.x);
    AddPrimitive(vector.y);
    AddPrimitive(vector.z);
}

StructureDataChunk::StructureDataChunk(const aiQuaternion& quat) : StructureDataChunk() {
    AddPrimitive(quat.x);
    AddPrimitive(quat.y);
    AddPrimitive(quat.z);
    AddPrimitive(quat.w);

}


void StructureDataChunk::Add(std::unique_ptr<DataChunk> entry) {
    mChildren.push_back(std::move(entry));
}

void StructureDataChunk::Add(const std::string& name, std::unique_ptr<DataChunk> entry) {
    mChildren.push_back(std::unique_ptr<DataChunk>(new StructureEntryDataChunk(
        name,
        std::move(entry)
    )));
}

#define MAX_CHARS_PER_LINE  80
#define SPACES_PER_INDENT   4

bool StructureDataChunk::Output(std::ostream& output, int indentLevel, int linePrefix) {
    output << '{';

    OutputChildren(mChildren, output, indentLevel, linePrefix + GetEstimatedLength(), true);

    output << '}';
    return true;
}

int StructureDataChunk::CalculateEstimatedLength() {
    int result = 2; // parenthesis
    
    for (auto it = mChildren.begin(); it != mChildren.end(); ++it){
        result += (*it)->GetEstimatedLength();
    }

    if (mChildren.size()) {
        result += 2 * (mChildren.size() - 1);
    }

    return result;
}

void StructureDataChunk::OutputIndent(std::ostream& output, int indentLevel) {
    for (int i = 0; i < indentLevel * SPACES_PER_INDENT; ++i) {
        output << ' ';
    }
}

void StructureDataChunk::OutputChildren(std::vector<std::unique_ptr<DataChunk>>& children, std::ostream& output, int indentLevel, int totalLength, bool trailingComma) {
    bool needsComma = false;
    
    if (totalLength < MAX_CHARS_PER_LINE) {
        for (size_t i = 0; i < children.size(); ++i) {
            if (needsComma) {
                output << ", ";
            }

            needsComma = children[i]->Output(output, indentLevel, 0);
        }
    } else {
        output << '\n';
        ++indentLevel;
        for (size_t i = 0; i < children.size(); ++i) {
            OutputIndent(output, indentLevel);
            if (children[i]->Output(output, indentLevel, indentLevel * SPACES_PER_INDENT) && (i < children.size() - 1 || trailingComma)) {
                output << ",\n";
            } else {
                output << "\n";
            }
        }
        --indentLevel;
        OutputIndent(output, indentLevel);
    }
}

MacroDataChunk::MacroDataChunk(const std::string& macroName): DataChunk(), mMacroName(macroName), mSingleLine(true) {}

MacroDataChunk::MacroDataChunk(const std::string& macroName, bool singleLine) : DataChunk(), mMacroName(macroName), mSingleLine(singleLine) {}

void MacroDataChunk::Add(std::unique_ptr<DataChunk> entry) {
    mParameters.push_back(std::move(entry));
}

bool MacroDataChunk::Output(std::ostream& output, int indentLevel, int linePrefix) {
    output << mMacroName << '(';

    StructureDataChunk::OutputChildren(mParameters, output, indentLevel, mSingleLine ? 0 : linePrefix + GetEstimatedLength(), false);

    output << ')';
    return true;
}

int MacroDataChunk::CalculateEstimatedLength() {
    int result = mMacroName.length() + 2; // parenthesis
    
    for (auto it = mParameters.begin(); it != mParameters.end(); ++it){
        result += (*it)->GetEstimatedLength();
    }

    if (mParameters.size()) {
        result += 2 * (mParameters.size() - 1);
    }

    return result;

}

CommentDataChunk::CommentDataChunk(const std::string& comment) : DataChunk(), mComment(comment) {}

bool CommentDataChunk::Output(std::ostream& output, int indentLevel, int linePrefix) {
    output << "/* " << mComment << " */";
    return false;
}

int CommentDataChunk::CalculateEstimatedLength() {
    return 6 + mComment.length();
}