#pragma once

struct ItemComponent
{
	int itemId{ 0 };
	bool isPickedUp{ false };
	float rotationSpeed{ 2.0f };
	float bobSpeed{ 3.0f };
	float bobAmount{ 0.3f };
	float baseY{ 0.0f };
	float baseScale{ 1.0f };
	float currentTime{ 0.0f };
};

struct ItemCreateInfo
{
	AtomEngine::Vector3 position{ 0.0f, 0.0f, 0.0f };
	std::wstring modelPath = L"Asset/Models/item/item.gltf";
	float scale{ 1.0f };
	std::string name = "Item";
};