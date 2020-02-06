struct PixelShaderInput
{
    float4 PositionWS : POSITION;
    float3 NormalWS   : NORMAL;
    float2 TexCoord   : TEXCOORD;
};

struct PixelShaderOutput
{
	float4 GPosition : SV_Target0;
	float4 GAlbedo : SV_Target1;
	float4 GMetallic : SV_Target2;
	float4 GNormal : SV_Target3;
	float4 GRoughness : SV_Target4;
};

struct Material
{
	float4 Emissive; //----------------------------------- (16 byte boundary)
	float4 Ambient; //----------------------------------- (16 byte boundary)
	float4 Diffuse; //----------------------------------- (16 byte boundary)
	float4 Specular; //----------------------------------- (16 byte boundary)
    float   SpecularPower;
	float3 Padding; //----------------------------------- (16 byte boundary)
}; // Total:                              16 * 5 = 80 bytes



ConstantBuffer<Material> MaterialCB : register( b0, space1 );
Texture2D AlbedoTexture            : register( t0 );
Texture2D MetallicTexture           : register( t1 );
Texture2D NormalTexture            : register( t2 );
Texture2D RoughnessTexture      : register( t3 );
SamplerState AnisotropicSampler : register(s0);

float3 getFromNormalMapping(float4 sampledNormal, PixelShaderInput IN)
{
	float3 tangentNormal = sampledNormal.xyz * 2.0 - 1.0;  // [-1, 1]
	
	float3 deltaXYZ_1 = ddx(IN.PositionWS.xyz);
	float3 deltaXYZ_2 = ddy(IN.PositionWS.xyz);
	float2 deltaUV_1 = ddx(IN.TexCoord);
	float2 deltaUV_2 = ddy(IN.TexCoord);
	
	float3 N = normalize(IN.NormalWS);
	float3 T = normalize(deltaUV_2.y * deltaXYZ_1 - deltaUV_1.y * deltaXYZ_2);
	float3 B = normalize(cross(N, T));
	float3x3 TBN = float3x3(T, B, N);
	
	return normalize(mul( tangentNormal, TBN)); // [-1, 1]
}

PixelShaderOutput main(PixelShaderInput IN)
{
	PixelShaderOutput pso;
	
    float4 emissive = MaterialCB.Emissive;
    float4 ambient = MaterialCB.Ambient;
    float4 diffuse = MaterialCB.Diffuse ;
    float4 specular = MaterialCB.Specular;
	
	pso.GPosition = IN.PositionWS;
	pso.GAlbedo = AlbedoTexture.Sample(AnisotropicSampler, IN.TexCoord) * diffuse;
	pso.GMetallic = MetallicTexture.Sample(AnisotropicSampler, IN.TexCoord);
	pso.GNormal = float4(getFromNormalMapping(NormalTexture.Sample(AnisotropicSampler, IN.TexCoord), IN), 1.0f);
	pso.GRoughness = RoughnessTexture.Sample(AnisotropicSampler, IN.TexCoord);
	
	return pso;
}