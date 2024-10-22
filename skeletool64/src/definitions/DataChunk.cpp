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
            result << currChar;
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

StructureDataChunk::StructureDataChunk(): DataChunk(), mHasNewlineHints(false) {}

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


StructureDataChunk::StructureDataChunk(const aiAABB& bb) : StructureDataChunk() {
    Add(std::unique_ptr<StructureDataChunk>(new StructureDataChunk(bb.mMin)));
    Add(std::unique_ptr<StructureDataChunk>(new StructureDataChunk(bb.mMax)));
}


void StructureDataChunk::Add(std::unique_ptr<DataChunk> entry) {
    if (dynamic_cast<NewlineHintChunk*>(entry.get()) != nullptr) {
        mHasNewlineHints = true;
    }

    mChildren.push_back(std::move(entry));
}

void StructureDataChunk::Add(const std::string& name, std::unique_ptr<DataChunk> entry) {
    mChildren.push_back(std::unique_ptr<DataChunk>(new StructureEntryDataChunk(
        name,
        std::move(entry)
    )));
}

void StructureDataChunk::AddNewlineHint() {
    mHasNewlineHints = true;
    mChildren.push_back(std::unique_ptr<DataChunk>(new NewlineHintChunk()));
}

#define MAX_CHARS_PER_LINE  80
#define SPACES_PER_INDENT   4

bool StructureDataChunk::Output(std::ostream& output, int indentLevel, int linePrefix) {
    output << '{';

    OutputChildren(mChildren, output, indentLevel, linePrefix + GetEstimatedLength(), true, !mHasNewlineHints);

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

void StructureDataChunk::OutputChildren(std::vector<std::unique_ptr<DataChunk>>& children, std::ostream& output, int indentLevel, int totalLength, bool trailingComma, bool includeNewlines) {
    bool needsComma = false;
    
    if (totalLength < MAX_CHARS_PER_LINE) {
        for (size_t i = 0; i < children.size(); ++i) {
            if (needsComma) {
                output << ", ";
            }

            if (dynamic_cast<NewlineHintChunk*>(children[i].get()) != nullptr) {
                needsComma = false;
                continue;
            }

            needsComma = children[i]->Output(output, indentLevel, 0);
        }
    } else {
        output << '\n';
        bool needsIndent = true;
        ++indentLevel;
        for (size_t i = 0; i < children.size(); ++i) {
            if (needsIndent) {
                OutputIndent(output, indentLevel);
                needsIndent = false;
            } else {
                output << ' ';
            }
            if (children[i]->Output(output, indentLevel, indentLevel * SPACES_PER_INDENT) && (i < children.size() - 1 || trailingComma)) {
                output << ",";
            }

            if (includeNewlines) {
                output << "\n";
                needsIndent = true;
            } else if (dynamic_cast<NewlineHintChunk*>(children[i].get()) != nullptr) {
                needsIndent = true;
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

    StructureDataChunk::OutputChildren(mParameters, output, indentLevel, mSingleLine ? 0 : linePrefix + GetEstimatedLength(), false, true);

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

NewlineHintChunk::NewlineHintChunk() : DataChunk() {}

bool NewlineHintChunk::Output(std::ostream& output, int indentLevel, int linePrefix) {
    output << "\n";
    return false;
}

int NewlineHintChunk::CalculateEstimatedLength() {
    return 0;
}