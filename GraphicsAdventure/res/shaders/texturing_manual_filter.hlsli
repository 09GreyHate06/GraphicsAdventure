
float4 NearestWrap(Texture2D tex, float2 texCoord)
{
    int2 texSize;
    tex.GetDimensions(texSize.x, texSize.y);
    
    // center
    float2 center = round(fmod(texCoord * (texSize - int2(1, 1)), texSize));

    return tex.Load(int3(center.xy, 0));
}

float4 BilinearWrap(Texture2D tex, float2 texCoord)
{
    int2 texSize;
    tex.GetDimensions(texSize.x, texSize.y);
    
    // center
    float2 center = fmod(texCoord * (texSize - int2(1, 1)), texSize);
    
    // drop the center fraction 
    float2 temp = center - float2(0.5f, -0.5f);
    
    float2 topLeft;
    float2 fractionalPart = modf(temp, topLeft);
    
    float2 topRight = float2(topLeft.x + 1.0f, topLeft.y);
    float2 bottomLeft = float2(topLeft.x, topLeft.y + 1.0f);
    float2 bottomRight = topLeft + 1.0f;
    
    float4 topTexelLerp = lerp(tex.Load(int3(topLeft.xy, 0)), tex.Load(int3(topRight.xy, 0)), fractionalPart.x);
    float4 botTexelLerp = lerp(tex.Load(int3(bottomLeft.xy, 0)), tex.Load(int3(bottomRight.xy, 0)), fractionalPart.x);

    return lerp(topTexelLerp, botTexelLerp, fractionalPart.y);
}

float2 Smoothstep(float2 x)
{
    return (x * x) * (3.0f - 2.0f * x);
}

float2 Quintic(float2 x)
{
    return (x * x * x) * ((6 * x * x) - 15.0f * x + 10.0f);
}

float2 GetBicubicTexCoordWrap(int2 texSize, float2 texCoord)
{
    // center
    float2 center = fmod(texCoord * (texSize - int2(1, 1)), texSize) + 0.5f;
    
    float2 intPart;
    float2 fractionalPart = modf(center, intPart);
    
    float2 tuv = (Quintic(fractionalPart) - 0.5f) + intPart;
    tuv.x /= float(texSize.x - 1.0f);
    tuv.y /= float(texSize.y - 1.0f);
    
    return tuv;
}

float4 BicubicWrap(Texture2D tex, float2 texCoord)
{
    int2 texSize;
    tex.GetDimensions(texSize.x, texSize.y);
    float2 tuv = GetBicubicTexCoordWrap(texSize, texCoord);
    
    
    float2 topLeft;
    float2 fractionalPart = modf(tuv, topLeft);
    
    float2 topRight = float2(topLeft.x + 1.0f, topLeft.y);
    float2 bottomLeft = float2(topLeft.x, topLeft.y + 1.0f);
    float2 bottomRight = topLeft + 1.0f;
    
    float4 topTexelLerp = lerp(tex.Load(int3(topLeft.xy, 0)), tex.Load(int3(topRight.xy, 0)), fractionalPart.x);
    float4 botTexelLerp = lerp(tex.Load(int3(bottomLeft.xy, 0)), tex.Load(int3(bottomRight.xy, 0)), fractionalPart.x);

    return lerp(topTexelLerp, botTexelLerp, fractionalPart.y);
}

