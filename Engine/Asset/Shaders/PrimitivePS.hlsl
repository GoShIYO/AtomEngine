
struct VSOutput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
};
float4 main(VSOutput i) : SV_TARGET
{
    return i.color;
}
