//#pragma once
//#include "Runtime/Resource/Animation.h"
//#include "Runtime/Resource/Model.h" // For Skeleton
//#include <vector>
//
//namespace AtomEngine
//{
//    class AnimatorComponent
//    {
//    public:
//        AnimatorComponent() = default;
//        AnimatorComponent(AnimationClip* clip)
//            : m_CurrentClip(clip)
//        {
//        }
//
//        void PlayAnimation(AnimationClip* pAnimation)
//        {
//            m_CurrentClip = pAnimation;
//            m_CurrentTime = 0.0f;
//        }
//
//        void UpdateAnimation(float dt)
//        {
//            if (m_CurrentClip)
//            {
//                m_CurrentTime += dt;
//                if (m_CurrentTime > m_CurrentClip->duration)
//                {
//                    m_CurrentTime = fmod(m_CurrentTime, m_CurrentClip->duration);
//                }
//            }
//        }
//        
//        void CalculateBoneTransforms(const Skeleton& skeleton);
//
//        const std::vector<JointXform>& GetFinalBoneXforms() const
//        {
//            return m_FinalBoneXforms;
//        }
//
//    private:
//        std::vector<JointXform> m_FinalBoneXforms;
//        AnimationClip* m_CurrentClip = nullptr;
//        float m_CurrentTime = 0.0f;
//    };
//}
