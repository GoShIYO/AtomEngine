#include"ParticleUtil.hlsli"

StructuredBuffer<ParticleVertex> gParticles : register(t0);

struct VSInput
{
	float3 pos : POSITION;
	float2 texcoord : TEXCOORD;
};

VSOutput main(VSInput input, uint instanceID : SV_InstanceID)
{
	VSOutput output;
	
	ParticleVertex particle = gParticles[instanceID];
    float4x4 worldMat = billboardMat;
	
	worldMat[0] *= particle.scale.x;
	worldMat[1] *= particle.scale.y;
	worldMat[2] *= particle.scale.z;
    worldMat[3].xyz = particle.position;
	
    output.position = mul(float4(input.pos, 1.0f), mul(worldMat, ViewProj));
	output.texcoord = input.texcoord;
	output.color = particle.color;
	
	return output;
}