#include "RenderChunkOrder.h"

#include "./materials/MaterialTransitionTiming.h"

#include <map>
#include <queue>
#include <vector>
#include <algorithm>

struct RenderChunkPath {
    RenderChunkPath(int size);

    double bestCase;
    double worstCase;
    double currentLength;
    int startIndex;
    int currentIndex;
    // map to index to from index
    std::vector<int> path;

    bool IsDone() const;

    bool operator <(const RenderChunkPath& other) const;
};

RenderChunkPath::RenderChunkPath(int size) :
    bestCase(0.0),
    worstCase(0.0),
    currentLength(0.0),
    startIndex(-1),
    currentIndex(0),
    path(size) {
    for (int i = 0; i < size; ++i) {
        path[i] = -1;
    }
}

bool RenderChunkPath::IsDone() const {
    return startIndex == currentIndex;
}

bool RenderChunkPath::operator <(const RenderChunkPath& other) const {
    if (bestCase == other.bestCase) {
        return worstCase > other.worstCase;
    }

    return bestCase > other.bestCase;
}

class RenderChunkPathCompare {
    public:
       bool operator()(RenderChunkPath a, RenderChunkPath b){
           if(a.bestCase == b.bestCase){
               return a.worstCase > b.worstCase;
           }
           return a.bestCase > b.bestCase;
      }
};

struct RenderChunkDistanceGraph {
    RenderChunkDistanceGraph(int numberOfEdges);

    std::vector<double> edgeDistance;
    std::vector<double> minDistanceTo;
    std::vector<double> maxDistanceTo;
    double bestWorstCase;
    int numberOfEdges;

    std::priority_queue<RenderChunkPath, std::vector<RenderChunkPath>, RenderChunkPathCompare> currentChunks;

    double GetDistance(int from, int to) const;
    void SetDistance(int from, int to, double value);

    struct RenderChunkPath currentBest;
};

RenderChunkDistanceGraph::RenderChunkDistanceGraph(int numberOfEdges) :
    bestWorstCase(0.0),
    numberOfEdges(numberOfEdges),
    currentBest(numberOfEdges) {
    edgeDistance.resize(numberOfEdges * numberOfEdges);
    maxDistanceTo.resize(numberOfEdges);
    minDistanceTo.resize(numberOfEdges);
}


double RenderChunkDistanceGraph::GetDistance(int from, int to) const {
    return edgeDistance[from * numberOfEdges + to];
}

void RenderChunkDistanceGraph::SetDistance(int from, int to, double value) {
    edgeDistance[from * numberOfEdges + to] = value;
}

void orderRenderGreedy(struct RenderChunkDistanceGraph& graph, struct RenderChunkPath& path);

void orderRenderAddNext(struct RenderChunkDistanceGraph& graph, struct RenderChunkPath& next) {
    if (next.bestCase > graph.bestWorstCase) {
        return;
    }

    if (next.worstCase < graph.bestWorstCase) {
        graph.bestWorstCase = next.worstCase;
    }

    if (!next.IsDone()) {
        graph.currentChunks.push(next);
    } else if (graph.currentBest.currentLength > next.currentLength) {
        graph.currentBest = next;
    }
}

void orderRenderPopulateNext(struct RenderChunkDistanceGraph& graph, struct RenderChunkPath& current) {
    for (int i = 0; i < graph.numberOfEdges; ++i) {
        if (i == current.startIndex) {
            // leave start index to be last
            continue;
        }

        if (i == current.currentIndex) {
            // dont go back to self
            continue;
        }

        if (current.path[i] != -1) {
            // dont visit a node that has already been visited
            continue;
        }

        double edgeDistnace = graph.GetDistance(current.currentIndex, i);

        struct RenderChunkPath next(current);

        next.bestCase += edgeDistnace - graph.minDistanceTo[i];
        next.worstCase += edgeDistnace - graph.maxDistanceTo[i];

        next.currentLength += edgeDistnace;
        next.path[i] = current.currentIndex;

        if (next.startIndex == -1) {
            next.startIndex = next.currentIndex;
        }

        next.currentIndex = i;

        if ((int)next.path.size() + 1 == graph.numberOfEdges) {
            next.currentLength += graph.GetDistance(next.currentIndex, next.startIndex);
            next.path[next.startIndex] = next.currentIndex;
            next.currentIndex = next.startIndex;

            next.bestCase = next.currentLength;
            next.worstCase = next.currentLength;
        }

        orderRenderAddNext(graph, next);
    }
}

void orderRenderBnB(struct RenderChunkDistanceGraph& graph, int maxIterations) {
    struct RenderChunkPath first(graph.numberOfEdges);
    first.currentLength = 0.0f;
    first.bestCase = 0.0f;
    first.worstCase = 0.0f;
    first.currentIndex = 0;
    first.startIndex = -1;

    for (int i = 0; i < graph.numberOfEdges; ++i) {
        first.bestCase += graph.minDistanceTo[i];
        first.worstCase += graph.maxDistanceTo[i];
    }

    // get a base case
    orderRenderGreedy(graph, graph.currentBest);
    graph.bestWorstCase = graph.currentBest.worstCase;

    orderRenderPopulateNext(graph, first);

    int iteration = 0;

    while (iteration < maxIterations && graph.currentChunks.size()) {
        struct RenderChunkPath current = graph.currentChunks.top();
        graph.currentChunks.pop();

        if (current.bestCase > graph.currentBest.worstCase) {
            break;
        }

        orderRenderPopulateNext(graph, current);
    }
}

void orderRenderGreedy(struct RenderChunkDistanceGraph& graph, struct RenderChunkPath& path) {
    path.bestCase = 0.0f;
    path.worstCase = 0.0f;
    path.currentLength = 0.0f;
    path.startIndex = 0;
    path.currentIndex = 0;

    while ((int)path.path.size() < graph.numberOfEdges) {
        int toIndex = path.startIndex;
        double minPathLength = graph.GetDistance(path.currentIndex, path.startIndex);
        
        for (int i = 0; i < graph.numberOfEdges; ++i) {
            if (i == path.startIndex) {
                // leave start index to be last
                continue;
            }

            if (i == path.currentIndex) {
                // dont go back to self
                continue;
            }

            if (path.path[i] != -1) {
                // dont visit a node that has already been visited
                continue;
            }

            double edgeDistnace = graph.GetDistance(path.currentIndex, i);

            if (toIndex == path.startIndex || edgeDistnace < minPathLength) {
                minPathLength = edgeDistnace;
                toIndex = i;
            }
        }

        path.currentLength += minPathLength;
        path.path[toIndex] = path.currentIndex;
        path.currentIndex = toIndex;
    }

    path.bestCase = path.currentLength;
    path.worstCase = path.currentLength;
}

double orderRenderDistance(const RenderChunk& from, const RenderChunk& to) {
    double result = 0.0;
    
    if (from.mMaterial && to.mMaterial) {
        result += materialTransitionTime(from.mMaterial->mState, to.mMaterial->mState);
    };

    Bone* ancestor = Bone::FindCommonAncestor(from.mBonePair.second, to.mBonePair.first);

    Bone* fromBone = from.mBonePair.second;
    Bone* toBone = to.mBonePair.first;

    int fromBoneHeight = 0;
    int toBoneHeight = 0;

    while (fromBone != ancestor) {
        fromBone = fromBone->GetParent();
        ++fromBoneHeight;
    }

    while (toBone != ancestor) {
        toBone = toBone->GetParent();
        ++toBoneHeight;
    }

    if (fromBoneHeight) {
        result += TIMING_DP_MATRIX_POP;
    }

    result += toBoneHeight * TIMING_DP_MATRIX_MUL;

    return result;
}

void orderRenderExtract(const RenderChunkPath& path, std::vector<RenderChunk>& input, int startIndex, std::vector<RenderChunk>& output) {
    int currentIndex = startIndex;

    for (unsigned i = 0; i < path.path.size(); ++i) {
        int nextIndex = path.path[currentIndex];

        if (nextIndex == -1) {
            output = input;
            std::cerr << "A full path was not found" << std::endl;
            return;
        }

        currentIndex = nextIndex;

        output.push_back(input[currentIndex]);
    }

    std::reverse(output.begin(), output.end());
}

void orderRenderChunks(std::vector<RenderChunk>& chunks, const DisplayListSettings& settings) {
    int startIndex = -1;

    for (unsigned i = 0; i < chunks.size(); ++i) {
        if (chunks[i].mBonePair.first == nullptr && 
            chunks[i].mBonePair.second == nullptr && 
            chunks[i].mMaterial != nullptr && 
            chunks[i].mMaterial->mName == settings.mDefaultMaterialName) {
            startIndex = i;
            break;
        }
    }

    if (startIndex == -1) {
        startIndex = (int)chunks.size();
        RenderChunk newChunk;

        auto defaultMaterial = settings.mMaterials.find(settings.mDefaultMaterialName);

        if (defaultMaterial != settings.mMaterials.end()) {
            newChunk.mMaterial = defaultMaterial->second.get();
        }

        chunks.push_back(newChunk);
    }

    RenderChunkDistanceGraph graph((int)chunks.size());

    for (unsigned from = 0; from < chunks.size(); ++from) {
        for (unsigned to = 0; to < chunks.size(); ++to) {
            if (from == to) {
                continue;;
            }

            double distance = orderRenderDistance(chunks[from], chunks[to]);

            graph.SetDistance(from, to, distance);

            if (from == 0 || distance < graph.minDistanceTo[to]) {
                graph.minDistanceTo[to] = distance;
            }

            if (distance > graph.maxDistanceTo[to]) {
                graph.maxDistanceTo[to] = distance;
            }
        }
    }

    orderRenderBnB(graph, settings.mMaxOptimizationIterations);

    std::vector<RenderChunk> result;
    orderRenderExtract(graph.currentBest, chunks, startIndex, result);

    if (result[0].mMesh == nullptr) {
        result.erase(result.begin());
    }

    chunks = result;
}