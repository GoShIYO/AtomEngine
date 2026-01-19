/**
 * @file TransformComponent.h
 * @brief トランスフォームコンポーネント
 * 
 * エンティティの位置・回転・スケールを管理するコンポーネント。
 * Transformクラスを継承し、ECSシステムで使用可能にする。
 */

#pragma once
#include "Runtime/Core/Math/Transform.h"

namespace AtomEngine
{
	/**
	 * @struct TransformComponent
	 * @brief トランスフォームコンポーネント
	 * 
	 * エンティティの3D空間での位置、回転、スケールを保持する。
	 * Transform基底クラスのすべての機能を継承する。
	 */
	struct TransformComponent : public Transform
	{
		/**
		 * @brief コンストラクタ（パラメータ付き）
		 * @param position_ 初期位置
		 * @param rotation_ 初期回転（デフォルト: 恒等回転）
		 * @param scale_ 初期スケール（デフォルト: (1,1,1)）
		 */
      TransformComponent(const Vector3& position_, 
			const Quaternion& rotation_ = Quaternion::IDENTITY,
			const Vector3& scale_ = Vector3::UNIT_SCALE)
			: Transform(position_, rotation_, scale_)
		{
		}
		
		/**
		 * @brief デフォルトコンストラクタ
		 */
		TransformComponent() = default;
		~TransformComponent() = default;
	};
}
