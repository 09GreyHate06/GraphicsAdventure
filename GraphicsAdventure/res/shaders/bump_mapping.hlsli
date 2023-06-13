
float3 NormalMapOrthogonalized(float3 sampledNormal, float3 tangent, float3 normal)
{
    // gram schmidt orthogonalization 
    float3 t = normalize(tangent - dot(tangent, normal) * normal);
    float3 b = cross(normal, t);
       
    // texture space to tangent space
    float3x3 tbn = float3x3(t, b, normal);
    float3 normal_;
    normal_ = sampledNormal * 2.0f - 1.0f;
    normal_ = normalize(mul(normal_, tbn));
    
    return normal_;
}

float3 NormalMap(float3 sampledNormal, float3 tangent, float3 bitangent, float3 normal)
{
    // texture space to tangent space
    float3x3 tbn = float3x3(tangent, bitangent, normal);
    float3 normal_;
    normal_ = sampledNormal * 2.0f - 1.0f;
    normal_ = normalize(mul(normal_, tbn));
    
    return normal_;
}