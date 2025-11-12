#include"GameObject.h"
#include "World.h"

namespace AtomEngine
{
    GameObject::GameObject(Entity handle, World* world)
        : mHandle(handle), mWorld(world), mGUID(ObjectGUID())
    {
    }

    bool GameObject::IsValid() const
    {
        return mHandle != entt::null && mWorld != nullptr;
    }

    void GameObject::Destroy()
    {
        if (IsValid()) mWorld->DestroyGameObject(mHandle);
    }

    std::string GameObject::GetName()
    {
        if (HasComponent<NameComponent>())
            return GetComponent<NameComponent>().value;
        return {};
    }

    void GameObject::SetName(const std::string& name)
    {
       mWorld->SetName(mHandle, name);
    }

    std::string GameObject::GetTag()
    {
        if (HasComponent<TagComponent>())
            return GetComponent<TagComponent>().tag;
        return {};
    }

    void GameObject::SetTag(const std::string& tag)
    {
        mWorld->SetTag(mHandle, tag);
    }
}
