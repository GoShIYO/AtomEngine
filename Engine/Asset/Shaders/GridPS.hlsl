cbuffer GridCB : register(b0)
{
    float4x4 ViewProj;

    float3 CameraPos;
    float farClip;

    float3 xAxisColor;
    float3 zAxisColor;

    float3 majorLineColor;
    float3 minorLineColor;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 viewDir : TEXCOORD3;
};

float2 fract(float2 x)
{
    return x - floor(x);
}

float ComputeDepthFromWorldPos(float3 p)
{
    float4 clip = mul(float4(p, 1.0), ViewProj);
    return clip.z / clip.w;
}

//Ref:https://iquilezles.org/articles/filterableprocedurals/
float filteredCheckers(float2 p, float2 dpdx, float2 dpdy)
{
    float2 w = max(abs(dpdx), abs(dpdy));
    float2 i = 2.0 * (abs(fract((p - 0.5 * w) * 0.5) - 0.5) -
                  abs(fract((p + 0.5 * w) * 0.5) - 0.5)) / w;

    return 0.5 - 0.5 * i.x * i.y;
}
//Ref:https://iquilezles.org/articles/filterableprocedurals/
float filteredGrid(float2 p, float2 dpdx, float2 dpdy, float N)
{
    float2 w = max(abs(dpdx), abs(dpdy));
    float2 a = p + 0.5 * w;
    float2 b = p - 0.5 * w;

    float2 i = (floor(a) + min(fract(a) * N, 1.0) -
                floor(b) - min(fract(b) * N, 1.0)) / (N * w);

    return (1.0 - i.x) * (1.0 - i.y);
}
//Ref::blender
float3 get_axes(float3 pos, float3 fw, float lineSize)
{
    float3 d = abs(pos);
    d /= fw;

    return 1.0 - smoothstep(lineSize, lineSize + 1.0, d);
}

struct PSOutput
{
    float4 color : SV_Target;
    float depth : SV_Depth;
};

PSOutput main(VSOutput input)
{
    float3 ro = CameraPos;
    float3 rd = normalize(input.viewDir);

    if (abs(rd.y) < 1e-6)
        discard;

    float t = -ro.y / rd.y;
    if (t < 0)
        discard;

    float3 worldPos = ro + rd * t;
    float3 P = worldPos;

    // fwidth
    float3 fw = abs(ddx(P)) + abs(ddy(P));

    //angle fade & alpha
    float3 viewToGrid = normalize(CameraPos - P);
    float angle = 1.0 - abs(viewToGrid.y);
    angle = angle * angle;
    float fade = 1.0 - angle * angle;

    float dist = length(CameraPos - P);
    fade *= saturate(1.0 - dist / (farClip * 0.5f));

    //メインuv
    float2 uvMajor = P.xz / 10.0;
    //サブ線uv
    float2 uvMinor = P.xz;

    float2 dpdxMajor = ddx(uvMajor);
    float2 dpdyMajor = ddy(uvMajor);

    float2 dpdxMinor = ddx(uvMinor);
    float2 dpdyMinor = ddy(uvMinor);

    float majorMask = filteredGrid(uvMajor, dpdxMajor, dpdyMajor, 500);
    float minorMask = filteredGrid(uvMinor, dpdxMinor, dpdyMinor, 100);
    //メインライン
    float majorLine = saturate(1.0 - majorMask);
    //サブライン
    float minorLine = saturate(1.0 - minorMask);
    
    // xz轴
    float3 axes = get_axes(P, fw, 0.5f);

    float axisX = axes.x;
    float axisZ = axes.z;
    
    float axisAlpha = max(axisX, axisZ);

    float3 axisColor = axisX * xAxisColor + axisZ * zAxisColor;

    float3 gridColor = lerp(minorLineColor, majorLineColor, majorLine);

    float3 color = lerp(gridColor, axisColor, axisAlpha);

    float alpha = max(max(majorLine, minorLine), axisAlpha);
    alpha *= fade;

    PSOutput o;
    o.color = float4(color, alpha);
    o.depth = ComputeDepthFromWorldPos(worldPos);

    return o;
}
