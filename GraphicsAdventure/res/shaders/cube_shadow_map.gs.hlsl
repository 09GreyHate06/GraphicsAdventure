#include "macros.hlsli"

struct GSOutput
{
    float4 position : SV_Position;
    uint slice : SV_RenderTargetArrayIndex;
};

cbuffer SystemCBuf : REG_SYSTEMCBUF
{
    float4x4 lightSpaceMatrices[6];
};

[maxvertexcount(18)]
void main(triangle float4 pixelWorldPos[3] : SV_Position, inout TriangleStream<GSOutput> outputStream)
{
    for (uint face = 0; face < 6; face++)
    {
        GSOutput gso;
        gso.slice = face;
        for (int i = 0; i < 3; i++)
        {
            gso.position = mul(pixelWorldPos[i], lightSpaceMatrices[face]);
            outputStream.Append(gso);
        }
        outputStream.RestartStrip();
    }
}