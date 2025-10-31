#pragma once
#include "Runtime/Core/Math/MathInclude.h"
#include <unordered_map>

namespace AtomEngine
{

    struct JointVertex
    {
        uint16_t jointIndices[4];
        float weights[4];
    };

    struct Joint
    {
        std::string name;
        int32_t index;
        int32_t parentIndex;
        std::vector<int32_t> children;

        Matrix4x4 localBindPose;
        Matrix4x4 inverseBindPose;
    };

    struct Skeleton
    {
        int32_t rootJoint = -1;
        std::vector<Joint> joints;
        std::unordered_map<std::string, int32_t> jointMap;
    };

    struct VertexWeightData
    {
        float weight;
        uint32_t vertexIndex;
    };

    struct JointWeightData
    {
        Matrix4x4 inverseBindPoseMatrix;
        std::vector<VertexWeightData> vertexWeights;
    };
}


