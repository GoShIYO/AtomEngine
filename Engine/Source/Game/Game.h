#pragma once
#include "Runtime/Application/GameApp.h"
#include "Runtime/Resource/AssetManager.h"

using namespace AtomEngine;
class Game :
	public GameApp
{
public:
	void Initialize() override;
	void Update(float deltaTime) override;
	void Render()override;
	void Shutdown() override;
	bool Exit() override;
private:
	World world;

	RootSignature m_RootSignature;
	ByteAddressBuffer m_VertexBuffer;
	ByteAddressBuffer m_IndexBuffer;
	GraphicsPSO m_PSO;

	Camera m_Camera;
	Matrix4x4 m_ViewProjMatrix;
	D3D12_VIEWPORT m_MainViewport;
	D3D12_RECT m_MainScissor;

	ModelInstance m_Model;

	float	g_SunLightIntensity = 4.0f;
	float g_SunOrientation = -0.5f;
	float g_SunInclination = 0.75f;

	float m_radius = 5.0f;
	float m_xRotate = 0.0f;
	float m_xLast = 0.0f;
	float m_xDiff = 0.0f;
	float m_yRotate = 0.0f;
	float m_yLast = 0.0f;
	float m_yDiff = 0.0f;
};
