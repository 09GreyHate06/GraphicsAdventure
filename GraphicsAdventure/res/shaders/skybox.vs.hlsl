#include "macros.hlsli"

struct VSOutput
{
    float3 pixelWorldSpacePos : PIXEL_WORLD_SPACE_POS;
    float4 position : SV_Position;
};

cbuffer EntityCBuf : REG_ENTITYCBUF
{
    float4x4 viewProj;
};

VSOutput main(float3 position : POSITION)
{
    VSOutput vso;
    vso.pixelWorldSpacePos = position;
    vso.position = mul(float4(position, 0.0f), viewProj);
    vso.position.z = vso.position.w;
    
    return vso;
}
