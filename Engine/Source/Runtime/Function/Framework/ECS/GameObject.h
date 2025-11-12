#pragma once
#include "ECSCommon.h"
#include "GUID.h"

namespace AtomEngine
{
	class World;

	class GameObject
	{
	public:
		GameObject() = default;
		GameObject(Entity handle, World* world);
		~GameObject()= default;
		operator Entity() const { return mHandle; }
		operator bool() const { return IsValid(); }

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

		template<typename T, typename... Args>
		GameObject& Add(Args&&... args) { AddComponent<T>(std::forward<Args>(args)...); return *this; }

		template<typename T>
		GameObject& Remove() { RemoveComponent<T>(); return *this; }

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

		uint64_t GetGUID() const { return mGUID.Value(); }

		bool operator==(const GameObject& other) const { return mHandle == other.mHandle; }
		bool operator!=(const GameObject& other) const { return !(*this == other); }

	private:
		Entity mHandle{ entt::null };
		World* mWorld = nullptr;
		ObjectGUID mGUID;
	};
}

#include "GameObject.inl"