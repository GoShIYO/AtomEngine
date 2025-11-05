#pragma once
#include <entt.hpp>
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <typeindex>
#include <cassert>
#include <utility>

namespace AtomEngine
{
    using namespace entt;

    using Entity = entt::entity;

    struct NameComponent
    {
        std::string value;
    };

    struct TagComponent
    {
        std::string tag;
    };
}
