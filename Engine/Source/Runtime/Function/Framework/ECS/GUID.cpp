#include "GUID.h"

namespace AtomEngine
{
	std::atomic<uint64_t> GUID::sCounter{ 0 };
}