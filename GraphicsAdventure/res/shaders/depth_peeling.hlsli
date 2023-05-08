
Texture2D prevDepthMap : register(t10);

void DepthPeel(int2 pixelScreenSpacePos, float curDepth)
{
    //uint2 screenSize;
    //prevDepthMap.GetDimensions(screenSize.x, screenSize.y);
    //float2 texCoord = float2(0.5f * (pixelClipSpacePosition.x + 1.0f), -0.5f * (pixelClipSpacePosition.y - 1.0f));
    //
    //int2 texCoordZeroBased = int2(
    //    min(screenSize.x - 1, texCoord.x * screenSize.x),
    //    min(screenSize.y - 1, texCoord.y * screenSize.y)
    //);
    
    //float curDepth = pixelClipSpacePosition.z;
    
    float prevDepth = prevDepthMap.Load(int3(pixelScreenSpacePos, 0)).x;
    clip(curDepth - prevDepth - 0.00001f);
}