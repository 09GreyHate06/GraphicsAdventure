#include "macros.hlsli"
#include "phong.hlsli"
#include "texturing_manual_filter.hlsli"
#include "bump_mapping.hlsli"
#include "shadow_mapping.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    float3 normal : NORMAL;
    float3 pixelWorldSpacePos : PIXEL_WORLD_SPACE_POS;
    float pixelViewSpaceDepth : PIXEL_VIEW_SPACE_POS_DEPTH; // for csm
    float3 viewPos : VIEW_POS;
};

static const uint s_numCascades = 5;

cbuffer SystemCBuf : REG_SYSTEMCBUF
{
    struct DirectionalLight
    {
        float3 color;
        float ambientIntensity;
        float3 direction;
        float intensity;
    
        float4x4 lightSpaces[s_numCascades];
        float4 cascadeFarZDist[s_numCascades]; // arranged from lowest to highest
    } dirLight;
};

cbuffer EntityCBuf : REG_ENTITYCBUF
{
    struct
    {
        float4 color;
        float2 tiling;
        float shininess;
        bool enableNormalMapping;
        bool enableParallaxMapping;
        float depthMapScale;
        int p0;
        int p1;
    } mat;
    
    bool receiveShadows;
    float p2;
    float p3;
    float p4;
};

Texture2D<float4> diffuseMap : register(t0);
Texture2D<float3> normalMap : register(t1);
Texture2D<float> depthMap : register(t2);
SamplerState samplerState : register(s0);

Texture2DArray<float> dirLightShadowMaps : register(t3);
SamplerState dirLightShadowMapsSampler : register(s1);

float4 main(VSOutput input) : SV_Target
{
    float3 tangent = normalize(input.tangent);
    float3 bitangent = normalize(input.bitangent);
    float3 normal = normalize(input.normal);
    float3 pixelToView = normalize(input.viewPos - input.pixelWorldSpacePos);
    float2 uv = input.uv * mat.tiling;

    if (mat.enableNormalMapping)
    {
        float3x3 tbn = TBNOrthogonalized(tangent, normal);
        if (mat.enableParallaxMapping)
            uv = ParallaxOcclusionMapping(depthMap, samplerState, mat.depthMapScale, uv, mul(pixelToView, transpose(tbn)));
        
        if (uv.x > mat.tiling.x || uv.y > mat.tiling.y || uv.x < 0.0 || uv.y < 0.0)
            clip(-1);
        
        normal = NormalMapping(normalMap.Sample(samplerState, uv), tbn);
        //normal = NormalMap(normalMap.Sample(mapSampler, uv).xyz, tangent, bitangent, normal);
    }
    
    float4 textureMapCol = diffuseMap.Sample(samplerState, uv) * mat.color;
    
    clip(textureMapCol.a - EPSILON);
    
    float3 pixelToLight = normalize(-dirLight.direction);
    int csmLayer = s_numCascades - 1;
    for (int i = 0; i < s_numCascades; i++)
    {
        if (input.pixelViewSpaceDepth < dirLight.cascadeFarZDist[i].x)
        {
            csmLayer = i;
            break;
        }
    }
    
    float shadow = receiveShadows ? ShadowMapping(dirLightShadowMaps, csmLayer, dirLightShadowMapsSampler, mul(float4(input.pixelWorldSpacePos, 1.0f), dirLight.lightSpaces[csmLayer])) : 1.0f;
    float3 dirLightPhong = Phong(dirLight.color, pixelToLight, pixelToView, normal, dirLight.ambientIntensity, dirLight.intensity, mat.shininess, shadow);
    
    // debug
    float4 res = float4(dirLightPhong * textureMapCol.rgb, textureMapCol.a);
    //if(csmLayer == 0) res *= float4(1.0f, 0.3f, 0.3f, 1.0f);
    //if(csmLayer == 1) res *= float4(0.3f, 1.0f, 0.3f, 1.0f);
    //if(csmLayer == 2) res *= float4(0.3f, 0.3f, 1.0f, 1.0f);
    
    return res;
}