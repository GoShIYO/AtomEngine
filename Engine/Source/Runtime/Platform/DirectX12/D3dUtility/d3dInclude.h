#pragma once
#define NOMINMAX
#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <dxgidebug.h>
#include <dxcapi.h>
#include "d3dx12.h"

#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

#include "Runtime/Core/Utility/Utility.h"
#include "Runtime/Core/Utility/Hash.h"
#include "Runtime/Core/LogSystem/LogSystem.h"
