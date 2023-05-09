#include "macros.hlsli"

cbuffer EntityCBuf : REG_ENTITYCBUF
{
    float4 color;
};

Texture2D tex : register(t0);
SamplerState samplerState : register(s0);

float4 main(float2 texCoord : TEXCOORD) : SV_Target
{
    return color * tex.Sample(samplerState, texCoord);
}