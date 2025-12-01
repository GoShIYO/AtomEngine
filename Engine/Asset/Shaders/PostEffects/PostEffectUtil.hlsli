float3 REC709toREC2020(float3 RGB709)
{
    static const float3x3 ConvMat =
    {
        0.627402, 0.329292, 0.043306,
        0.069095, 0.919544, 0.011360,
        0.016394, 0.088028, 0.895578
    };
    return mul(ConvMat, RGB709);
}

float RGBToLuminance(float3 x)
{
    return dot(x, float3(0.212671, 0.715160, 0.072169)); // Defined by sRGB/Rec.709 gamut
}

float LinearToLogLuminance(float x, float gamma = 4.0)
{
    return log2(lerp(1, exp2(gamma), x)) / gamma;
}

float RGBToLogLuminance(float3 x, float gamma = 4.0)
{
    return LinearToLogLuminance(RGBToLuminance(x), gamma);
}

float3 TM_Reinhard(float3 hdr, float k = 1.0)
{
    return hdr / (hdr + k);
}

float3 TM_Stanard(float3 hdr)
{
    return TM_Reinhard(hdr * sqrt(hdr), sqrt(4.0 / 27.0));
}
