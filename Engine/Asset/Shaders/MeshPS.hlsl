Texture2D gTexture : register(t0);
SamplerState gLinear : register(s0);

struct VSOutput
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
    float4 color : COLOR0;
};

float4 main(VSOutput input) : SV_TARGET
{
    float4 textureColor = gTexture.Sample(gLinear, input.uv);
    float3 color = input.color.rgb * textureColor.rgb;
    float alpha = input.color.a * textureColor.a;
    return float4(color, alpha);
}