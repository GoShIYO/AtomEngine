#pragma once
#include "ECSCommon.h"

namespace AtomEngine
{
	class GameObject;
    
    using ComponentCallback = std::function<void(Entity, std::type_index)>;

    class World
    {
    public:
        World();
        ~World();

        GameObject CreateGameObject(const std::string& name = "");
        void DestroyGameObject(Entity entity);
        GameObject FindByName(const std::string& name);
        GameObject FindByTag(const std::string& tag);

        template<typename T, typename... Args>
        T& AddComponent(Entity e, Args&&... args)
        {
            auto& component = mRegistry.emplace<T>(e, std::forward<Args>(args)...);
            mDispatcher.trigger(ComponentAddedEvent{ e, std::type_index(typeid(T)) });
            return component;
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
        decltype(auto) ViewFor(Entity e)
        {
            return mRegistry.try_get<Components...>(e);
        }

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

        void OnComponentAdded(const ComponentAddedEvent& e);

        void OnComponentRemoved(const ComponentRemovedEvent& e);

        void SetOnComponentAdded(ComponentCallback cb) { mOnComponentAdded = std::move(cb); }

        void SetOnComponentRemoved(ComponentCallback cb) { mOnComponentRemoved = std::move(cb); }

        entt::dispatcher& GetDispatcher() { return mDispatcher; }

        entt::registry& GetRegistry() { return mRegistry; }

    private:
        entt::registry mRegistry;
        entt::dispatcher mDispatcher;

        std::unordered_map<std::string, Entity> mNameLookup;
        std::unordered_multimap<std::string, Entity> mTagLookup;

        ComponentCallback mOnComponentAdded = nullptr;
        ComponentCallback mOnComponentRemoved = nullptr;
    };
}


