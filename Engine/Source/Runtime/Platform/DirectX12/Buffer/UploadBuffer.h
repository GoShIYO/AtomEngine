#pragma once
#include "GpuResource.h"

namespace AtomEngine
{
    class UploadBuffer : public GpuResource
    {
    public:
        virtual ~UploadBuffer() { Destroy(); }

        void Create(const std::wstring& name, size_t BufferSize);

        void* Map(void);
        void Unmap(size_t begin = 0, size_t end = -1);

        size_t GetBufferSize() const { return mBufferSize; }

    protected:

        size_t mBufferSize;
    };
}
