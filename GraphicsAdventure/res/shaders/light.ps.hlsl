#include "depth_peeling.hlsli"

struct VSOutput
{
    float4 position : SV_Position;
    float4 pPosition : TEXCOORD1; // projected pos
    float2 texCoord : TEXCOORD2;
    float3 normal : NORMAL;
    float3 pixelWorldSpacePos : PIXEL_WORLD_SPACE_POS;
    float3 viewPos : VIEW_POS;
};


struct PSOutput
{
    float4 color : SV_Target;
    float depth : SV_Depth;
};

struct DirectionalLight
{
    float3 color;
    float ambientIntensity;
    float3 direction;
    float intensity;
};

struct PointLight
{
    float3 color;
    float ambientIntensity;
    float3 position;
    float intensity;
    
    float attConstant;
    float attLinear;
    float attQuadratic;
    float p0;
};

struct SpotLight
{
    float3 color;
    float ambientIntensity;
    float3 direction;
    float intensity;
    
    float3 position;
    float attConstant;
    
    float attLinear;
    float attQuadratic;
    float innerCutOffCosAngle;
    float outerCutOffCosAngle;
};

struct Material
{
    float4 color;
    float2 tiling;
    float shininess;
    float p0;
};

static const uint s_maxLights = 32;

cbuffer SystemCBuf : register(b0)
{
    DirectionalLight dirLights[s_maxLights];
    PointLight pointLights[s_maxLights];
    SpotLight spotLights[s_maxLights];
    
    uint activeDirLights = 0;
    uint activePointLights = 0;
    uint activeSpotLights = 0;
    uint p0;
};

cbuffer EntityCBuf : register(b1)
{
    Material mat;
};

Texture2D textureMap : register(t0);
SamplerState textureMapSampler : register(s0);

float3 Phong(float3 lightCol, float3 pixelToLight, float3 pixelToView, float3 normal, float ambientIntensity, float lightIntensity);
float Attenuation(float c, float l, float q, float d);

PSOutput main(VSOutput input)
{
    float3 pixelClipSpacePos = input.pPosition.xyz / input.pPosition.w;
    
    DepthPeel(pixelClipSpacePos);
    
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
        dirLightPhong += Phong(light.color, pixelToLight, pixelToView, normal, light.ambientIntensity, light.intensity);
    }
    
    for (uint i = 0; i < activePointLights; i++)
    {
        PointLight light = pointLights[i];
        
        float3 pixelToLight = normalize(light.position - input.pixelWorldSpacePos);
        float3 pixelToView = normalize(input.viewPos - input.pixelWorldSpacePos);
        
        float att = Attenuation(light.attConstant, light.attLinear, light.attQuadratic, length(light.position - input.pixelWorldSpacePos));
        pointLightPhong += Phong(light.color, pixelToLight, pixelToView, normal, light.ambientIntensity, light.intensity) * att;
    }
    
    for (uint i = 0; i < activeSpotLights; i++)
    {
        SpotLight light = spotLights[i];
        
        float3 pixelToLight = normalize(light.position - input.pixelWorldSpacePos);
        float3 pixelToView = normalize(input.viewPos - input.pixelWorldSpacePos);
        
        float att = Attenuation(light.attConstant, light.attLinear, light.attQuadratic, length(light.position - input.pixelWorldSpacePos));
        
        float cosTheta = dot(pixelToLight, normalize(-light.direction));
        float epsilon = light.innerCutOffCosAngle - light.outerCutOffCosAngle;
        float intensity = clamp((cosTheta - light.outerCutOffCosAngle) / epsilon, 0.0f, 1.0f);
        
        spotLightPhong += Phong(light.color, pixelToLight, pixelToView, normal, light.ambientIntensity, light.intensity) * att * intensity;
    }

    PSOutput pso;
    pso.color = float4((dirLightPhong + pointLightPhong + spotLightPhong) * textureMapCol.rgb, textureMapCol.a);
    pso.depth = pixelClipSpacePos.z;
    return pso;
}

float3 Phong(float3 lightCol, float3 pixelToLight, float3 pixelToView, float3 normal, float ambientIntensity, float lightIntensity)
{
    float3 ambient = ambientIntensity * lightCol;
    
    float diffuseFactor = max(dot(normal, pixelToLight), 0.0f);
    float3 diffuse = lightIntensity * lightCol * diffuseFactor;
    
    float3 specular = float3(0.0f, 0.0f, 0.0f);
    if (diffuseFactor >= 0.0001f)
    {
        float3 halfwayDir = normalize(pixelToView + pixelToLight);
        float specularFactor = pow(max(dot(normal, halfwayDir), 0.0f), mat.shininess);
        specular = lightIntensity * lightCol * specularFactor;
    }

    
    return ambient + diffuse + specular;
}

float Attenuation(float c, float l, float q, float d)
{
    return 1.0f / (c + l * d + q * d * d);

}