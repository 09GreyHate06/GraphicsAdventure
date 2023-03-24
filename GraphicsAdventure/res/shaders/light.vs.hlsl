struct VSInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 pixelWorldSpacePos : PIXEL_WORLD_SPACE_POS;
    float3 viewPos : VIEW_POS;
};

cbuffer SystemCBuf
{
    float4x4 viewProjection;
    float3 viewPos; // camera pos
    float p0;
};

cbuffer EntityCBuf
{
    float4x4 transform;
    float4x4 normalMatrix;
};

VSOutput main(VSInput input)
{
    VSOutput vso;
    float4 pixelWorldSpacePos = mul(float4(input.position, 1.0f), transform);
    vso.position = mul(pixelWorldSpacePos, viewProjection);
    vso.texCoord = input.texCoord;
    vso.normal = mul(input.normal, (float3x3)normalMatrix);
    vso.pixelWorldSpacePos = pixelWorldSpacePos;
    vso.viewPos = viewPos;
    
    return vso;
}