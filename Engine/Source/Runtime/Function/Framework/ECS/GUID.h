#pragma once
#include <cstdint>
#include <atomic>

namespace AtomEngine
{
    class ObjectGUID
    {
    public:
        //オブジェクト生成時に自動でGUIDを割り当てる
        ObjectGUID() : mValue(++sCounter) {}
        explicit ObjectGUID(uint64_t value) : mValue(value) {}
        uint64_t Value() const { return mValue; }

        bool operator==(const ObjectGUID& other) const { return mValue == other.mValue; }
        bool operator!=(const ObjectGUID& other) const { return !(*this == other); }

    private:
        uint64_t mValue;
        static std::atomic<uint64_t> sCounter;
    };
}


