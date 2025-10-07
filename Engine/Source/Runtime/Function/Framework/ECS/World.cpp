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
    }

    GameObject World::CreateGameObject(const std::string& name)
    {
        Entity e = mRegistry.create();
        GameObject obj(e, this);

        if (!name.empty())
        {
            obj.AddComponent<NameComponent>(name);
            mNameLookup[name] = e;
        }

        return obj;
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

    GameObject World::FindByName(const std::string& name)
    {
        auto it = mNameLookup.find(name);
        if (it != mNameLookup.end())
            return GameObject(it->second, this);
        return {};
    }

    GameObject World::FindByTag(const std::string& tag)
    {
        auto range = mTagLookup.equal_range(tag);
        if (range.first != range.second)
            return GameObject(range.first->second, this);
        return {};
    }

    void World::OnComponentAdded(const ComponentAddedEvent& e) 
    {
        std::string typeName = e.type.name();
        Log(L"[Component Added]:%s -> Entity(%d)", typeName.c_str(), (int)e.entity);
    }

    void World::OnComponentRemoved(const ComponentRemovedEvent& e)
    {
        std::string typeName = e.type.name();
        Log(L"[Component Removed]:%s -> Entity(%d)", typeName.c_str(), (int)e.entity);
    }
}