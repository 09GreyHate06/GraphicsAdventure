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
};

struct SpotLight
{
    float3 color;
    float ambientIntensity;
    float3 direction;
    float intensity;
    
    float3 position;
    float innerCutOffCosAngle;
    
    float outerCutOffCosAngle;
    float p0;
    float p1;
    float p2;
};