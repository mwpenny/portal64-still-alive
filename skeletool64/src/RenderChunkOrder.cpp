#include "RenderChunkOrder.h"

#include "./materials/MaterialTransitionTiming.h"

#include <map>
#include <queue>
#include <vector>
#include <algorithm>
#include <memory>

EstimatedTime::EstimatedTime(): materialSwitching(0.0), matrixSwitching(0.0) {
    
}

EstimatedTime::EstimatedTime(double total, double materialSwitching, double matrixSwitching): 
    materialSwitching(materialSwitching), matrixSwitching(matrixSwitching) {

}

double EstimatedTime::GetTotal() const {
    return matrixSwitching + materialSwitching;
}

struct RenderChunkPath {
    RenderChunkPath(int size);

    double bestCase;
    double worstCase;
    double currentLength;
    int startIndex;
    int currentIndex;
    int edgeCount;
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
    edgeCount(0),
    path(size) {
    for (int i = 0; i < size; ++i) {
        path[i] = -1;
    }
}

bool RenderChunkPath::IsDone() const {
    return startIndex == currentIndex && edgeCount > 0;
}

bool RenderChunkPath::operator <(const RenderChunkPath& other) const {
    if (bestCase == other.bestCase) {
        return worstCase > other.worstCase;
    }

    return bestCase > other.bestCase;
}

class RenderChunkPathCompare {
    public:
       bool operator()(const std::shared_ptr<RenderChunkPath>& a, const std::shared_ptr<RenderChunkPath>& b){
           if(a->bestCase == b->bestCase){
               return a->worstCase > b->worstCase;
           }
           return a->bestCase > b->bestCase;
      }
};

struct RenderChunkDistanceGraph {
    RenderChunkDistanceGraph(int numberOfEdges);

    std::vector<double> edgeDistance;
    std::vector<double> materialDistance;
    std::vector<double> matrixDistance;
    std::vector<double> minDistanceTo;
    std::vector<double> maxDistanceTo;
    double bestWorstCase;
    int numberOfEdges;

    std::priority_queue<std::shared_ptr<RenderChunkPath>, std::vector<std::shared_ptr<RenderChunkPath>>, RenderChunkPathCompare> currentChunks;

    double GetDistance(int from, int to) const;
    double GetMaterialDistance(int from, int to) const;
    double GetMatrixDistance(int from, int to) const;
    void SetDistance(int from, int to, struct EstimatedTime estimatedTime);

    std::shared_ptr<RenderChunkPath> currentBest;
};

RenderChunkDistanceGraph::RenderChunkDistanceGraph(int numberOfEdges) :
    bestWorstCase(0.0),
    numberOfEdges(numberOfEdges),
    currentBest(nullptr) {
    edgeDistance.resize(numberOfEdges * numberOfEdges);
    materialDistance.resize(numberOfEdges * numberOfEdges);
    matrixDistance.resize(numberOfEdges * numberOfEdges);
    maxDistanceTo.resize(numberOfEdges);
    minDistanceTo.resize(numberOfEdges);
}


double RenderChunkDistanceGraph::GetDistance(int from, int to) const {
    return edgeDistance[from * numberOfEdges + to];
}

double RenderChunkDistanceGraph::GetMaterialDistance(int from, int to) const{
    return materialDistance[from * numberOfEdges + to];
}

double RenderChunkDistanceGraph::GetMatrixDistance(int from, int to) const {
    return matrixDistance[from * numberOfEdges + to];
}

void RenderChunkDistanceGraph::SetDistance(int from, int to, struct EstimatedTime estimatedTime) {
    edgeDistance[from * numberOfEdges + to] = estimatedTime.GetTotal();
    materialDistance[from * numberOfEdges + to] = estimatedTime.materialSwitching;
    matrixDistance[from * numberOfEdges + to] = estimatedTime.matrixSwitching;
}

void orderRenderGreedy(struct RenderChunkDistanceGraph& graph, struct RenderChunkPath& path);

void orderRenderAddNext(struct RenderChunkDistanceGraph& graph, std::shared_ptr<RenderChunkPath>& next) {
    if (next->bestCase > graph.bestWorstCase) {
        return;
    }

    if (next->worstCase < graph.bestWorstCase) {
        graph.bestWorstCase = next->worstCase;
    }

    if (!next->IsDone()) {
        graph.currentChunks.push(next);
    } else if (graph.currentBest == nullptr || graph.currentBest->currentLength > next->currentLength) {
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

        std::shared_ptr<RenderChunkPath> next(new RenderChunkPath(current));

        next->bestCase += edgeDistnace - graph.minDistanceTo[i];
        next->worstCase += edgeDistnace - graph.maxDistanceTo[i];

        next->currentLength += edgeDistnace;
        next->edgeCount += 1;
        next->path[i] = current.currentIndex;

        if (next->startIndex == -1) {
            next->startIndex = next->currentIndex;
        }

        next->currentIndex = i;

        if (next->edgeCount + 1 == graph.numberOfEdges) {
            next->currentLength += graph.GetDistance(next->currentIndex, next->startIndex);
            next->path[next->startIndex] = next->currentIndex;
            next->currentIndex = next->startIndex;

            next->bestCase = next->currentLength;
            next->worstCase = next->currentLength;
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

    graph.currentBest = std::shared_ptr<RenderChunkPath>(new RenderChunkPath(graph.numberOfEdges));

    // get a base case
    orderRenderGreedy(graph, *graph.currentBest);
    graph.bestWorstCase = graph.currentBest->worstCase;

    double greedyLength = graph.currentBest->currentLength;

    orderRenderPopulateNext(graph, first);

    int iteration = 0;

    while (iteration < maxIterations && graph.currentChunks.size()) {
        std::shared_ptr<RenderChunkPath> current = graph.currentChunks.top();
        graph.currentChunks.pop();

        if (current->bestCase > graph.currentBest->worstCase) {
            break;
        }

        orderRenderPopulateNext(graph, *current);

        iteration += 1;

        if ((iteration % 10000) == 0) {
            std::cout << iteration << "/" << maxIterations << " searching for better solution. current improvement:" << (graph.currentBest->currentLength / greedyLength) << std::endl;
        }
    }

    if (graph.currentBest->currentLength == greedyLength) {
        std::cout << "Branch and bound could not find a better solution" << std::endl;
    } else {
        std::cout << "Branch and bound found a solution better by " << (graph.currentBest->currentLength / greedyLength) << std::endl;
    }
}

void orderRenderGreedy(struct RenderChunkDistanceGraph& graph, struct RenderChunkPath& path) {
    path.bestCase = 0.0f;
    path.worstCase = 0.0f;
    path.currentLength = 0.0f;
    path.startIndex = 0;
    path.currentIndex = 0;

    while (!path.IsDone()) {
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
        path.edgeCount += 1;
    }

    path.bestCase = path.currentLength;
    path.worstCase = path.currentLength;
}

struct EstimatedTime orderRenderDistance(const RenderChunk& from, const RenderChunk& to) {
    struct EstimatedTime result;
    
    if (from.mMaterial && to.mMaterial) {
        result.materialSwitching += materialTransitionTime(from.mMaterial->mState, to.mMaterial->mState);
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
        result.matrixSwitching += TIMING_DP_MATRIX_POP;
    }

    result.matrixSwitching += toBoneHeight * TIMING_DP_MATRIX_MUL;

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

            EstimatedTime time = orderRenderDistance(chunks[from], chunks[to]);

            graph.SetDistance(from, to, time);

            double distance = time.GetTotal();

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
    orderRenderExtract(*graph.currentBest, chunks, startIndex, result);

    if (result[0].mMesh == nullptr) {
        result.erase(result.begin());
    }

    chunks = result;
}