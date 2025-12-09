#pragma once
#include "Runtime/Core/Math/MathInclude.h"
#include <unordered_map>
#include <optional>

namespace AtomEngine
{

    struct JointVertex
    {
        float weights[4];
        uint32_t jointIndices[4];
    };

    struct Joint
    {
        Transform transform;                //transfrom情報
        Matrix4x4 localMatrix;              //localMatrix
        Matrix4x4 skeletonSpaceMatrix;      //skeletonSpaceでの変換行列
        std::string name;                   //名前
        std::vector<int32_t>children;       //子JointのIndexのリスト。いなければ空
        int32_t index;                      //自身のIndex
        std::optional<int32_t>parent;       //親JointのIndex。いなければnull
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


