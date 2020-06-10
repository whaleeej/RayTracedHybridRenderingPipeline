struct PixelShaderInput
{
    // Skybox texture coordinate
    float3 TexCoord : TEXCOORD;
};

TextureCube<float4> SkyboxTexture : register(t0);
SamplerState LinearClampSampler : register(s0);

float4 main(PixelShaderInput IN) : SV_Target
{
	return float4(SkyboxTexture.Sample(LinearClampSampler, IN.TexCoord).xyz, 0.0f);
}