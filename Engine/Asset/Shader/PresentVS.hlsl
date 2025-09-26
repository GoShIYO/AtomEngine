void main(
    in uint VertID : SV_VertexID,
    out float4 pos : SV_Position,
    out float2 uv : TexCoord0
)
{
    uv = float2(uint2(VertID, VertID << 1) & 2);
    pos = float4(lerp(float2(-1, 1), float2(1, -1), uv), 0, 1);
}