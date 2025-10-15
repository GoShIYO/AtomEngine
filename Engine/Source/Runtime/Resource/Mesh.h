#pragma once
#include "Runtime/Core/Math/MathInclude.h"

namespace AtomEngine
{
	struct Vertex
	{
		Vector3 position;
        Vector2 texcoord;
        Vector2 texcoord1;
		Vector3 normal;
        Vector4 tangent;
	};

	struct JointVertex
	{
		Vertex vertex;
		uint16_t jointIndices[4];
		float weights[4];
	};

	struct Mesh
	{
        float    bounds[4];     
        uint32_t vbOffset;      
        uint32_t vbSize;        
        uint32_t vbDepthOffset; 
        uint32_t vbDepthSize;   
        uint32_t ibOffset;      
        uint32_t ibSize;        
        uint8_t  vbStride;      
        uint8_t  ibFormat;      
        uint16_t meshCBV;       
        uint16_t materialCBV;   
        uint16_t srvTable;      
        uint16_t samplerTable;  
        uint16_t psoFlags;      
        uint16_t pso;           
        uint16_t numJoints;     
        uint16_t startJoint;    
        uint16_t numDraws;      

        struct Draw
        {
            uint32_t primCount;
            uint32_t startIndex;
            uint32_t baseVertex;
        };

        Draw draw[1];
	};
}