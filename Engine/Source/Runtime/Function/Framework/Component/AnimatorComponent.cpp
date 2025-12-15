//#include "AnimatorComponent.h"
//#include <algorithm>
//
//namespace AtomEngine
//{
//    void AnimatorComponent::CalculateBoneTransforms(const Skeleton& skeleton)
//    {
//        if (!m_CurrentClip) {
//            return;
//        }
//
//        const size_t numJoints = skeleton.joints.size();
//        if (numJoints == 0) {
//            return;
//        }
//
//        m_FinalBoneXforms.assign(numJoints, { Matrix4x4::IDENTITY, Matrix4x4::IDENTITY });
//        std::vector<Matrix4x4> globalTransforms(numJoints, Matrix4x4::IDENTITY);
//
//        // Calculate local animated transforms for all joints
//        std::vector<Transform> localAnimatedTransforms;
//        localAnimatedTransforms.reserve(numJoints);
//        for (const auto& joint : skeleton.joints) {
//            localAnimatedTransforms.push_back(joint.transform); // Start with bind pose
//        }
//
//        for (const auto& curve : m_CurrentClip->curves)
//        {
//            uint32_t jointIndex = curve.targetJoint;
//            if (jointIndex < numJoints)
//            {
//                Transform& localTransform = localAnimatedTransforms[jointIndex];
//
//                if (!curve.translation.empty()) {
//                    localTransform.transition = CalculateValue(curve.translation, m_CurrentTime);
//                }
//                if (!curve.rotation.empty()) {
//                    localTransform.rotation = CaculateRotation(curve.rotation, m_CurrentTime);
//                }
//                if (!curve.scale.empty()) {
//                    localTransform.scale = CalculateValue(curve.scale, m_CurrentTime);
//                }
//            }
//        }
//        
//        // Recursively calculate global transforms
//        std::function<void(int32_t, const Matrix4x4&)> calculateGlobalTransform =
//            [&](int32_t jointIndex, const Matrix4x4& parentGlobalTransform)
//            {
//                const Joint& joint = skeleton.joints[jointIndex];
//                Matrix4x4 localMatrix = localAnimatedTransforms[jointIndex].GetMatrix();
//                Matrix4x4 globalMatrix = localMatrix * parentGlobalTransform;
//                globalTransforms[jointIndex] = globalMatrix;
//
//                for (int32_t childIndex : joint.children)
//                {
//                    calculateGlobalTransform(childIndex, globalMatrix);
//                }
//            };
//        
//        // Start recursion from the root joint
//        if(skeleton.rootJoint != -1)
//        {
//            calculateGlobalTransform(skeleton.rootJoint, Matrix4x4::IDENTITY);
//        }
//
//        // Calculate final skinning matrices
//        for (size_t i = 0; i < numJoints; ++i)
//        {
//            m_FinalBoneXforms[i].posXform = skeleton.joints[i].inverseBindPose * globalTransforms[i];
//            m_FinalBoneXforms[i].nrmXform = m_FinalBoneXforms[i].posXform.Inverse().Transpose();
//        }
//    }
//}