struct DirectionalLight
{
    float3 color;
    float ambientIntensity;
    float3 direction;
    float intensity;
    
    float4x4 lightSpace;
};

struct PointLight
{
    float3 color;
    float ambientIntensity;
    float3 position;
    float intensity;
    
    float nearZ;
    float farZ;
    float p0;
    float p1;
    
    float4x4 lightSpace;
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
    
    float4x4 lightSpace;
};