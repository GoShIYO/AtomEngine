//to 1d functions

//get a scalar random value from a 3d value
float rand3dTo1d(float3 value, float3 dotDir = float3(12.9898, 78.233, 37.719))
{
	//make value smaller to avoid artefacts
    float3 smallValue = sin(value);
	//get scalar value from 3d vector
    float random = dot(smallValue, dotDir);
	//make value more random by making it bigger and then taking the factional part
    random = frac(sin(random) * 143758.5453);
    return random;
}

float rand2dTo1d(float2 value, float2 dotDir = float2(12.9898, 78.233))
{
    float2 smallValue = sin(value);
    float random = dot(smallValue, dotDir);
    random = frac(sin(random) * 143758.5453);
    return random;
}

float rand1dTo1d(float3 value, float mutator = 0.546)
{
    float random = frac(sin(value + mutator) * 143758.5453);
    return random;
}

//to 2d functions

float2 rand3dTo2d(float3 value)
{
    return float2(
		rand3dTo1d(value, float3(12.989, 78.233, 37.719)),
		rand3dTo1d(value, float3(39.346, 11.135, 83.155))
	);
}

float2 rand2dTo2d(float2 value)
{
    return float2(
		rand2dTo1d(value, float2(12.989, 78.233)),
		rand2dTo1d(value, float2(39.346, 11.135))
	);
}

float2 rand1dTo2d(float value)
{
    return float2(
		rand2dTo1d(value, 3.9812),
		rand2dTo1d(value, 7.1536)
	);
}

//to 3d functions

float3 rand3dTo3d(float3 value)
{
    return float3(
		rand3dTo1d(value, float3(12.989, 78.233, 37.719)),
		rand3dTo1d(value, float3(39.346, 11.135, 83.155)),
		rand3dTo1d(value, float3(73.156, 52.235, 09.151))
	);
}

float3 rand2dTo3d(float2 value)
{
    return float3(
		rand2dTo1d(value, float2(12.989, 78.233)),
		rand2dTo1d(value, float2(39.346, 11.135)),
		rand2dTo1d(value, float2(73.156, 52.235))
	);
}

float3 rand1dTo3d(float value)
{
    return float3(
		rand1dTo1d(value, 3.9812),
		rand1dTo1d(value, 7.1536),
		rand1dTo1d(value, 5.7241)
	);
}

float wangHashFloat(uint x)
{
    x = (x ^ 61u) ^ (x >> 16);
    x *= 9u;
    x = x ^ (x >> 4);
    x *= 0x27d4eb2du;
    x = x ^ (x >> 15);
    return asfloat((x & 0x007FFFFFu) | 0x3f800000u) - 1.0f; // [0,1)
}

uint uhash(uint a, uint b, uint c)
{
    // simple 3-int mixer
    uint x = a + 0x9e3779b9u + (b << 6) + (b >> 2);
    uint y = b + 0x9e3779b9u + (c << 6) + (c >> 2);
    uint z = c + 0x9e3779b9u + (a << 6) + (a >> 2);
    x ^= (y >> 13);
    y ^= (z << 8);
    z ^= (x >> 13);
    return x ^ y ^ z;
}

float random01(uint i, uint dtX, uint seed)
{
    uint h = uhash(i, dtX, seed);
    return wangHashFloat(h);
}

float3 randUnitSphere(uint i, uint dtX, uint seed)
{
    // Marsaglia method using two random floats
    float u = random01(i, dtX, seed) * 2.0f - 1.0f;
    float v = random01(i + 1, dtX, seed) * 2.0f - 1.0f;
    float s = u * u + v * v;
    if (s >= 1.0f || s == 0.0f)
    {
        // fallback: rehash
        return float3(0, 1, 0);
    }
    float factor = sqrt(1.0f - s);
    return normalize(float3(2.0f * u * factor, 1.0f - 2.0f * s, 2.0f * v * factor));
}