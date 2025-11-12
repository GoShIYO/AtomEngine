#include "GUID.h"

namespace AtomEngine
{
	std::atomic<uint64_t> ObjectGUID::sCounter{ 0 };
}