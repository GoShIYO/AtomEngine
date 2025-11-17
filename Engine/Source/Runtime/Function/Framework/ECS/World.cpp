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
	}

	std::unique_ptr<GameObject> World::CreateGameObject(const std::string& name)
	{
		Entity e = mRegistry.create();
		auto obj = std::make_unique<GameObject>(e, this);
		if (!name.empty())
		{
			obj->AddComponent<DeathFlag>();
			obj->AddComponent<NameComponent>(name);
			mNameLookup[name] = e;
		}
		mEntityToObject[e] = obj.get();
		Log("[Object Added]:%s -> Entity(%d)\n", name.c_str(), (uint32_t)e);

		return std::move(obj);
	}

	GameObject* World::GetGameObject(Entity entity)
	{
		if (mEntityToObject.find(entity) != mEntityToObject.end())
		{
			return mEntityToObject[entity];
		}
		return nullptr;
	}

	void World::DestroyGameObject(Entity entity)
	{
		auto it = mEntityToObject.find(entity);
		if (it != mEntityToObject.end())
		{
			auto* obj = it->second;
			if (obj->HasComponent<NameComponent>())
			{
				Log("[Object Destroyed]:%s -> Entity(%d)\n",
					obj->GetComponent<NameComponent>().value.c_str(), (uint32_t)entity);
				mNameLookup.erase(obj->GetComponent<NameComponent>().value);
			}
			mEntityToObject.erase(it);
		}
		mRegistry.destroy(entity);
	}

	Entity World::FindByName(const std::string& name)
	{
		auto it = mNameLookup.find(name);
		if (it != mNameLookup.end())
			return it->second;
		return entt::null;
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