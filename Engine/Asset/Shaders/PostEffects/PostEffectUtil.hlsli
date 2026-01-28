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

float ToneMapLuma(float luma)
{
    return 1 - exp2(-luma);
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

// The standard 32-bit HDR color format.  Each float has a 5-bit exponent and no sign bit.
uint Pack_R11G11B10_FLOAT(float3 rgb)
{
    // Clamp upper bound so that it doesn't accidentally round up to INF 
    // Exponent=15, Mantissa=1.11111
    rgb = min(rgb, asfloat(0x477C0000));
    uint r = ((f32tof16(rgb.x) + 8) >> 4) & 0x000007FF;
    uint g = ((f32tof16(rgb.y) + 8) << 7) & 0x003FF800;
    uint b = ((f32tof16(rgb.z) + 16) << 17) & 0xFFC00000;
    return r | g | b;
}

float3 Unpack_R11G11B10_FLOAT(uint rgb)
{
    float r = f16tof32((rgb << 4) & 0x7FF0);
    float g = f16tof32((rgb >> 7) & 0x7FF0);
    float b = f16tof32((rgb >> 17) & 0x7FE0);
    return float3(r, g, b);
}
