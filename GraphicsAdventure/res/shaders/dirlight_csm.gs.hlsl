#include "macros.hlsli"

struct GSOutput
{
    float4 position : SV_Position;
    uint slice : SV_RenderTargetArrayIndex;
};

cbuffer SystemCBuf : REG_SYSTEMCBUF
{
    float4x4 lightSpaceMatrices[3];
};

[maxvertexcount(3)] 
[instance(5)] // num cascade
void main(triangle float4 pixelWorldPos[3] : SV_Position, inout TriangleStream<GSOutput> outputStream, in uint id : SV_GSInstanceID)
{
    for (int i = 0; i < 3; i++)
    {
        GSOutput gso;
        gso.position = mul(pixelWorldPos[i], lightSpaceMatrices[id]);
        gso.slice = id;
        outputStream.Append(gso);
    }
    outputStream.RestartStrip();
}