#pragma once
#include "ECSCommon.h"

namespace AtomEngine
{
	class GameObject;
    
    using ComponentCallback = std::function<void(Entity, std::type_index)>;

    struct ComponentAddedEvent
    {
        Entity entity;
        std::type_index type;
    };
    struct ComponentRemovedEvent
    {
        Entity entity;
        std::type_index type;
    };

    class World
    {
    public:
        World();
        ~World();

        std::unique_ptr<GameObject> CreateGameObject(const std::string& name = "");
        GameObject* GetGameObject(Entity entity);
        void DestroyGameObject(Entity entity);

        //名前でエンティティを探す
        Entity FindByName(const std::string& name);

        //コンポーネントを追加
        template<typename T, typename... Args>
        void AddComponent(Entity e, Args&&... args)
        {
            mRegistry.emplace<T>(e, std::forward<Args>(args)...);
            mDispatcher.trigger(ComponentAddedEvent{ e, std::type_index(typeid(T)) });
        }

        template<typename T>
        bool HasComponent(Entity e) const { return mRegistry.any_of<T>(e); }

        template<typename T>
        T& GetComponent(Entity e) { return mRegistry.get<T>(e); }

        template<typename T>
        void RemoveComponent(Entity e)
        {
            mRegistry.remove<T>(e);
            mDispatcher.trigger(ComponentRemovedEvent{ e, std::type_index(typeid(T)) });
        }

        template<typename... Components>
        decltype(auto) View()
        {
            return mRegistry.view<Components...>();
        }

        template<typename... Components>
        decltype(auto) TryGet(Entity e)
        {
            return mRegistry.try_get<Components...>(e);
        }

        void SetOnComponentAdded(ComponentCallback cb) { mOnComponentAdded = std::move(cb); }

        void SetOnComponentRemoved(ComponentCallback cb) { mOnComponentRemoved = std::move(cb); }

        entt::dispatcher& GetDispatcher() { return mDispatcher; }

        entt::registry& GetRegistry() { return mRegistry; }

        void SetName(Entity entity, const std::string& name);
    private:
        entt::registry mRegistry;
        entt::dispatcher mDispatcher;

        std::unordered_map<Entity, GameObject*> mEntityToObject;

        std::unordered_map<std::string, Entity> mNameLookup;

        ComponentCallback mOnComponentAdded = nullptr;
        ComponentCallback mOnComponentRemoved = nullptr;

    private:

        void OnComponentAdded(const ComponentAddedEvent& e);

        void OnComponentRemoved(const ComponentRemovedEvent& e);
        
    };
}


