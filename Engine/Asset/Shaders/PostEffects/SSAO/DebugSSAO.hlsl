Texture2D<float> SsaoBuffer : register(t0);
RWTexture2D<float3> OutColor : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    OutColor[DTid.xy] = SsaoBuffer[DTid.xy].xxx;
}