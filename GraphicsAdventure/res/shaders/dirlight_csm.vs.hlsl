#include "macros.hlsli"

cbuffer EntityCBuf : REG_ENTITYCBUF
{
    float4x4 transform;
};

float4 main(float3 position : POSITION) : SV_Position
{
    return mul(float4(position, 1.0f), transform);
}