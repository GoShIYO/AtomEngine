#pragma once
#include "Runtime/Function/Framework/ECS/World.h"
#include "../Voxel/VoxelWorld.h"
#include <unordered_set>
#include <unordered_map>

struct VoxelColliderComponent;
struct VoxelHit;
namespace AtomEngine
{
	struct TransformComponent;
}

/**
 * @class VoxelCollisionSystem
 * @brief ボクセルベースの衝突検出・応答システム
 * 
 * 主な責務:
 * - エンティティとボクセルワールド間の衝突検出・解決
 * - 動的ボディとの衝突処理
 * - 重力適用とグラウンド状態管理
 * - ステップクライミング（階段登り）
 * - プラットフォームライディング（移動床への追従）
 * - スタック検出と緊急脱出処理
 */
class VoxelCollisionSystem
{
public:
	/**
	 * @brief デフォルトコンストラクタ
	 */
	VoxelCollisionSystem() = default;

	/**
	 * @brief ボクセルワールドを設定
	 * @param world ボクセルワールドポインタ
	 */
	void SetVoxelWorld(VoxelWorld* world) { mVoxelWorld = world; }

	/**
	 * @brief 重力値を設定
	 * @param gravity 重力加速度（正の値）
	 */
	void SetGravity(float gravity) { mGravity = gravity; }

	/**
	 * @brief 重力値を取得
	 * @return 現在の重力加速度
	 */
	float GetGravity() const { return mGravity; }

	/**
	 * @brief システム更新（全エンティティを処理）
	 * @param world ECSワールド
	 * @param deltaTime フレーム時間（秒）
	 */
	void Update(AtomEngine::World& world, float deltaTime);

	/**
	 * @brief 単一エンティティの衝突処理
	 * @param world ECSワールド
	 * @param entity 処理対象エンティティ
	 */
	void ProcessEntity(AtomEngine::World& world, AtomEngine::Entity entity);

	/**
	 * @brief レイキャスト（静的および動的ボディ含む）
	 * @param origin レイの起点
	 * @param direction レイの方向（正規化推奨）
	 * @param maxDistance 最大距離
	 * @return ヒット情報（ヒットしない場合はnullopt）
	 */
	std::optional<VoxelHit> Raycast(const AtomEngine::Vector3& origin,
		const AtomEngine::Vector3& direction,
		float maxDistance = 1000.0f) const;

	/**
	 * @brief レイキャスト（静的ボクセルのみ）
	 * @param origin レイの起点
	 * @param direction レイの方向（正規化推奨）
	 * @param maxDistance 最大距離
	 * @return ヒット情報（ヒットしない場合はnullopt）
	 */
	std::optional<VoxelHit> RaycastStatic(const AtomEngine::Vector3& origin,
		const AtomEngine::Vector3& direction,
		float maxDistance = 1000.0f) const;

	/**
	 * @brief 指定位置が有効か判定（静的ボクセルと重なっていないか）
	 * @param position 中心位置
	 * @param halfExtents AABB半サイズ
	 * @return 有効な位置ならtrue
	 */
	bool IsPositionValid(const AtomEngine::Vector3& position,
		const AtomEngine::Vector3& halfExtents) const;

	/**
	 * @brief エンティティが乗っているプラットフォームを取得
	 * @param world ECSワールド
	 * @param entity 対象エンティティ
	 * @return プラットフォームエンティティ（存在しない場合はentt::null）
	 */
	AtomEngine::Entity GetPlatformUnder(AtomEngine::World& world, AtomEngine::Entity entity) const;

private:
	VoxelWorld* mVoxelWorld{ nullptr };      ///< ボクセルワールドへの参照
	float mGravity{ 40.0f };              ///< 重力加速度

	/// プラットフォームとそれに乗っているエンティティのマップ
	std::unordered_map<AtomEngine::Entity, std::unordered_set<AtomEngine::Entity>> mPlatformRiders;

	/// エンティティごとのスタック検出フレームカウント
	std::unordered_map<AtomEngine::Entity, int> mStuckFrameCount;

	/// エンティティの前フレーム位置記録
	std::unordered_map<AtomEngine::Entity, AtomEngine::Vector3> mLastPositions;

	// 定数定義
	static constexpr float kMinMovementThreshold = 0.001f;      ///< 最小移動閾値
	static constexpr int kMaxStuckFrames = 10;       ///< スタック判定フレーム数
	static constexpr float kEmergencyPushDistance = 0.5f;       ///< 緊急脱出時の押し出し距離
	static constexpr float kSkinWidth = 0.01f;       ///< 衝突スキン幅
	static constexpr float kMaxPenetrationResolve = 2.0f;       ///< 最大貫通解決距離

	/**
	 * @brief 重力を適用
	 * @param world ECSワールド
	 * @param entity 対象エンティティ
	 * @param deltaTime フレーム時間
	 */
	void ApplyGravity(AtomEngine::World& world, AtomEngine::Entity entity, float deltaTime);

	/**
	 * @brief エンティティの移動処理（メイン）
	 * @param world ECSワールド
	 * @param entity 対象エンティティ
	 * @param deltaTime フレーム時間
	 */
	void ProcessMovement(AtomEngine::World& world, AtomEngine::Entity entity, float deltaTime);

	/**
	 * @brief 移動ベクトルを計算
	 * @param world ECSワールド
	 * @param entity 対象エンティティ
	 * @param collider コライダーコンポーネント
	 * @param deltaTime フレーム時間
	 * @return 計算された移動ベクトル
	 */
	AtomEngine::Vector3 ComputeMovementVector(AtomEngine::World& world,
		AtomEngine::Entity entity,
		const VoxelColliderComponent& collider,
		float deltaTime) const;

	/**
	 * @brief サブステップ数を計算
	 * @param movement 移動ベクトル
	 * @return サブステップ数（1-8の範囲）
	 */
	int CalculateSubSteps(const AtomEngine::Vector3& movement) const;

	/**
	 * @brief スライディング移動を実行
	 * @param world ECSワールド
	 * @param entity 対象エンティティ
	 * @param currentPos 現在位置
	 * @param stepMovement ステップ移動量
	 * @param stepHorizontalMovement 水平方向ステップ移動量
	 * @param collider コライダーコンポーネント
	 * @return 新しい位置
	 */
	AtomEngine::Vector3 ProcessSlidingMovement(AtomEngine::World& world,
		AtomEngine::Entity entity,
		const AtomEngine::Vector3& currentPos,
		const AtomEngine::Vector3& stepMovement,
		const AtomEngine::Vector3& stepHorizontalMovement,
		VoxelColliderComponent& collider);

	/**
	 * @brief スイープ移動を実行
	 * @param world ECSワールド
	 * @param entity 対象エンティティ
	 * @param currentPos 現在位置
	 * @param stepMovement ステップ移動量
	 * @param stepHorizontalMovement 水平方向ステップ移動量
	 * @param collider コライダーコンポーネント
	 * @return 新しい位置
	 */
	AtomEngine::Vector3 ProcessSweepMovement(AtomEngine::World& world,
		AtomEngine::Entity entity,
		const AtomEngine::Vector3& currentPos,
		const AtomEngine::Vector3& stepMovement,
		const AtomEngine::Vector3& stepHorizontalMovement,
		VoxelColliderComponent& collider);

	/**
	 * @brief 衝突応答を処理（速度調整）
	 * @param world ECSワールド
	 * @param entity 対象エンティティ
	 * @param collider コライダーコンポーネント
	 * @param blockedMovement ブロックされた移動量
	 */
	void HandleCollisionResponse(AtomEngine::World& world,
		AtomEngine::Entity entity,
		VoxelColliderComponent& collider,
		const AtomEngine::Vector3& blockedMovement);

	/**
	 * @brief ステップクライミングを試行
	 * @param world ECSワールド
	 * @param entity 対象エンティティ
	 * @param currentPos 現在位置
	 * @param stepHorizontalMovement 水平移動量
	 * @param collider コライダーコンポーネント
	 * @param transform トランスフォームコンポーネント
	 * @param effectiveMaxStepHeight 有効な最大ステップ高さ
	 * @return クライミング成功時true
	 */
	bool TryPerformStepClimb(AtomEngine::World& world,
		AtomEngine::Entity entity,
		const AtomEngine::Vector3& currentPos,
		const AtomEngine::Vector3& stepHorizontalMovement,
		VoxelColliderComponent& collider,
		AtomEngine::TransformComponent& transform,
		float effectiveMaxStepHeight);

	/**
	 * @brief グラウンド状態を更新
	 * @param world ECSワールド
	 * @param entity 対象エンティティ
	 */
	void UpdateGroundedState(AtomEngine::World& world, AtomEngine::Entity entity);

	/**
	 * @brief スムーズステップクライミング処理
	 * @param world ECSワールド
	 * @param entity 対象エンティティ
	 * @param deltaTime フレーム時間
	 */
	void ProcessStepClimbSmooth(AtomEngine::World& world, AtomEngine::Entity entity, float deltaTime);

	/**
	 * @brief ワールド境界内にクランプ
	 * @param position 位置
	 * @param halfExtents AABB半サイズ
	 * @return クランプされた位置
	 */
	AtomEngine::Vector3 ClampToWorldBounds(const AtomEngine::Vector3& position,
		const AtomEngine::Vector3& halfExtents) const;

	/**
	 * @brief ボクセルに対するステップクライミング試行
	 * @param currentPos 現在位置
	 * @param horizontalMovement 水平移動量
	 * @param halfExtents AABB半サイズ
	 * @param maxStepHeight 最大ステップ高さ
	 * @param stepSearchOvershoot 検索オーバーシュート距離
	 * @param minStepDepth 最小ステップ深さ
	 * @param outNewPos 成功時の新しい位置（出力）
	 * @return 成功時true
	 */
	bool TryStepClimb(const AtomEngine::Vector3& currentPos,
		const AtomEngine::Vector3& horizontalMovement,
		const AtomEngine::Vector3& halfExtents,
		float maxStepHeight,
		float stepSearchOvershoot,
		float minStepDepth,
		AtomEngine::Vector3& outNewPos) const;

	/**
	 * @brief プラットフォームに対するステップクライミング試行
	 * @param world ECSワールド
	 * @param entity 対象エンティティ
	 * @param currentPos 現在位置
	 * @param horizontalMovement 水平移動量
	 * @param halfExtents AABB半サイズ
	 * @param maxStepHeight 最大ステップ高さ
	 * @param outNewPos 成功時の新しい位置（出力）
	 * @return 成功時true
	 */
	bool TryStepClimbOnPlatform(AtomEngine::World& world,
		AtomEngine::Entity entity,
		const AtomEngine::Vector3& currentPos,
		const AtomEngine::Vector3& horizontalMovement,
		const AtomEngine::Vector3& halfExtents,
		float maxStepHeight,
		AtomEngine::Vector3& outNewPos) const;

	/**
	 * @brief 動的ボディの更新
	 * @param world ECSワールド
	 * @param deltaTime フレーム時間
	 */
	void UpdateDynamicBodies(AtomEngine::World& world, float deltaTime);

	/**
	 * @brief 動的ボディとの衝突処理
	 * @param world ECSワールド
	 * @param deltaTime フレーム時間
	 */
	void ProcessDynamicBodyCollisions(AtomEngine::World& world, float deltaTime);

	/**
	 * @brief プラットフォームライダーの更新
	 * @param world ECSワールド
	 * @param deltaTime フレーム時間
	 */
	void UpdatePlatformRiders(AtomEngine::World& world, float deltaTime);

	/**
	 * @brief 動的ボディとの衝突チェック
	 * @param world ECSワールド
	 * @param boxMin AABB最小点
	 * @param boxMax AABB最大点
	 * @param excludeEntity 除外エンティティ
	 * @param outPushVector 押し出しベクトル（出力）
	 * @param outCollidedBody 衝突したボディ（出力）
	 * @return 衝突検出時true
	 */
	bool CheckDynamicBodyCollision(AtomEngine::World& world,
		const AtomEngine::Vector3& boxMin,
		const AtomEngine::Vector3& boxMax,
		AtomEngine::Entity excludeEntity,
		AtomEngine::Vector3& outPushVector,
		AtomEngine::Entity& outCollidedBody) const;

	/**
	 * @brief 動的ボディのAABBを取得
	 * @param world ECSワールド
	 * @param entity 対象エンティティ
	 * @param outMin AABB最小点（出力）
	 * @param outMax AABB最大点（出力）
	 */
	void GetDynamicBodyAABB(AtomEngine::World& world, AtomEngine::Entity entity,
		AtomEngine::Vector3& outMin, AtomEngine::Vector3& outMax) const;

	/**
	 * @brief ワンウェイコリジョンとの衝突判定
	 * @param velocity エンティティの速度
	 * @param oneWayDirection ワンウェイ方向
	 * @return 衝突すべき場合true
	 */
	bool ShouldCollideWithOneWay(const AtomEngine::Vector3& velocity,
		const AtomEngine::Vector3& oneWayDirection) const;

	/**
	 * @brief エンティティがスタックしているか判定
	 * @param world ECSワールド
	 * @param entity 対象エンティティ
	 * @param currentPos 現在位置
	 * @return スタック検出時true
	 */
	bool IsEntityStuck(AtomEngine::World& world, AtomEngine::Entity entity,
		const AtomEngine::Vector3& currentPos);

	/**
	 * @brief 緊急脱出を試行
	 * @param world ECSワールド
	 * @param entity 対象エンティティ
	 * @return 脱出成功時true
	 */
	bool TryEmergencyEscape(AtomEngine::World& world, AtomEngine::Entity entity);

	/**
	 * @brief 最も近い安全な位置を検索
	 * @param position 基準位置
	 * @param halfExtents AABB半サイズ
	 * @return 安全な位置
	 */
	AtomEngine::Vector3 FindNearestSafePosition(const AtomEngine::Vector3& position,
		const AtomEngine::Vector3& halfExtents) const;

	/**
	 * @brief 衝突を段階的に解決
	 * @param position 位置
	 * @param halfExtents AABB半サイズ
	 * @param maxCorrectionPerFrame フレームあたりの最大補正量
	 * @return 補正された位置
	 */
	AtomEngine::Vector3 ResolveCollisionProgressive(
		const AtomEngine::Vector3& position,
		const AtomEngine::Vector3& halfExtents,
		float maxCorrectionPerFrame);

	/**
	 * @brief 分離軸テスト（SAT）
	 * @param boxMin1 1つ目のAABB最小点
	 * @param boxMax1 1つ目のAABB最大点
	 * @param boxMin2 2つ目のAABB最小点
	 * @param boxMax2 2つ目のAABB最大点
	 * @param outMTV 最小移動ベクトル（出力）
	 * @return 衝突時true
	 */
	bool SeparatingAxisTest(const AtomEngine::Vector3& boxMin1, const AtomEngine::Vector3& boxMax1,
		const AtomEngine::Vector3& boxMin2, const AtomEngine::Vector3& boxMax2,
		AtomEngine::Vector3& outMTV) const;
};
