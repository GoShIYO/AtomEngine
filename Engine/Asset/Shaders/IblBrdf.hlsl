RWTexture2D<float4> BRDFResult : register(u0);

#define PI 3.14159265358979323f
#define INV_PI 0.31830988618379067239521257108191f

// Geometry term
// http://graphicrants.blogspot.com.au/2013/08/specular-brdf-reference.html
// I could not have arrived at this without the notes at :
// http://www.gamedev.net/topic/658769-ue4-ibl-glsl/

float GGX(float NoV, float roughness)
{
    // http://graphicrants.blogspot.com.au/2013/08/specular-brdf-reference.html
    // Schlick-Beckmann G.
    float k = roughness / 2;
    return NoV / (NoV * (1.0f - k) + k);
}

float geometryForLut(float roughness, float NoL)
{
    return GGX(NoL, roughness * roughness);
}

// Visibility term
float visibilityForLut(float roughness, float NoV)
{
    return GGX(NoV, roughness * roughness);
}

// Fresnel Term.
// Inputs, view dot half angle.
float fresnelForLut(float VoH)
{
    return pow(1.0 - VoH, 5);
}


// Summation of Lut term while iterating over samples
float2 sumLut(float2 current, float G, float V, float F, float VoH, float NoL, float NoH, float NoV)
{
    G = G * V;
    float G_Vis = G * VoH / (NoH * NoV);
    current.x += (1.0 - F) * G_Vis;
    current.y += F * G_Vis;

    return current;
}

//------------------------------------------------------------------------------------//
// Used by IblBrdf.hlsl generation and IblImportanceSamplingSpecular.fx               //
// Inputs:                                                                            //
//   Spherical hammersley generated coordinate and roughness.                         //
//   Roughness                                                                        //
//   Normal                                                                           //
// Base on GGX example in:                                                            //
// http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
//------------------------------------------------------------------------------------//
float3 importanceSampleGGX(float2 Xi, float roughness, float3 N)
{
    float a = roughness * roughness;

    float Phi = 2 * PI * Xi.x;
    float CosTheta = sqrt((1 - Xi.y) / (1 + (a * a - 1) * Xi.y));
    float SinTheta = sqrt(1 - CosTheta * CosTheta);

    float3 H;
    H.x = SinTheta * cos(Phi);
    H.y = SinTheta * sin(Phi);
    H.z = CosTheta;

    float3 UpVector = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 TangentX = normalize(cross(UpVector, N));
    float3 TangentY = cross(N, TangentX);

    return TangentX * H.x + TangentY * H.y + N * H.z;
}


//------------------------------------------------------------------------------------//
// Shader functions used by IblImportanceSamplingSpecular.fx                          //
//------------------------------------------------------------------------------------//
// D(h) for GGX.
// http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html
float specularD(float roughness, float NoH)
{
    float r2 = roughness * roughness;
    float NoH2 = NoH * NoH;
    float a = 1.0 / (PI * r2 * pow(NoH, 4));
    float b = exp((NoH2 - 1) / r2 * NoH2);
    return a * b;
}

float4 sumSpecular(float3 hdrPixel, float NoL, float4 result)
{
    result.xyz += (hdrPixel * NoL);
    result.w += NoL;
    return result;
}

//------------------------------------------------------------------------------------//
// Shader functions used by IblImportanceSamplingDiffuse.fx                           //
//------------------------------------------------------------------------------------//
//
// Derived from GGX example in:
// http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
// Image Based Lighting.
//
float3 importanceSampleDiffuse(float2 Xi, float3 N)
{
    float CosTheta = 1.0 - Xi.y;
    float SinTheta = sqrt(1.0 - CosTheta * CosTheta);
    float Phi = 2 * PI * Xi.x;

    float3 H;
    H.x = SinTheta * cos(Phi);
    H.y = SinTheta * sin(Phi);
    H.z = CosTheta;

    float3 UpVector = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 TangentX = normalize(cross(UpVector, N));
    float3 TangentY = cross(N, TangentX);

    return TangentX * H.x + TangentY * H.y + N * H.z;
}


// Sum the diffuse term while iterating over all samples.
float4 sumDiffuse(float3 diffuseSample, float NoV, float4 result)
{
    result.xyz += diffuseSample;
    result.w++;
    return result;
}
float radicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10f; // / 0x100000000
}

float2 hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), radicalInverse_VdC(i));
}

float2 integrate(float roughness, float NoV)
{
    float3 N = float3(0.0f, 0.0f, 1.0f);
    float3 V = float3(sqrt(1.0f - NoV * NoV), 0.0f, NoV);
    float2 result = float2(0, 0);

    const uint NumSamples = 1024;

    precise float Vis = visibilityForLut(roughness, NoV);

    for (uint i = 0; i < NumSamples; i++)
    {
        float2 Xi = hammersley(i, NumSamples);
        float3 H = importanceSampleGGX(Xi, roughness, N);
        precise float3 L = 2.0f * dot(V, H) * H - V;

        float NoL = saturate(L.z);
        float NoH = saturate(H.z);
        float VoH = saturate(dot(V, H));
        float NoV = saturate(dot(N, V));
        if (NoL > 0)
        {
            precise float G = geometryForLut(roughness, NoL);
            precise float F = fresnelForLut(VoH);
            result = sumLut(result, G, Vis, F, VoH, NoL, NoH, NoV);
        }
    }

    result.x = (result.x / float(NumSamples));
    result.y = (result.y / float(NumSamples));

    return result;
}

[numthreads(16, 16, 1)]
void CSMain(uint2 id : SV_DispatchThreadID)
{
    float roughness = (float) (id.y + 0.5) / 512.0;
    float NoV = (float) (id.x + 0.5) / 512.0;
      
    float2 result = integrate(roughness, NoV);

    BRDFResult[int2(id.x, 255 - id.y)] = float4(result.x, result.y, roughness, 1);
}