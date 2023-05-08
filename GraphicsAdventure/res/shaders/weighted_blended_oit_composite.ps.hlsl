
Texture2D accumulationMap : register(t0);
Texture2D revealMap : register(t1);

#define EPSILON 0.0001

// get the max value between three values
float Max3(float3 v)
{
    return max(max(v.x, v.y), v.z);
}

float4 main(float4 position : SV_Position) : SV_TARGET
{
    // BlendState(BLEND_SRC_ALPHA, BLEND_INV_SRC_ALPHA) "OVER operator"
    // DepthFuncComp = Always
    
	// pixel coordination
    int2 coords = floor(position.xy);
    
    // pixel revealage
    float revealage = revealMap.Load(int3(coords, 0)).x;
    
    // save the blending and color texture fetch cost if there is not a transparent pixel
    clip(1.0f - revealage - EPSILON);
        
    // pixel color
    float4 accumulation = accumulationMap.Load(int3(coords, 0));
    
    // suppress overflow
    if (isinf(Max3(abs(accumulation.rgb)))) 
        accumulation.rgb = accumulation.aaa;

    // prevent floating point precision bug
    float3 averageCol = accumulation.rgb / max(accumulation.a, EPSILON);
    
    // blend pixels
    return float4(averageCol, 1.0f - revealage);
}