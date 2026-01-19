#pragma once
#include "ParticleCommon.h"
#include "Runtime/Platform/DirectX12/Buffer/GpuBuffer.h"
#include "Runtime/Platform/DirectX12/Pipeline/PipelineState.h"
#include "Runtime/Platform/DirectX12/Core/GraphicsCommon.h"
#include "Runtime/Platform/DirectX12/Context/ComputeContext.h"
#include "Runtime/Resource/TextureRef.h"

namespace AtomEngine
{
	struct ParticleVertex
	{
		Vector4 Color;
		Vector3 Position;
		uint32_t TextureID;
		Vector3 Scale;
	};

	class Particle
	{
	public:
		Particle(ParticleProperty& Property);
		~Particle();
		void Initialize();
		void Update(ComputeContext& CompContext, float deltaTime);
		float GetLifetime() const { return mProperty.TotalActiveLifetime; }
		float GetElapsedTime() const { return mElapsedTime; }
		void Reset();
		// ランタイム編集用 API
		ParticleProperty& GetProperty();
		const ParticleProperty& GetProperty() const;
		void SetProperty(const ParticleProperty& prop);
		void Emit(const Vector3& emitPosW);

	private:
		StructuredBuffer mStateBuffers[2];
		StructuredBuffer mInitBuffer;
		uint32_t mCurrentStateBuffer;
		IndirectArgsBuffer mDispatchIndirectArgs;

		ParticleProperty mProperty;
		ParticleProperty mOriginalProperty;
		float mElapsedTime;

	};
}


