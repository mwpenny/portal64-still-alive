#include "MES.h"

#include <random>
#include <math.h>
#include <iostream>
#include <assimp/scene.h>
#include "Vector4.h"
#include <set>

Sphere::Sphere() : radius(0.0f) {}

Sphere::Sphere(const aiVector3D& center, float radius): center(center), radius(radius) {}

bool Sphere::Contains(const aiVector3D& point) {
    return (point - center).SquareLength() <= radius * radius * 1.001f;
}

struct Sphere miniumEnclosingSphereTwoPoints(const aiVector3D& a, const aiVector3D& b) {
    return Sphere((a + b) * 0.5f, (a - b).Length() * 0.5);
}

bool miniumEnclosingSphereThreePoints(const aiVector3D& a, const aiVector3D& b, const aiVector3D& c, Sphere& output) {
    aiMatrix4x4 coef;

    aiVector3D edgeA = a - c;
    aiVector3D edgeB = b - c;

    aiVector3D normal = edgeA ^ edgeB;

    if (normal.SquareLength() < 0.00001f) {
        return false;
    }

    aiVector3D top = ((edgeB * edgeA.SquareLength()) - (edgeA * edgeB.SquareLength())) ^ normal;
    float bottom = 0.5f / normal.SquareLength();
    aiVector3D center = top * bottom;

    output.center = c + center;
    output.radius = center.Length();

    return true;
}

bool miniumEnclosingSphereFourPoints(const aiVector3D& a, const aiVector3D& b, const aiVector3D& c,const aiVector3D& d, Sphere& output) {
    
    aiVector3D edge1 = b - a;
    aiVector3D edge2 = c - a;
    aiVector3D edge3 = d - a;
    
    float sqLength1 = edge1.SquareLength();
    float sqLength2 = edge2.SquareLength();
    float sqLength3 = edge3.SquareLength();

    float determinant = edge1.x * (edge2.y * edge3.z - edge3.y * edge2.z)
        - edge2.x * (edge1.y * edge3.z - edge3.y * edge1.z)
        + edge3.x * (edge1.y * edge2.z - edge2.y * edge1.z);

    if (fabs(determinant) < 0.000001f) {
        return false;
    }

    float scalar = 0.5f / determinant;

    aiVector3D offset(
      scalar * ((edge2.y * edge3.z - edge3.y * edge2.z) * sqLength1 - (edge1.y * edge3.z - edge3.y * edge1.z) * sqLength2 + (edge1.y * edge2.z - edge2.y * edge1.z) * sqLength3),
      scalar * (-(edge2.x * edge3.z - edge3.x * edge2.z) * sqLength1 + (edge1.x * edge3.z - edge3.x * edge1.z) * sqLength2 - (edge1.x * edge2.z - edge2.x * edge1.z) * sqLength3),
      scalar * ((edge2.x * edge3.y - edge3.x * edge2.y) * sqLength1 - (edge1.x * edge3.y - edge3.x * edge1.y) * sqLength2 + (edge1.x * edge2.y - edge2.x * edge1.y) * sqLength3)
    );

    output.center = a + offset;
    output.radius = offset.Length();

    return true;
}

struct Sphere miniumEnclosingSphereTrivial(const std::vector<aiVector3D>& points) {
    if (points.size() > 4) {
        // degenerate case
        return Sphere(aiVector3D(0.0f, 0.0f, 0.0f), 0.0f);
    }

    if (points.size() == 0) {
        return Sphere(aiVector3D(0.0f, 0.0f, 0.0f), 0.0f);
    }
    
    if (points.size() == 1) {
        return Sphere(points[0], 0.0f);
    }

    if (points.size() == 2) {
        return miniumEnclosingSphereTwoPoints(points[0], points[1]);
    }

    // see if two points is sufficient
    for (unsigned i = 0; i < points.size(); ++i) {
        for (unsigned j = i + 1; j < points.size(); ++j) {
            Sphere test = miniumEnclosingSphereTwoPoints(points[i], points[j]);
            
            unsigned otherIndex;

            for (otherIndex = 0; otherIndex < points.size(); ++otherIndex) {
                if (otherIndex == i || otherIndex == j) {
                    continue;
                }

                if (!test.Contains(points[otherIndex])) {
                    break;
                }
            }

            if (otherIndex == points.size()) {
                return test;
            }
        }
    }

    if (points.size() == 3) {
        Sphere result;

        if (miniumEnclosingSphereThreePoints(points[0], points[1], points[2], result)) {
            return result;
        } else {
            return miniumEnclosingSphereTwoPoints(points[0], points[1]);
        }
    }

    // see if 3 points are sufficent
    for (unsigned i = 0; i < 4; ++i) {
        Sphere result;

        if (miniumEnclosingSphereThreePoints(points[i == 0 ? 1 : 0], points[i <= 1 ? 2 : 1], points[i <= 2 ? 3 : 2], result) && result.Contains(points[i])) {
            return result;
        }
    }

    Sphere result;
    if (miniumEnclosingSphereFourPoints(points[0], points[1], points[2], points[3], result)) {
        return result;
    }

    return Sphere(aiVector3D(0.0f, 0.0f, 0.0f), 0.0f);
}

struct Sphere minimumEnclosingSphereMutateInput(std::vector<aiVector3D>& input) {
    std::shuffle(input.begin(), input.end(), std::default_random_engine(rand()));
    std::set<unsigned> edgeIndices;
    Sphere currentSphere;

    unsigned pointIndex = 0;

    while (pointIndex < input.size()) {
        if (edgeIndices.find(pointIndex) != edgeIndices.end() || currentSphere.Contains(input[pointIndex])) {
            ++pointIndex;
        } else {
            std::set<unsigned> setCopy;
            std::vector<aiVector3D> edgePoints;

            for (auto current : edgeIndices) {
                if (current > pointIndex) {
                    setCopy.insert(current);
                    edgePoints.push_back(input[current]);
                }
            }

            edgeIndices = setCopy;
            edgeIndices.insert(pointIndex);
            edgePoints.push_back(input[pointIndex]);

            currentSphere = miniumEnclosingSphereTrivial(edgePoints);

            if (edgePoints.size() > 1 && currentSphere.radius == 0.0f) {
                miniumEnclosingSphereTrivial(edgePoints);
                std::cout << "something wrong" << std::endl;
            }

            if (edgeIndices.size() < 4) {
                pointIndex = 0;
            } else {
                pointIndex = pointIndex + 1;
            }
        }
    }

    return currentSphere;
}

struct Sphere minimumEnclosingSphereForMeshes(const std::vector<aiMesh*>& input) {
    std::vector<aiVector3D> allPoints;

    for (auto mesh : input) {
        allPoints.insert(allPoints.end(), mesh->mVertices, mesh->mVertices + mesh->mNumVertices);
    }

    return minimumEnclosingSphereMutateInput(allPoints);
}

struct Sphere minimumEnclosingSphere(const std::vector<aiVector3D>& input) {
    std::vector<aiVector3D> copy = input;
    return minimumEnclosingSphereMutateInput(copy);
}