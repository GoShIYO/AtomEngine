#pragma once
#include "Runtime/Resource/Mesh.h"

namespace AtomEngine
{
	
	class GeometryGenerator
	{ 
	public:

		static Mesh* CreateBox(float xLength, float yLength, float zLength);
		static Mesh* CreateSphere(float radius, uint32_t sliceCount, uint32_t stackCount);
		static Mesh* CreateSphereTessellation(float radius, uint32_t subdivisionNum);
		static Mesh* CreateCircle(float radius, uint32_t sliceCount);
		static Mesh* CreateCone(float radius, float height, uint32_t sliceCount);
		static Mesh* CreateCylinder(float topRadius, float bottomRadius, float height, uint32_t sliceCount, uint32_t stackCount);
		static Mesh* CreatePlane(float xLength, float zLength, uint32_t xSplit, uint32_t zSplit);
		static Mesh* CreateQuad(float xLength, float yLength);


	};
}