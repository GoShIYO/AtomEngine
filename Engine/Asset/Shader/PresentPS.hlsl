float3 ApplySRGBCurve(float3 x)
{
    // Approximately pow(x, 1.0 / 2.2)
    return select(x < 0.0031308, 12.92 * x, 1.055 * pow(x, 1.0 / 2.4) - 0.055);
}

Texture2D<float3> ColorTex : register(t0);

float4 main(float4 position : SV_Position) : SV_Target0
{
    float3 LinearRGB = ColorTex[(int2) position.xy];
    float4 output = float4(ApplySRGBCurve(LinearRGB), 1.0f);
    return output;
}