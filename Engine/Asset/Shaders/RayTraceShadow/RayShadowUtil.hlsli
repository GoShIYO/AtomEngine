struct RayPayload
{
    bool isHit;
};

struct DynamicCB
{
    float4x4 cameraToWorld;
    float3 worldCameraPosition;
    uint padding;
    float2 resolution;
};

RaytracingAccelerationStructure gAccel : register(t0);

Texture2D<float> gDepth : register(t1);

RWTexture2D<float> gShadowMask : register(u0);

cbuffer CameraCB : register(b0)
{
    float4x4 gInvViewProj;
    float3 gCameraPos;
    float3 gLightDir;
};

cbuffer b1 : register(b1)
{
    DynamicCB gDynamic;
};

float3 GetWorldPosition(float2 uv, float z)
{
    float2 xy = uv * 2.0 - 1.0;
    xy.y = -xy.y;
    
    float4 clipPos = float4(xy, z, 1.0);
    float4 worldPos = mul(clipPos, gInvViewProj);
    return worldPos.xyz / worldPos.w;
}

void GenerateCameraRay(uint2 index, out float3 origin, out float3 direction)
{
    float2 xy = index + 0.5;
    float2 screenPos = xy / gDynamic.resolution * 2.0 - 1.0;

    // DX12 y is flipped
    screenPos.y = -screenPos.y;

    // camera view
    float4 cameraView = mul(float4(screenPos, 0, 1),gDynamic.cameraToWorld);
    float3 world = cameraView.xyz / cameraView.w;
    origin = gDynamic.worldCameraPosition;
    direction = normalize(world - origin);
}
static const float SHADOW_RAY_EPSILON = 0.001f;
static const float FLT_MAX = asfloat(0x7F7FFFFF);