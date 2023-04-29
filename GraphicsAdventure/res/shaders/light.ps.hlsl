struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 pixelWorldSpacePos : PIXEL_WORLD_SPACE_POS;
    float3 viewPos : VIEW_POS;
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

float4 main(VSOutput input) : SV_Target
{
    float3 normal = normalize(input.normal);
    
    float4 textureMapCol = textureMap.Sample(textureMapSampler, input.texCoord * mat.tiling) * mat.color;
    
    
    // screen door transparency
    // https://digitalrune.github.io/DigitalRune-Documentation/html/fa431d48-b457-4c70-a590-d44b0840ab1e.htm#Implementation
    float4x4 thresholdMatrix =
    {
        1.0 / 17.0, 9.0 / 17.0, 3.0 / 17.0, 11.0 / 17.0,
        13.0 / 17.0, 5.0 / 17.0, 15.0 / 17.0, 7.0 / 17.0,
        4.0 / 17.0, 12.0 / 17.0, 2.0 / 17.0, 10.0 / 17.0,
        16.0 / 17.0, 8.0 / 17.0, 14.0 / 17.0, 6.0 / 17.0
    };
    clip(textureMapCol.a - thresholdMatrix[input.position.x % 4][input.position.y % 4]);
    
    
    
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

    return float4((dirLightPhong + pointLightPhong + spotLightPhong) * textureMapCol.rgb, textureMapCol.a);
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