cbuffer PerObject : register(b2)
{
    float3 lightPos;
    float farPlane;
};

struct PS_IN
{
    float4 position : SV_POSITION;
    float3 worldPos : WORLDPOS;
};

float main(PS_IN input) : SV_TARGET
{
    float lightDistance = length(input.worldPos - lightPos);
    lightDistance = lightDistance / farPlane;
    return lightDistance;
}
