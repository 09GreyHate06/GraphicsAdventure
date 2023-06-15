
float3x3 TBNOrthogonalized(float3 tangent, float3 normal)
{
    // gram schmidt orthogonalization 
    float3 t = normalize(tangent - dot(tangent, normal) * normal);
    float3 b = cross(normal, t);
    
    return float3x3(t, b, normal);
}

float3 NormalMap(float3 sampledNormal, float3 tangent, float3 bitangent, float3 normal)
{
    float3x3 tbn = float3x3(tangent, bitangent, normal);
    float3 normal_;
    normal_ = sampledNormal * 2.0f - 1.0f;
    normal_ = normalize(mul(normal_, tbn));
    
    return normal_;
}

float3 NormalMap(float3 sampledNormal, float3x3 tbn)
{
    float3 normal_;
    normal_ = sampledNormal * 2.0f - 1.0f;
    normal_ = normalize(mul(normal_, tbn));
    
    return normal_;
}

float2 ParallaxMap(float sampledHeight, float heightScale, float2 uv, float3 pixelToViewTS)
{
    float2 p = pixelToViewTS.xy * (sampledHeight * heightScale) / pixelToViewTS.z;
    return uv - p;
}