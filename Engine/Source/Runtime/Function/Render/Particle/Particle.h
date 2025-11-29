#pragma once
#include "ParticleCommon.h"
#include "Runtime/Platform/DirectX12/Buffer/GpuBuffer.h"
#include "Runtime/Platform/DirectX12/Pipline/PiplineState.h"
#include "Runtime/Platform/DirectX12/Core/GraphicsCommon.h"
#include "Runtime/Platform/DirectX12/Context/ComputeContext.h"
#include "Runtime/Resource/TextureRef.h"

namespace AtomEngine
{
	struct ParticleGPU
	{
		Vector3 position;
		Vector3 scale;
		float lifeTime;
		Vector3 velocity;
		float currentTime;
		Color color;
		uint32_t flags;
	};

	struct ParticleVertex
	{
		Vector4 color;
		Vector3 position;
		Vector3 scale;
	};

	class Particle
	{
	public:
		Particle(ParticleProperty& Property);
		~Particle();
		void Initialize();
		void Update(ComputeContext& CompContext, float deltaTime);
		void Render(GraphicsContext& gfxContext);
		float GetLifetime() const { return mProperty.TotalActiveLifetime; }
		float GetElapsedTime() const { return mElapsedTime; }
		void SetTexture(const TextureRef& Texture);
		void Reset();
		// ランタイム編集用 API
		ParticleProperty& GetProperty();
		const ParticleProperty& GetProperty() const;
		void SetProperty(const ParticleProperty& prop);
	private:
		StructuredBuffer mStateBuffers[2];
		StructuredBuffer mInitBuffer;
		uint32_t mCurrentStateBuffer;
		IndirectArgsBuffer mDispatchIndirectArgs;

		ParticleProperty mProperty;
		ParticleProperty mOriginalProperty;
		float mElapsedTime;

		TextureRef mTexture;

	};
}


