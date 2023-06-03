
TextureCube tex : register(t0);
SamplerState sam : register(s0);

float4 main(float3 uvw : PIXEL_WORLD_SPACE_POS) : SV_Target
{
    return tex.Sample(sam, uvw);
}