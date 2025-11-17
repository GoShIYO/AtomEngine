#include"GameObject.h"
#include "World.h"

namespace AtomEngine
{
    GameObject::GameObject(Entity handle, World* world)
        : mHandle(handle), mWorld(world)
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

}
