#include "World.h"
#include "GameObject.h"

#include "Runtime/Core/LogSystem/LogSystem.h"

namespace AtomEngine
{
    World::World()
    {
        mDispatcher.sink<ComponentAddedEvent>().connect<&World::OnComponentAdded>(this);
        mDispatcher.sink<ComponentRemovedEvent>().connect<&World::OnComponentRemoved>(this);
    }

    World::~World()
    {
        mRegistry.clear();
        mDispatcher.clear();

        mNameLookup.clear();
        mTagLookup.clear();
        mGUIDLookup.clear();
    }

    GameObject World::CreateGameObject(const std::string& name)
    {
        Entity e = mRegistry.create();
        GameObject obj(e, this);

        mGUIDLookup[obj.GetGUID()] = e;

        if (!name.empty())
        {
            obj.AddComponent<NameComponent>(name);
            mNameLookup[name] = e;
        }

        return std::move(obj);
    }

    void World::DestroyGameObject(Entity entity)
    {
        if (mRegistry.valid(entity))
        {
            GameObject obj(entity, this);

            if (obj.HasComponent<NameComponent>())
                mNameLookup.erase(obj.GetComponent<NameComponent>().value);
            if (obj.HasComponent<TagComponent>())
                mTagLookup.erase(obj.GetComponent<TagComponent>().tag);
            
            mRegistry.destroy(entity);
        }
    }

    Entity World::FindByName(const std::string& name)
    {
        auto it = mNameLookup.find(name);
        if (it != mNameLookup.end())
            return it->second;
        return entt::null;
    }

    std::vector<Entity> World::FindByTag(const std::string& tag)
    {
        std::vector<Entity> result;
        auto range = mTagLookup.equal_range(tag);
        result.reserve(std::distance(range.first, range.second));
        for (auto it = range.first; it != range.second; ++it)
        {
            result.push_back(it->second);
        }

        return result;
    }

    Entity World::FindByGUID(uint64_t guid)
    {
        auto it = mGUIDLookup.find(guid);
        if(it != mGUIDLookup.end())
            return it->second;
        return entt::null;
    }

    void World::SetTag(Entity entity, const std::string& tag)
    {
        if (!mRegistry.valid(entity)) return;

        if (mRegistry.any_of<TagComponent>(entity))
        {
            const auto& oldTag = mRegistry.get<TagComponent>(entity).tag;
            auto range = mTagLookup.equal_range(oldTag);
            for (auto it = range.first; it != range.second; ++it)
            {
                if (it->second == entity)
                {
                    mTagLookup.erase(it);
                    break;
                }
            }
        }

        if (mRegistry.any_of<TagComponent>(entity))
            mRegistry.get<TagComponent>(entity).tag = tag;
        else
            mRegistry.emplace<TagComponent>(entity, tag);

        mTagLookup.emplace(tag, entity);
    }

    void World::SetName(Entity entity, const std::string& name)
    {
        if (!mRegistry.valid(entity)) return;

        if (mRegistry.any_of<NameComponent>(entity))
        {
            const auto& oldName = mRegistry.get<NameComponent>(entity).value;
            mNameLookup.erase(oldName);
            mRegistry.get<NameComponent>(entity).value = name;
        }
        else
        {
            mRegistry.emplace<NameComponent>(entity, name);
        }

        mNameLookup[name] = entity;
    }

    void World::SetGUID(Entity entity, uint64_t guid)
    {
        if (!mRegistry.valid(entity))return;
        mGUIDLookup[guid] = entity;
    }

    void World::OnComponentAdded(const ComponentAddedEvent& e)
    {
        std::string typeName = e.type.name();
        Log("[Component Added]:%s -> Entity(%d)\n", typeName.c_str(), (uint32_t)e.entity);
    }

    void World::OnComponentRemoved(const ComponentRemovedEvent& e)
    {
        std::string typeName = e.type.name();
        Log("[Component Removed]:%s -> Entity(%d)\n", typeName.c_str(), (uint32_t)e.entity);
    }
}