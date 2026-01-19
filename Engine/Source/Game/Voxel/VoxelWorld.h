/**
 * @file VoxelWorld.h
 * @brief ボクセルベースのワールド表現と物理演算
 * 
 * 3Dグリッドベースのボクセルワールドを管理し、
 * レイキャスト、AABB衝突検出、スイープテストなどを提供する。
 */

#pragma once
#include "Runtime/Core/Math/MathInclude.h"
#include <array>
#include <optional>
#include <vector>
#undef min
#undef max

using namespace AtomEngine;

/**
 * @struct VoxelHit
 * @brief ボクセルへのレイキャストヒット情報
 */
struct VoxelHit
{
	Vector3 position{};   ///< ワールド空間での接触点
	Vector3 normal{};     ///< 分離法線
	float distance{ 0.0f }; ///< レイ原点からの距離
	int voxelX{ 0 }, voxelY{ 0 }, voxelZ{ 0 }; ///< ボクセル座標
};

/**
 * @struct VoxelSweepResult
 * @brief AABBスイープテストの結果
 */
struct VoxelSweepResult
{
	bool hit{ false };      ///< ヒットしたか
	float time{ 1.0f }; ///< ヒット時刻（0.0～1.0）
	Vector3 normal{};       ///< 衝突法線
	Vector3 contactPoint{}; ///< 接触点
	int voxelX{ 0 }, voxelY{ 0 }, voxelZ{ 0 }; ///< ヒットしたボクセル座標
};

/**
 * @struct VoxelCollisionResult
 * @brief AABB衝突解決の結果
 */
struct VoxelCollisionResult
{
	bool collided{ false };     ///< 衝突したか
	Vector3 penetration{};          ///< 貫通ベクトル
	Vector3 normal{};       ///< 衝突法線
	Vector3 correctedPosition{};    ///< 補正後の位置
};

/**
 * @struct VoxelWorld
 * @brief ボクセルワールドの管理構造体
 * 
 * 3Dグリッドベースのボクセルデータを保持し、
 * 各種物理クエリ（レイキャスト、衝突検出など）を提供する。
 * 
 * 主な機能:
 * - .voxファイルからのワールド読み込み
 * - レイキャスト
 * - AABBとの衝突判定・解決
 * - スイープテスト
 * - グラウンド検出
 */
struct VoxelWorld
{
	/**
	 * @struct CellCoord
	 * @brief ボクセルのセル座標
	 */
	struct CellCoord
	{
		int x{ 0 }, y{ 0 }, z{ 0 };
	};

	int width{ 0 }, height{ 0 }, depth{ 0 };  ///< ワールドサイズ（ボクセル単位）
	float voxelSize = 1.0f;         ///< 1ボクセルのサイズ（ワールド単位）
	Vector3 origin = Vector3::ZERO;      ///< ワールド原点
	std::vector<uint8_t> data;    ///< ボクセルデータ（0 = 空、それ以外 = ソリッド）
	Vector3 worldSize = Vector3::ZERO;        ///< ワールドサイズ（ワールド単位）

	/**
	 * @brief .voxファイルからワールドを読み込み
	 * @param file ファイルパス
	 * @return 成功したらtrue
	 */
	bool Load(const std::string& file);

	VoxelWorld() = default;

	/**
	 * @brief ワールドサイズを変更
	 * @param w 幅
	 * @param h 高さ
	 * @param d 奥行き
	 */
	void Resize(int w, int h, int d);

	/**
	 * @brief 座標からインデックスを計算
	 * @param x X座標
	 * @param y Y座標
	 * @param z Z座標
	 * @return 1次元インデックス
	 */
	inline int Index(int x, int y, int z) const { return x + y * width + z * width * height; }
	
	/**
	 * @brief 座標が範囲内か判定
	 * @param x X座標
	 * @param y Y座標
	 * @param z Z座標
	 * @return 範囲内ならtrue
	 */
	bool InBounds(int x, int y, int z) const
	{
		return x >= 0 && x < width && y >= 0 && y < height && z >= 0 && z < depth;
	}

	/**
	 * @brief ボクセルをソリッドに設定
	 * @param x X座標
	 * @param y Y座標
	 * @param z Z座標
	 * @param solid ソリッドにするか
	 */
	void SetSolid(int x, int y, int z, bool solid = true)
	{
		if (!InBounds(x, y, z)) return;
		data[Index(x, y, z)] = solid ? 1 : 0;
	}
	
	/**
	 * @brief ボクセルがソリッドか判定
	 * @param x X座標
	 * @param y Y座標
	 * @param z Z座標
	 * @return ソリッドならtrue
	 */
	bool IsSolid(int x, int y, int z) const
	{
		if (!InBounds(x, y, z)) return false;
		return data[Index(x, y, z)] != 0;
	}

	/**
	 * @brief ボクセル値を設定
	 * @param x X座標
	 * @param y Y座標
	 * @param z Z座標
	 * @param value 設定する値
	 */
	void Set(int x, int y, int z, uint8_t value)
	{
		if (!InBounds(x, y, z))return;
		data[Index(x, y, z)] = value;
	}

	/**
	 * @brief ボクセル値を取得
	 * @param x X座標
	 * @param y Y座標
	 * @param z Z座標
	 * @return ボクセル値
	 */
	uint8_t Get(int x, int y, int z) const
	{
		if (!InBounds(x, y, z))return 0;
		return data[Index(x, y, z)];
	}

	/**
	 * @brief ワールド座標からボクセル座標に変換
	 * @param worldPos ワールド座標
	 * @return ボクセル座標（範囲外ならnullopt）
	 */
	std::optional<CellCoord> WorldToCoord(const Vector3& worldPos) const;
	
	/**
	 * @brief ボクセル座標からワールド座標の中心点を取得
	 * @param coord ボクセル座標
	 * @return ワールド座標
	 */
	Vector3 CoordToWorldCenter(const CellCoord& coord) const;
	
	/**
	 * @brief ボクセルのワールド空間最小点を取得
	 * @param x X座標
	 * @param y Y座標
	 * @param z Z座標
	 * @return 最小点
	 */
	Vector3 CoordToWorldMin(int x, int y, int z) const;
	
	/**
	 * @brief ボクセルのワールド空間最大点を取得
	 * @param x X座標
	 * @param y Y座標
	 * @param z Z座標
	 * @return 最大点
	 */
	Vector3 CoordToWorldMax(int x, int y, int z) const;

	/**
	 * @brief ワールド座標がソリッドか判定
	 * @param worldPos ワールド座標
	 * @return ソリッドならtrue
	 */
	bool IsSolid(const Vector3& worldPos) const;
	
	/**
	 * @brief AABBがソリッドボクセルと重なるか判定
	 * @param worldMin AABB最小点
	 * @param worldMax AABB最大点
	 * @return 重なるならtrue
	 */
	bool OverlapsSolid(const Vector3& worldMin, const Vector3& worldMax) const;

	/**
	 * @brief レイキャスト
	 * @param origin レイの原点
	 * @param direction レイの方向
	 * @param maxDistance 最大距離
	 * @return ヒット情報（ヒットしない場合はnullopt）
	 */
	std::optional<VoxelHit> Raycast(const Vector3& origin, const Vector3& direction, float maxDistance = 1000.0f) const;

	/**
	 * @brief AABBスイープテスト
	 * @param boxMin AABB最小点
	 * @param boxMax AABB最大点
	 * @param velocity 移動速度
	 * @return スイープ結果
	 */
	VoxelSweepResult SweepAABB(const Vector3& boxMin, const Vector3& boxMax,
		const Vector3& velocity) const;

	/**
	 * @brief AABB衝突を解決
	 * @param boxMin AABB最小点
	 * @param boxMax AABB最大点
	 * @return 衝突解決結果
	 */
	VoxelCollisionResult ResolveAABBCollision(const Vector3& boxMin, const Vector3& boxMax) const;

	/**
	 * @brief AABBと重なるボクセルを取得
	 * @param boxMin AABB最小点
	 * @param boxMax AABB最大点
	 * @return 重なるボクセルのリスト
	 */
	std::vector<CellCoord> GetOverlappingVoxels(const Vector3& boxMin, const Vector3& boxMax) const;

	/**
	 * @brief 移動とスライド（衝突応答付き移動）
	 * @param position 現在位置
	 * @param halfExtents AABBハーフエクステント
	 * @param velocity 移動速度
	 * @param maxIterations 最大反復回数
	 * @return 移動後の位置
	 */
	Vector3 MoveAndSlide(const Vector3& position, const Vector3& halfExtents,
		const Vector3& velocity, int maxIterations = 4) const;

	/**
	 * @brief 接地判定
	 * @param position 位置
	 * @param halfExtents AABBハーフエクステント
	 * @param groundCheckDistance 接地チェック距離
	 * @return 接地しているならtrue
	 */
	bool IsGrounded(const Vector3& position, const Vector3& halfExtents, float groundCheckDistance = 0.1f) const;

private:
	/**
	 * @brief ボクセルまでの符号付き距離を計算
	 * @param point 点
	 * @param vx ボクセルX座標
	 * @param vy ボクセルY座標
	 * @param vz ボクセルZ座標
	 * @return 符号付き距離
	 */
	float SignedDistanceToVoxel(const Vector3& point, int vx, int vy, int vz) const;
	
	/**
	 * @brief ボクセルの法線を取得
	 * @param point 点
	 * @param vx ボクセルX座標
	 * @param vy ボクセルY座標
	 * @param vz ボクセルZ座標
	 * @return 法線ベクトル
	 */
	Vector3 GetVoxelNormal(const Vector3& point, int vx, int vy, int vz) const;
};

/**
 * @struct VoxelWorldComponent
 * @brief ボクセルワールドコンポーネント
 * 
 * ECSでVoxelWorldを管理するためのコンポーネント。
 */
struct VoxelWorldComponent
{
	VoxelWorld world;  ///< ボクセルワールドインスタンス
};
