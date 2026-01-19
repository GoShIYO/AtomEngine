/**
 * @file BVHNode.h
 * @brief 境界ボリューム階層（BVH）のノード構造と動的BVH管理
 * 
 * 高速な衝突判定のための空間分割構造を提供する。
 * 動的にオブジェクトの追加・削除・更新が可能。
 */

#pragma once
#include "CollisionCommon.h"
#include "Runtime/Function/Framework/ECS/GameObject.h"

namespace AtomEngine
{
	/**
	 * @struct BVHNode
	 * @brief BVH木の単一ノード
	 * 
	 * 葉ノード（リーフ）はゲームオブジェクトを保持し、
	 * 内部ノードは子ノードのAABBを統合した境界を保持する。
	 */
	struct BVHNode
	{
		BVHNode* mParent = nullptr;  ///< 親ノードへのポインタ
		BVHNode* mLeft = nullptr;    ///< 左の子ノード
		BVHNode* mRight = nullptr;   ///< 右の子ノード

		GameObject* mObject = nullptr;  ///< 葉ノードの場合、対応するゲームオブジェクト

		Collision::AABB mBound;  ///< このノードの境界ボックス

		/**
		 * @brief このノードが葉（リーフ）かどうか判定
		 * @return 葉ノードならtrue
		 */
		bool IsLeaf()const { return mObject != nullptr; }

		/**
		 * @brief 境界をリセット
		 */
		void Reset() { mBound.Reset(); }

		/**
		 * @brief 子ノードから境界を更新
		 */
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

		/**
		 * @brief このノードと親ノードを再帰的に更新
		 */
		void Update()
		{
			UpdateFromChildren();
			if(mParent)
				mParent->Update();
		}
	};

	/**
	 * @class DynamicBVH
	 * @brief 動的境界ボリューム階層
	 * 
	 * オブジェクトの動的な追加・削除・移動に対応したBVH構造。
	 * Surface Area Heuristic (SAH) を用いて最適なツリー構造を維持する。
	 * 
	 * 主な用途:
	 * - 広域フェーズの衝突検出
	 * - 空間クエリ（範囲検索）
	 * - レイキャスト高速化
	 */
	class DynamicBVH
	{
	public:
		DynamicBVH() = default;
		~DynamicBVH();

		/**
		 * @brief オブジェクトをBVHに挿入
		 * @param object 挿入するゲームオブジェクト
		 * @param bound オブジェクトの境界ボックス
		 * @return 作成された葉ノード
		 */
		BVHNode* Insert(GameObject* object, const Collision::AABB& bound);

		/**
		 * @brief 葉ノードを削除
		 * @param leaf 削除する葉ノード
		 */
		void Remove(BVHNode* leaf);

		/**
		 * @brief オブジェクトの境界を更新
		 * @param object 更新するゲームオブジェクト
		 * @param newBound 新しい境界ボックス
		 */
		void Update(GameObject* object, const Collision::AABB& newBound);

		/**
		 * @brief オブジェクトに対応する葉ノードを検索
		 * @param object 検索するゲームオブジェクト
		 * @return 見つかった葉ノード（なければnullptr）
		 */
		BVHNode* FindLeaf(GameObject* object);

		/**
		 * @brief ルートノードを取得
		 * @return ルートノード
		 */
		BVHNode* Root() const { return mRoot; }

		/**
		 * @brief 指定境界と重なるノードを検索
		 * @param queryBound クエリ境界
		 * @param out 結果を格納するベクター
		 */
		void Query(const Collision::AABB& queryBound, std::vector<BVHNode*>& out) const;

		/**
		 * @brief 衝突可能性のあるノードペアを収集
		 * @param outPairs 結果を格納するベクター
		 */
		void CollectPairs(std::vector<std::pair<BVHNode*, BVHNode*>>& outPairs) const;

	private:
		/**
		 * @brief 新しい葉の最適な兄弟ノードを探索
		 * @param newLeaf 新しい葉ノード
		 * @return 最適な兄弟ノード
		 */
		BVHNode* FindBestSibling(BVHNode* newLeaf);
		
		/**
		 * @brief Surface Area Heuristic (SAH) コストを評価
		 * @param node 評価対象のノード
		 * @param newLeaf 新しい葉ノード
		 * @return SAHコスト値
		 */
		float EvaluateSAH(BVHNode* node, BVHNode* newLeaf) const;
		
		/**
		 * @brief 親方向にノードを再フィット
		 * @param node 開始ノード
		 */
		void RefitUpwards(BVHNode* node);

	private:
		BVHNode* mRoot = nullptr;           ///< BVHのルートノード
		std::vector<BVHNode*> mLeafs;      ///< 全ての葉ノードへのポインタ
	};
}


