#include "macros.hlsli"
#include "light_source.hlsli"
#include "phong.hlsli"
#include "texturing_manual_filter.hlsli"
#include "bump_mapping.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    float3 normal : NORMAL;
    float3 pixelWorldSpacePos : PIXEL_WORLD_SPACE_POS;
    float3 viewPos : VIEW_POS;
};

static const uint s_maxLights = 32;

cbuffer SystemCBuf : REG_SYSTEMCBUF
{
    DirectionalLight dirLights[s_maxLights];
    PointLight pointLights[s_maxLights];
    SpotLight spotLights[s_maxLights];
    
    uint activeDirLights = 0;
    uint activePointLights = 0;
    uint activeSpotLights = 0;
    uint p0;
};

cbuffer EntityCBuf : REG_ENTITYCBUF
{
    struct
    {
        float4 color;
        float2 tiling;
        float shininess;
        bool enableNormalMap;
        bool enableHeightMap;
        float heightMapScale;
        int p0;
        int p1;
    } mat;
};

Texture2D<float4> diffuseMap : register(t0);
Texture2D<float3> normalMap : register(t1);
Texture2D<float> heightMap : register(t2);
SamplerState mapSampler : register(s0);

float4 main(VSOutput input) : SV_Target
{
    float3 tangent = normalize(input.tangent);
    float3 bitangent = normalize(input.bitangent);
    float3 normal = normalize(input.normal);
    float3 pixelToView = normalize(input.viewPos - input.pixelWorldSpacePos);
    float2 uv = input.uv * mat.tiling;
    
    if (mat.enableNormalMap)
    {
        float3x3 tbn = TBNOrthogonalized(tangent, normal);
        if(mat.enableHeightMap)
            uv = ParallaxMap(heightMap.Sample(mapSampler, uv), mat.heightMapScale, uv, mul(pixelToView, transpose(tbn)));
        
        if (uv.x > mat.tiling.x || uv.y > mat.tiling.y || uv.x < 0.0 || uv.y < 0.0)
            clip(-1);
        
        normal = NormalMap(normalMap.Sample(mapSampler, uv), tbn);
        //normal = NormalMap(normalMap.Sample(mapSampler, uv).xyz, tangent, bitangent, normal);
    }
    
    float4 textureMapCol = diffuseMap.Sample(mapSampler, uv) * mat.color;
    
    clip(textureMapCol.a - EPSILON);
    
    float3 dirLightPhong = float3(0.0f, 0.0f, 0.0f);
    float3 pointLightPhong = float3(0.0f, 0.0f, 0.0f);
    float3 spotLightPhong = float3(0.0f, 0.0f, 0.0f);
    
    for (uint i = 0; i < activeDirLights; i++)
    {
        DirectionalLight light = dirLights[i];
        
        float3 pixelToLight = normalize(-light.direction);
        dirLightPhong += Phong(light.color, pixelToLight, pixelToView, normal, light.ambientIntensity, light.intensity, mat.shininess);
    }
    
    for (uint i = 0; i < activePointLights; i++)
    {
        PointLight light = pointLights[i];
        
        float3 pixelToLight = normalize(light.position - input.pixelWorldSpacePos);
        
        float att = Attenuation(length(light.position - input.pixelWorldSpacePos));
        pointLightPhong += Phong(light.color, pixelToLight, pixelToView, normal, light.ambientIntensity, light.intensity, mat.shininess) * att;
    }
    
    for (uint i = 0; i < activeSpotLights; i++)
    {
        SpotLight light = spotLights[i];
        
        float3 pixelToLight = normalize(light.position - input.pixelWorldSpacePos);
        
        float att = Attenuation(length(light.position - input.pixelWorldSpacePos));
        
        float cosTheta = dot(pixelToLight, normalize(-light.direction));
        float epsilon = light.innerCutOffCosAngle - light.outerCutOffCosAngle;
        float intensity = clamp((cosTheta - light.outerCutOffCosAngle) / epsilon, 0.0f, 1.0f);
        
        spotLightPhong += Phong(light.color, pixelToLight, pixelToView, normal, light.ambientIntensity, light.intensity, mat.shininess) * att * intensity;
    }
    
    return float4((dirLightPhong + pointLightPhong + spotLightPhong) * textureMapCol.rgb, textureMapCol.a);
}