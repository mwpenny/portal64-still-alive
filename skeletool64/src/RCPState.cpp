
#include "./RCPState.h"

#include <set>

VertexData::VertexData() :
    mVertexBuffer("-1"),
    mVertexIndex(-1),
    mMatrixIndex(-1) {

}

VertexData::VertexData(std::string vertexBuffer, int vertexIndex, int matrixIndex) :
    mVertexBuffer(vertexBuffer),
    mVertexIndex(vertexIndex),
    mMatrixIndex(matrixIndex) {

}

const bool VertexData::operator==(const VertexData& other) {
    return mVertexBuffer == other.mVertexBuffer &&
        mVertexIndex == other.mVertexIndex &&
        mMatrixIndex == other.mMatrixIndex;
}

RCPState::RCPState(const MaterialState& materialState, unsigned int maxVertexCount, unsigned int maxMatrixDepth, bool canPopMultiple) :
    mMaterialState(materialState),
    mMaxVertices(maxVertexCount),
    mMaxMatrixDepth(maxMatrixDepth),
    mCanPopMultiple(canPopMultiple) {

}

ErrorCode RCPState::TraverseToBone(Bone* bone, DisplayList& output) {
    std::set<Bone*> bonesToPop(mBoneMatrixStack.begin(), mBoneMatrixStack.end());

    Bone* curr = bone;

    while (curr) {
        bonesToPop.erase(curr);
        curr = curr->GetParent();
    }

    if (mCanPopMultiple) {
        if (bonesToPop.size() != 0) {
            output.AddCommand(std::unique_ptr<DisplayListCommand>(new PopMatrixCommand(bonesToPop.size())));
        }
    } else {
        for (unsigned int i = 0; i < bonesToPop.size(); ++i) {
            output.AddCommand(std::unique_ptr<DisplayListCommand>(new PopMatrixCommand(1)));
        }
    }

    mBoneMatrixStack.resize(mBoneMatrixStack.size() - bonesToPop.size());

    std::vector<Bone*> bonesToAdd;

    curr = bone;

    while (curr != nullptr && (mBoneMatrixStack.size() == 0 || *mBoneMatrixStack.rbegin() != curr)) {
        bonesToAdd.push_back(curr);
        curr = curr->GetParent();
    }

    for (auto curr = bonesToAdd.rbegin(); curr != bonesToAdd.rend(); ++curr) {
        if (mBoneMatrixStack.size() == mMaxMatrixDepth) {
            return ErrorCode::MatrixStackOverflow;
        }

        mBoneMatrixStack.push_back(*curr);
        output.AddCommand(std::unique_ptr<DisplayListCommand>(new CommentCommand((*curr)->GetName())));
        output.AddCommand(std::unique_ptr<DisplayListCommand>(new PushMatrixCommand((*curr)->GetIndex(), false)));
    }

    return ErrorCode::None;
}

void RCPState::AssignSlots(VertexData* newVertices, unsigned int* slotIndex, unsigned int vertexCount) {
    bool usedSlots[MAX_VERTEX_CACHE_SIZE];
    bool assignedVertices[MAX_VERTEX_CACHE_SIZE];
    for (unsigned int i = 0; i < MAX_VERTEX_CACHE_SIZE; ++i) {
        usedSlots[i] = false;
        assignedVertices[i] = false;
    }

    for (unsigned int currentVertex = 0; currentVertex < mMaxVertices; ++currentVertex) {
        for (unsigned int newVertex = 0; newVertex < vertexCount; ++newVertex) {
            if (mVertices[currentVertex] == newVertices[newVertex]) {
                usedSlots[currentVertex] = true;
                assignedVertices[newVertex] = true;

                slotIndex[newVertex] = currentVertex;
            }
        }
    }

    unsigned int nextTarget = 0;
    unsigned int nextSource = 0;

    while (nextTarget < mMaxVertices && nextSource < vertexCount) {
        if (usedSlots[nextTarget]) {
            ++nextTarget;
        } else if (assignedVertices[nextSource]) {
            ++nextSource;
        } else {
            slotIndex[nextSource] = nextTarget;
            ++nextTarget;
            ++nextSource;
        }
    }
}

const unsigned int RCPState::GetMaxVertices() {
    return mMaxVertices;
}

MaterialState& RCPState::GetMaterialState() {
    return mMaterialState;
}