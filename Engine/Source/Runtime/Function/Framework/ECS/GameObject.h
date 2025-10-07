#pragma once
#include "ECSCommon.h"

namespace AtomEngine
{
	class World;

	class GameObject
	{
	public:
		GameObject() = default;
		GameObject(Entity handle, World* world);

		bool IsValid() const;
		Entity GetHandle() const { return mHandle; }

		/// <summary>
		/// 新しいコンポーネントを追加する
		/// </summary>
		/// <typeparam name="T">コンポーネント</typeparam>
		/// <typeparam name="...Args"></typeparam>
		/// <param name="...args"></param>
		/// <returns>コンポーネントの参照</returns>
		template<typename T, typename... Args>
		T& AddComponent(Args&&... args);

		/// <summary>
		/// コンポーネントを取得する
		/// </summary>
		/// <typeparam name="T">コンポーネント</typeparam>
		/// <returns>あるかどうか</returns>
		template<typename T>
		bool HasComponent() const;

		/// <summary>
		/// 指定したコンポーネントを取得
		/// </summary>
		/// <typeparam name="T">コンポーネント</typeparam>
		/// <returns>コンポーネントの参照</returns>
		template<typename T>
		T& GetComponent();

		/// <summary>
		/// 指定したコンポーネントを削除する
		/// </summary>
		/// <typeparam name="T">コンポーネント</typeparam>
		template<typename T>
		void RemoveComponent();

		void Destroy();
		std::string GetName();
		void SetName(const std::string& name);

		std::string GetTag();
		void SetTag(const std::string& tag);

	private:
		Entity mHandle{ entt::null };
		World* mWorld{ nullptr };
	};
}

#include "GameObject.inl"