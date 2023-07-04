
float ShadowMapping(Texture2DArray<float> smap, int index, SamplerState smapSampler, float4 pixelLightSpace)
{
    float3 projCoord = pixelLightSpace.xyz / pixelLightSpace.w;
    
    if(projCoord.z > 1.0f)
        return 1.0f;
    
    projCoord.x = projCoord.x * 0.5f + 0.5f;
    projCoord.y = -projCoord.y * 0.5f + 0.5f;
    
    float occluderDepth = smap.Sample(smapSampler, float3(projCoord.xy, (float)index));
    
    if(projCoord.z < occluderDepth)
        return 1.0f;
    
    return 0.0f;
}

float OmniDirShadowMapping(TextureCubeArray<float> smap, int index, SamplerState smapSampler, float3 pixelLightSpace, float nearZ, float farZ)
{
    // Z vector mult in projection matrix
    float zMult = farZ / (farZ - nearZ);
    float zAdd = -nearZ * farZ / (farZ - nearZ);
    
    // the largest component is the depth
    float3 m = abs(pixelLightSpace);
    // get the length in the dominant axis
    // (this correlates with shadow map face and derives comparison depth)
    float lightSpaceDepth = max(m.x, max(m.y, m.z));
    
    // converting from distance in shadow light space to projected depth
    float projDepth = (lightSpaceDepth * zMult + zAdd) / lightSpaceDepth;
    
    if (projDepth < smap.Sample(smapSampler, float4(pixelLightSpace, (float) index)))
        return 1.0f;

    return 0.0f;
}