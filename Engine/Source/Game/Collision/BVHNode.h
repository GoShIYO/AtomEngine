#pragma once
#include "CollisionCommon.h"
#include "Runtime/Function/Framework/ECS/GameObject.h"

namespace AtomEngine
{
	struct BVHNode
	{
		BVHNode* mParent = nullptr;
		BVHNode* mLeft = nullptr;
		BVHNode* mRight = nullptr;

		GameObject* mObject = nullptr;

		Collision::AABB mBound;

		bool IsLeaf()const { return mObject != nullptr; }

		void Reset() { mBound.Reset(); }

		void UpdateFromChildren()
		{
			if (mLeft && mRight)
			{
				mBound = mLeft->mBound.Union(mRight->mBound);
			}
			else if (mLeft)
			{
				mBound = mLeft->mBound;
			}
			else if (mRight)
			{
				mBound = mRight->mBound;
			}
			else
			{
				mBound.Reset();
			}
		}


		void Update()
		{
			UpdateFromChildren();
			if(mParent)
				mParent->Update();
		}
	};

	class DynamicBVH
	{
	public:
		DynamicBVH() = default;
		~DynamicBVH();

		BVHNode* Insert(GameObject* object, const Collision::AABB& bound);

		void Remove(BVHNode* leaf);

		void Update(GameObject* object, const Collision::AABB& newBound);

		BVHNode* FindLeaf(GameObject* object);

		BVHNode* Root() const { return mRoot; }

		void Query(const Collision::AABB& queryBound, std::vector<BVHNode*>& out) const;

		void CollectPairs(std::vector<std::pair<BVHNode*, BVHNode*>>& outPairs) const;

	private:
		BVHNode* FindBestSibling(BVHNode* newLeaf);
		float EvaluateSAH(BVHNode* node, BVHNode* newLeaf) const;
		void RefitUpwards(BVHNode* node);

	private:
		BVHNode* mRoot = nullptr;
		std::vector<BVHNode*> mLeafs;
	};
}


