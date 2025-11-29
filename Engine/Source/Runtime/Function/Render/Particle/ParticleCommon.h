#pragma once
#include "Runtime/Core/Math/MathInclude.h"

namespace AtomEngine
{
	struct PrewView
	{
		Matrix4x4 ViewProj;
		Matrix4x4 billboardMat;
		float time;
		float deltaTime;
	};

	struct ParticleEmitData
	{
		float AgeRate;
		float RotationSpeed;
		float StartSize;
		float EndSize;
		Vector3 Velocity; 
		float Mass;
		Vector3 SpreadOffset;
		float Random;
		Vector4 StartColor;
		Vector4 EndColor;
	};

	struct ParticleMotion
	{
		Vector4 Color;
		Vector3 Position;
		float Mass;
		Vector3 Velocity;
		float Age;
		float Size;
		float Rotation;
		uint32_t ResetDataIndex;
	};

	_declspec(align(16)) struct EmitterProperty
	{
		Vector3 LastEmitPosW;
		float EmitSpeed = 1.0f;
		Vector3 EmitPosW = { 0.0f,0.0f,0.0f};
		float FloorHeight = -0.7f;
		Vector3 EmitDirW = { 0.0f,0.0f,1.0f };
		float Restitution = 0.6f;
		Vector3 EmitRightW = { 1.0f,0.0f,0.0f };
		float EmitterVelocitySensitivity;
		Vector3 EmitUpW = { 0.0,1.0,0.0 };
		uint32_t MaxParticles = 500;
		Vector3 Gravity = { 0,-5,0 };
		float pad0;
		Vector3 EmissiveColor;
		float pad1;
	};

	_declspec(align(16)) struct ParticleProperty
	{
		Vector4 MinStartColor;
		Vector4 MaxStartColor;
		Vector4 MinEndColor;
		Vector4 MaxEndColor;
		EmitterProperty  EmitProperties;
		Vector4 Velocity;
		Vector4 Size;
		Vector3 Spread;
		float EmitRate;
		Vector2 LifeMinMax;
		Vector2 MassMinMax;

		//Non Shader
		std::wstring TexturePath;
		float TotalActiveLifetime;

		ParticleProperty()
		{
			MinStartColor = Vector4(0.8f, 0.8f, 1.0f,1.0f);
			MaxStartColor = Vector4(0.9f, 0.9f, 1.0f,1.0f);
			MinEndColor = Vector4(1.0f, 1.0f, 1.0f,1.0f);
			MaxEndColor = Vector4(1.0f, 1.0f, 1.0f,1.0f);
			EmitProperties = EmitterProperty();
			EmitRate = 200;
			LifeMinMax = Vector2(1.0f, 2.0f);
			MassMinMax = Vector2(0.5f, 1.0f);
			Size = Vector4(0.07f, 0.7f, 0.8f, 0.8f); // (Start size min, Start size max, End size min, End size max) 		
			Spread = Vector3(0.5f, 1.5f, 0.1f);
			TexturePath = L"Asset/Textures/circle2.dds";
			TotalActiveLifetime = 20.0;
			Velocity = Vector4(0.5, 3.0, -0.5, 3.0); // (X velocity min, X velocity max, Y velocity min, Y velocity max)
		};
	};

	constexpr uint32_t kMaxParticles = 0x8000;
}