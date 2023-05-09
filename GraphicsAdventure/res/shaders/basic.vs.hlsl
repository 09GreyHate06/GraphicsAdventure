#include "macros.hlsli"

struct VSOutput
{
    float2 texCoord : TEXCOORD;
    float4 position : SV_Position;
};

cbuffer SystemCBuf : REG_SYSTEMCBUF
{
    float4x4 viewProjection;
};

cbuffer EntityCBuf : REG_ENTITYCBUF
{
    float4x4 transform;
};

VSOutput main(float3 position : POSITION, float2 texCoord : TEXCOORD)
{
    VSOutput vso;
    vso.position = mul(float4(position, 1.0f), mul(transform, viewProjection));
    vso.texCoord = texCoord;
    return vso;
}