#include "Particle.h"
#include "ParticleSystem.h"
#include "Runtime/Resource/AssetManager.h"
#include "Runtime/Core/Math/Random.h"

namespace AtomEngine
{
	Particle::Particle(ParticleProperty& Property)
	{
		mProperty = Property;
		mElapsedTime = 0.0f;
		if (mProperty.TotalActiveLifetime == 0)
			mProperty.TotalActiveLifetime = FLT_MAX;
		mCurrentStateBuffer = 0;
	}

	Particle::~Particle()
	{
		mStateBuffers[0].Destroy();
		mStateBuffers[1].Destroy();
		mInitBuffer.Destroy();
		mDispatchIndirectArgs.Destroy();
	}

	// ランタイムでプロパティを更新
	ParticleProperty& Particle::GetProperty()
	{
		return mProperty;
	}

	const ParticleProperty& Particle::GetProperty() const
	{
		return mProperty;
	}

	void Particle::SetProperty(const ParticleProperty& prop)
	{
		// 単純に内部プロパティを置き換える
		mProperty = prop;
		mOriginalProperty = prop;
	}

	inline static Vector3 RandSpread(const Vector3& s)
	{
		return Vector3(
			Random::uniform(-s.x, s.x),
			Random::uniform(-s.y, s.y),
			Random::uniform(-s.z, s.z)
		);
	}

	inline static Vector4 RandColor(Vector4 c0, Vector4 c1)
	{
		return Vector4(
			Random::uniform(c0.x, c1.x),
			Random::uniform(c0.y, c1.y),
			Random::uniform(c0.z, c1.z),
			Random::uniform(c0.w, c1.w)
		);
	}

	void Particle::Initialize()
	{
		mOriginalProperty = mProperty;

		ParticleEmitData* pSpawnData = new ParticleEmitData[mProperty.EmitProperties.MaxParticles];
		for (UINT i = 0; i < mProperty.EmitProperties.MaxParticles; i++)
		{
			ParticleEmitData rd = {};

			rd.AgeRate = 1.0f / Random::uniform(mProperty.LifeMinMax.x, mProperty.LifeMinMax.y);

			float horizontalAngle = Random::uniform(0.0f, Math::TwoPI);
			float horizontalVelocity = Random::uniform(mProperty.Velocity.x, mProperty.Velocity.y);

			rd.Velocity.x = horizontalVelocity * cos(horizontalAngle);
			rd.Velocity.y = Random::uniform(mProperty.Velocity.z, mProperty.Velocity.w);
			rd.Velocity.z = horizontalVelocity * sin(horizontalAngle);
			rd.SpreadOffset = RandSpread(mProperty.Spread);
			rd.StartSize = Random::uniform(mProperty.Size.x, mProperty.Size.y);
			rd.EndSize = Random::uniform(mProperty.Size.z, mProperty.Size.w);
			rd.StartColor = RandColor(mProperty.MinStartColor, mProperty.MaxStartColor);
			rd.EndColor = RandColor(mProperty.MinEndColor, mProperty.MaxEndColor);
			rd.Mass = Random::uniform(mProperty.MassMinMax.x, mProperty.MassMinMax.y);
			rd.RotationSpeed = Random::uniform_unit();
			rd.Random = Random::uniform_unit();

			pSpawnData[i] = rd;
		}
		mInitBuffer.Create(L"Particle Init Buffer", mProperty.EmitProperties.MaxParticles, sizeof(ParticleEmitData), pSpawnData);
		delete[] pSpawnData;

		mStateBuffers[0].Create(L"Particle Buffer0", mProperty.EmitProperties.MaxParticles, sizeof(ParticleMotion));
		mStateBuffers[1].Create(L"Particle Buffer1", mProperty.EmitProperties.MaxParticles, sizeof(ParticleMotion));
		mCurrentStateBuffer = 0;

		__declspec(align(16)) UINT DispatchIndirectData[3] = { 0, 1, 1 };
		mDispatchIndirectArgs.Create(L"Particle DispatchIndirectArgs", 1, sizeof(D3D12_DISPATCH_ARGUMENTS), DispatchIndirectData);

	}

	void Particle::Update(ComputeContext& CompContext, float deltaTime)
	{
		if (deltaTime == 0.0f) return;

		mElapsedTime += deltaTime;
		mProperty.EmitProperties.LastEmitPosW = mProperty.EmitProperties.EmitPosW;

		CompContext.SetDynamicConstantBufferView(1, sizeof(ParticleProperty), &mProperty);

		UINT source = mCurrentStateBuffer;
		UINT target = source ^ 1;

		CompContext.SetRootSignature(ParticleSystem::mParticleSig);
		CompContext.SetPipelineState(ParticleSystem::sParticleDispatchIndirectArgsCS);

		CompContext.TransitionResource(mStateBuffers[source], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		CompContext.TransitionResource(mDispatchIndirectArgs, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		CompContext.SetDynamicDescriptor(3, 0, mStateBuffers[source].GetCounterSRV(CompContext));
		CompContext.SetDynamicDescriptor(2, 1, mDispatchIndirectArgs.GetUAV());

		CompContext.Dispatch(1, 1, 1);

		CompContext.TransitionResource(mDispatchIndirectArgs, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

		CompContext.ResetCounter(mStateBuffers[target]);

		CompContext.SetDynamicDescriptor(3, 0, mInitBuffer.GetSRV());
		CompContext.SetDynamicDescriptor(3, 1, mStateBuffers[source].GetSRV());

		CompContext.SetPipelineState(ParticleSystem::sParticleUpdateCS);
		CompContext.TransitionResource(mStateBuffers[target], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		CompContext.SetDynamicDescriptor(2, 1, mStateBuffers[target].GetUAV());
		CompContext.DispatchIndirect(mDispatchIndirectArgs, 0);

		CompContext.InsertUAVBarrier(mStateBuffers[target]);

		CompContext.SetPipelineState(ParticleSystem::sParticleEmitCS);
		CompContext.SetDynamicDescriptor(3, 0, mInitBuffer.GetSRV());
		//u1 (target UAV) は既にバインドされている前提
		UINT NumToSpawn = (UINT)(mProperty.EmitRate * deltaTime);
		if (NumToSpawn > mProperty.EmitProperties.MaxParticles) NumToSpawn = mProperty.EmitProperties.MaxParticles;
		UINT NumSpawnThreads = (NumToSpawn + 63) / 64;
		CompContext.Dispatch((NumSpawnThreads == 0) ? 1 : NumSpawnThreads, 1, 1);

		CompContext.InsertUAVBarrier(mStateBuffers[target]);

		// 切り替え（次フレームの source とする）
		mCurrentStateBuffer = target;
	}

	void Particle::Reset()
	{
		mProperty = mOriginalProperty;
	}
}