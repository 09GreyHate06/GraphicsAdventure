
float3x3 TBNOrthogonalized(float3 tangent, float3 normal)
{
    // gram schmidt orthogonalization 
    float3 t = normalize(tangent - dot(tangent, normal) * normal);
    float3 b = cross(normal, t);
    
    return float3x3(t, b, normal);
}

float3 NormalMapping(float3 sampledNormal, float3 tangent, float3 bitangent, float3 normal)
{
    float3x3 tbn = float3x3(tangent, bitangent, normal);
    float3 normal_;
    normal_ = sampledNormal * 2.0f - 1.0f;
    normal_ = normalize(mul(normal_, tbn));
    
    return normal_;
}

float3 NormalMapping(float3 sampledNormal, float3x3 tbn)
{
    float3 normal_;
    normal_ = sampledNormal * 2.0f - 1.0f;
    normal_ = normalize(mul(normal_, tbn));
    
    return normal_;
}

float2 ParallaxMapping(float sampledDepth, float heightScale, float2 uv, float3 pixelToViewTS)
{
    float2 p = pixelToViewTS.xy * (sampledDepth * heightScale) / pixelToViewTS.z;
    return uv - p;
}

float2 SteepParallaxMapping(Texture2D<float> depthMap, SamplerState sam, float depthScale, float2 uv, float3 pixelToViewTS)
{
    const float minLayers = 8.0f;
    const float maxLayers = 32.0f;

    // larger layer if viewing from steep angle    
    float numLayers = lerp(maxLayers, minLayers, max(dot(float3(0.0f, 0.0f, 1.0f), pixelToViewTS), 0.0f));
    
    float layerDepth = 1.0f / numLayers;
    float curLayerDepth = 0.0f;
    
    float2 p = pixelToViewTS.xy * depthScale;
    float2 deltaUV = p / (pixelToViewTS.z * numLayers);
    
    float2 curUV = uv;
    float curDepthMapValue = depthMap.Sample(sam, curUV);

    [unroll(32)]
    while (curDepthMapValue > curLayerDepth)
    {
        curUV -= deltaUV;
        curDepthMapValue = depthMap.Sample(sam, curUV);
        curLayerDepth += layerDepth;
    }
    
    return curUV;
}

float2 ParallaxOcclusionMapping(Texture2D<float> depthMap, SamplerState sam, float depthScale, float2 uv, float3 pixelToViewTS)
{
    const float minLayers = 8.0f;
    const float maxLayers = 32.0f;

    // larger layer if viewing from steep angle    
    float numLayers = lerp(maxLayers, minLayers, max(dot(float3(0.0f, 0.0f, 1.0f), pixelToViewTS), 0.0f));
    
    float layerDepth = 1.0f / numLayers;
    float curLayerDepth = 0.0f;
    
    float2 p = pixelToViewTS.xy * depthScale;
    float2 deltaUV = p / (pixelToViewTS.z * numLayers);;
    
    float2 curUV = uv;
    float curDepthMapValue = depthMap.Sample(sam, curUV);

    [unroll(32)]
    while (curDepthMapValue > curLayerDepth)
    {
        curUV -= deltaUV;
        curDepthMapValue = depthMap.Sample(sam, curUV);
        curLayerDepth += layerDepth;
    }
    
    float2 prevUV = curUV + deltaUV;
    
    float afterHeight = curLayerDepth - curDepthMapValue;
    float beforeHeight = curLayerDepth - layerDepth - depthMap.Sample(sam, prevUV);
    
    float weight = afterHeight / (afterHeight - beforeHeight);
    float2 finalUV = lerp(curUV, prevUV, weight);
     
    return finalUV;
}