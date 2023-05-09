#include "macros.hlsli"
#include "light_source.hlsli"
#include "phong.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 pixelWorldSpacePos : PIXEL_WORLD_SPACE_POS;
    float3 viewPos : VIEW_POS;
};

struct PSOutput
{
    // accumulation of pre-multiplied color values
    float4 accumulation : SV_Target0;
    
    // pixel revealage
    float reveal : SV_Target1;
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

PSOutput main(VSOutput input)
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
    
    float4 color = float4((dirLightPhong + pointLightPhong + spotLightPhong) * textureMapCol.rgb, textureMapCol.a);
    
    PSOutput pso;
    // weight. the color-based factor
    // avoids color pollution from the edges of wispy clouds. the z-based
    // factor gives precedence to nearer surfaces
    // 3000 because 2^16 = 3000 (our render target format is RGBA16)
    float weight = clamp(pow(min(1.0f, color.a * 10.0f) + 0.01f, 3.0f) * 1e8f * pow(1.0f - input.position.z * 0.9f, 3.0f), 0.01f, 3000.0f);
    // store pixel color accumulation
    // BlendState(BLEND_ONE, BLEND_ONE)
    pso.accumulation = float4(color.rgb * color.a, color.a) * weight;
    // store pixel revealage threshold
    // BlendState(BLEND_ZERO, BLEND_INV_SRC_COLOR)
    pso.reveal = color.a;
    return pso;
}