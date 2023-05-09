#include "macros.hlsli"

cbuffer SystemCBuf : REG_SYSTEMCBUF
{
    float gamma;
    float3 p0;
};

Texture2D tex : register(t0);
SamplerState samplerState : register(s0);

float4 main(float2 texCoord : TEXCOORD) : SV_Target
{
    float4 color = tex.Sample(samplerState, texCoord);
    return float4(pow(color.rgb, 1.0f / gamma), color.a);
}