/**
 * @file BVHNode.cpp
 * @brief 動的境界ボリューム階層（Dynamic BVH）の実装
 *
 * 効率的な空間分割構造を提供し、広域フェーズの衝突検出を高速化する。
 */

#include "BVHNode.h"
#include "CollisionFunc.h"
#include <numeric>
#include <memory>

namespace AtomEngine
{
	DynamicBVH::~DynamicBVH()
	{
		std::function<void(BVHNode*)> destroy = [&](BVHNode* n)
			{
				if (!n) return;
				destroy(n->mLeft);
				destroy(n->mRight);
				delete n; // ルートノード削除時のみ必要
			};
		destroy(mRoot);
		mRoot = nullptr;
		mLeafs.clear();
	}

	void SetParent(BVHNode* child, BVHNode* parent)
	{
		if (child) child->mParent = parent;
	}

	BVHNode* DynamicBVH::Insert(GameObject* object, const Collision::AABB& bound)
	{
		auto leaf = std::make_unique<BVHNode>();
		BVHNode* leafPtr = leaf.get();
		leafPtr->mObject = object;
		leafPtr->mBound = bound;

		if (!mRoot)
		{
			mRoot = leaf.release(); // 所有権を移譲
			mLeafs.push_back(mRoot);
			return mRoot;
		}

		BVHNode* best = FindBestSibling(leafPtr);
		mLeafs.push_back(leafPtr);

		if (!best)
		{
			auto newParent = std::make_unique<BVHNode>();
			BVHNode* newParentPtr = newParent.get();

			newParentPtr->mLeft = mRoot;
			newParentPtr->mRight = leaf.release(); // 所有権を移譲
			SetParent(mRoot, newParentPtr);
			SetParent(newParentPtr->mRight, newParentPtr);
			newParentPtr->UpdateFromChildren();

			mRoot = newParent.release(); // 所有権を移譲
			mRoot->mParent = nullptr;
			return leafPtr;
		}

		BVHNode* oldParent = best->mParent;
		auto newParent = std::make_unique<BVHNode>();
		BVHNode* newParentPtr = newParent.get();
		newParentPtr->mParent = oldParent;

		newParentPtr->mLeft = best;
		newParentPtr->mRight = leaf.release(); // 所有権を移譲
		SetParent(best, newParentPtr);
		SetParent(newParentPtr->mRight, newParentPtr);

		newParentPtr->UpdateFromChildren();

		if (oldParent)
		{
			if (oldParent->mLeft == best)
				oldParent->mLeft = newParentPtr;
			else
				oldParent->mRight = newParentPtr;

			oldParent->Update();
			newParent.release(); // 親が管理するため所有権を放棄
		}
		else
		{
			mRoot = newParent.release(); // 所有権を移譲
			mRoot->mParent = nullptr;
		}

		return leafPtr;
	}

	void DynamicBVH::Remove(BVHNode* leaf)
	{
		if (!leaf) return;

		auto it = std::find(mLeafs.begin(), mLeafs.end(), leaf);
		if (it != mLeafs.end()) mLeafs.erase(it);

		if (leaf == mRoot)
		{
			delete leaf;
			mRoot = nullptr;
			return;
		}

		BVHNode* parent = leaf->mParent;
		BVHNode* sibling = (parent->mLeft == leaf) ? parent->mRight : parent->mLeft;
		BVHNode* grand = parent->mParent;

		if (grand)
		{
			if (grand->mLeft == parent)
				grand->mLeft = sibling;
			else
				grand->mRight = sibling;

			sibling->mParent = grand;
			grand->Update();
		}
		else
		{
			mRoot = sibling;
			sibling->mParent = nullptr;
		}

		delete parent;
		delete leaf;
	}

	void DynamicBVH::Update(GameObject* object, const Collision::AABB& newBound)
	{
		BVHNode* leaf = FindLeaf(object);
		if (!leaf)
		{
			Insert(object, newBound);
			return;
		}

		if (leaf->mBound.Contains(newBound) || newBound.Contains(leaf->mBound))
		{
			leaf->mBound = newBound;
			RefitUpwards(leaf->mParent);
			return;
		}

		Remove(leaf);
		Insert(object, newBound);
	}

	BVHNode* DynamicBVH::FindLeaf(GameObject* object)
	{
		for (auto* l : mLeafs)
		{
			if (l->mObject == object) return l;
		}
		return nullptr;
	}

	BVHNode* DynamicBVH::FindBestSibling(BVHNode* newLeaf)
	{
		float bestCost = std::numeric_limits<float>::max();
		BVHNode* bestSibling = nullptr;

		for (auto* leaf : mLeafs)
		{
			if (leaf == newLeaf) continue;

			float cost = EvaluateSAH(leaf, newLeaf);
			if (cost < bestCost)
			{
				bestCost = cost;
				bestSibling = leaf;
			}
		}

		return bestSibling;
	}


	float DynamicBVH::EvaluateSAH(BVHNode* node, BVHNode* newLeaf) const
	{
		Collision::AABB united = node->mBound.Union(newLeaf->mBound);
		float cost = united.SurfaceArea();

		BVHNode* parent = node->mParent;
		Collision::AABB leafUnion = newLeaf->mBound;
		while (parent)
		{
			Collision::AABB oldAABB = parent->mBound;
			Collision::AABB newAABB = oldAABB.Union(leafUnion);
			cost += (newAABB.SurfaceArea() - oldAABB.SurfaceArea());
			parent = parent->mParent;
		}
		return cost;
	}

	void DynamicBVH::RefitUpwards(BVHNode* node)
	{
		while (node)
		{
			node->UpdateFromChildren();
			node = node->mParent;
		}
	}

	void DynamicBVH::Query(const Collision::AABB& queryBound, std::vector<BVHNode*>& out) const
	{
		if (!mRoot) return;
		std::vector<BVHNode*> stack;
		stack.push_back(mRoot);

		while (!stack.empty())
		{
			BVHNode* n = stack.back();
			stack.pop_back();

			if (!n) continue;
			if (!Collision::IsCollision(n->mBound, queryBound)) continue;

			if (n->IsLeaf())
			{
				out.push_back(n);
			}
			else
			{
				if (n->mLeft) stack.push_back(n->mLeft);
				if (n->mRight) stack.push_back(n->mRight);
			}
		}
	}

	void DynamicBVH::CollectPairs(std::vector<std::pair<BVHNode*, BVHNode*>>& outPairs) const
	{
		if (!mRoot) return;

		std::vector<std::pair<BVHNode*, BVHNode*>> stack;
		if (mRoot->mLeft && mRoot->mRight) stack.emplace_back(mRoot->mLeft, mRoot->mRight);

		while (!stack.empty())
		{
			auto p = stack.back();
			stack.pop_back();

			BVHNode* a = p.first;
			BVHNode* b = p.second;
			if (!Collision::IsCollision(a->mBound, b->mBound)) continue;

			if (a->IsLeaf() && b->IsLeaf())
			{
				outPairs.emplace_back(a, b);
			}
			else if (a->IsLeaf() && !b->IsLeaf())
			{
				if (b->mLeft) stack.emplace_back(a, b->mLeft);
				if (b->mRight) stack.emplace_back(a, b->mRight);
			}
			else if (!a->IsLeaf() && b->IsLeaf())
			{
				if (a->mLeft) stack.emplace_back(a->mLeft, b);
				if (a->mRight) stack.emplace_back(a->mRight, b);
			}
			else
			{
				if (a->mLeft && b->mLeft)  stack.emplace_back(a->mLeft, b->mLeft);
				if (a->mLeft && b->mRight) stack.emplace_back(a->mLeft, b->mRight);
				if (a->mRight && b->mLeft) stack.emplace_back(a->mRight, b->mLeft);
				if (a->mRight && b->mRight)stack.emplace_back(a->mRight, b->mRight);
			}
		}
	}
}