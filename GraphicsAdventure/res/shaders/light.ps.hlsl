#include "macros.hlsli"
#include "light_source.hlsli"
#include "phong.hlsli"
#include "texturing_manual_filter.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD;
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
        float p0;
    } mat;
};

Texture2D textureMap : register(t0);
SamplerState textureMapSampler : register(s0);

float4 main(VSOutput input) : SV_Target
{
    float3 normal = normalize(input.normal);
    
    float4 textureMapCol = textureMap.Sample(textureMapSampler, input.texCoord * mat.tiling) * mat.color;
    
    float3 dirLightPhong = float3(0.0f, 0.0f, 0.0f);
    float3 pointLightPhong = float3(0.0f, 0.0f, 0.0f);
    float3 spotLightPhong = float3(0.0f, 0.0f, 0.0f);
    
    for (uint i = 0; i < activeDirLights; i++)
    {
        DirectionalLight light = dirLights[i];
        
        float3 pixelToLight = normalize(-light.direction);
        float3 pixelToView = normalize(input.viewPos - input.pixelWorldSpacePos);
        dirLightPhong += Phong(light.color, pixelToLight, pixelToView, normal, light.ambientIntensity, light.intensity, mat.shininess);
    }
    
    for (uint i = 0; i < activePointLights; i++)
    {
        PointLight light = pointLights[i];
        
        float3 pixelToLight = normalize(light.position - input.pixelWorldSpacePos);
        float3 pixelToView = normalize(input.viewPos - input.pixelWorldSpacePos);
        
        float att = Attenuation(length(light.position - input.pixelWorldSpacePos));
        pointLightPhong += Phong(light.color, pixelToLight, pixelToView, normal, light.ambientIntensity, light.intensity, mat.shininess) * att;
    }
    
    for (uint i = 0; i < activeSpotLights; i++)
    {
        SpotLight light = spotLights[i];
        
        float3 pixelToLight = normalize(light.position - input.pixelWorldSpacePos);
        float3 pixelToView = normalize(input.viewPos - input.pixelWorldSpacePos);
        
        float att = Attenuation(length(light.position - input.pixelWorldSpacePos));
        
        float cosTheta = dot(pixelToLight, normalize(-light.direction));
        float epsilon = light.innerCutOffCosAngle - light.outerCutOffCosAngle;
        float intensity = clamp((cosTheta - light.outerCutOffCosAngle) / epsilon, 0.0f, 1.0f);
        
        spotLightPhong += Phong(light.color, pixelToLight, pixelToView, normal, light.ambientIntensity, light.intensity, mat.shininess) * att * intensity;
    }
    
    return float4((dirLightPhong + pointLightPhong + spotLightPhong) * textureMapCol.rgb, textureMapCol.a);
}