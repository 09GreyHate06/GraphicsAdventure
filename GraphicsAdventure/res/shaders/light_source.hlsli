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