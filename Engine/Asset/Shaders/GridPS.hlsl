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

float ComputeDepthFromWorldPos(float3 worldPos)
{
    float4 clip = mul(float4(worldPos, 1.0), ViewProj);
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

struct PSOutput
{
    float4 color : SV_Target;
    float depth : SV_Depth;
};

PSOutput main(VSOutput input)
{
    float3 ro = CameraPos;
    float3 rd = normalize(input.viewDir);

    float denom = rd.y;
    if (abs(denom) < 1e-6)
        discard;

    float t = -ro.y / denom;
    if (t < 0)
        discard;

    float3 worldPos = ro + rd * t;

    //メイングリッド
    const float majorStep = 10.0;
    
    float2 uv = worldPos.xz / majorStep;
    float2 dpdx_uv = ddx(uv);
    float2 dpdy_uv = ddy(uv);

    float majorMask = filteredGrid(uv, dpdx_uv, dpdy_uv, 500);
    
    //サブグリッド
    float2 minjorUV = worldPos.xz;
    float2 minjor_dpdx_uv = ddx(minjorUV);
    float2 minjor_dpdy_uv = ddy(minjorUV);

    float minorMask = filteredGrid(minjorUV, minjor_dpdx_uv, minjor_dpdy_uv, 100);
    
    float majorLine = saturate(1.0 - majorMask);
    float minorLine = saturate(1.0 - minorMask);
    
    //原点の色
    float3 originColor = float3(1.0, 1.0, 1.0);
    float3 gridColor = lerp(minorLineColor, majorLineColor, majorLine);

    float gridAlpha = max(majorLine, minorLine);
    
    //x軸の線
    float ddx_x = ddx(worldPos.x);
    float ddy_x = ddy(worldPos.x);
    float axisWidth_x = max(abs(ddx_x), abs(ddy_x));
    //z軸の線
    float ddx_z = ddx(worldPos.z);
    float ddy_z = ddy(worldPos.z);
    float axisWidth_z = max(abs(ddx_z), abs(ddy_z));

    const float fade = 0.02;
    float x_axis_mask = smoothstep(fade + axisWidth_x, fade - axisWidth_x, abs(worldPos.x));
    float z_axis_mask = smoothstep(fade + axisWidth_z, fade - axisWidth_z, abs(worldPos.z));

    float axisX = saturate(x_axis_mask);
    float axisZ = saturate(z_axis_mask);
    
    float3 axisColor = xAxisColor * axisX + zAxisColor * axisZ;
    float overlap = axisX * axisZ;
    axisColor = lerp(axisColor, originColor, overlap);

    float axisAlpha = max(axisX, axisZ);
    //色を合成
    float3 color = lerp(gridColor, axisColor, axisAlpha);
    
    //線の部分だけ色を付ける
    float alpha = max(majorLine, minorLine);
    alpha = max(alpha, axisAlpha);

    float dist = length(worldPos - CameraPos);
    alpha *= saturate(1.0 - dist / farClip);

    PSOutput o;
    o.color = float4(color, alpha);
    o.depth = ComputeDepthFromWorldPos(worldPos);
    return o;
}
