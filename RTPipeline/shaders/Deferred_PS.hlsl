struct PixelShaderInput
{
    float4 PositionWS : POSITION;
    float3 NormalWS   : NORMAL;
    float2 TexCoord   : TEXCOORD;
};

struct PixelShaderOutput
{
	float4 GPosition : SV_Target0;
	float4 GAlbedoMetallic : SV_Target1;
	float4 GNormalRoughness : SV_Target2;
	float4 GExtra : SV_Target3;
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

struct PBRMaterial
{
	float Metallic;
	float Roughness;
	float2 Padding;
};// Totoal: 4*2 +8 = 16 bytes
struct ObjectIndex
{
	float index;
	float3 padding;
};
ConstantBuffer<Material> MaterialCB : register( b0, space1 );
ConstantBuffer<PBRMaterial> PBRMaterialCB : register(b1, space1);
ConstantBuffer<ObjectIndex> GameObjectIndex : register(b2, space1);
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
	
	float metallic = PBRMaterialCB.Metallic;
	float roughness = PBRMaterialCB.Roughness;
	
	//xyz =POSITION w=Hit
	pso.GPosition = IN.PositionWS; 
	//xyz=Albedo w=metallic
	pso.GAlbedoMetallic = float4(AlbedoTexture.Sample(AnisotropicSampler, IN.TexCoord).xyz * diffuse.xyz, MetallicTexture.Sample(AnisotropicSampler, IN.TexCoord).x * metallic);
	//xyz=Normal w=roughness
	pso.GNormalRoughness = float4(getFromNormalMapping(NormalTexture.Sample(AnisotropicSampler, IN.TexCoord), IN), RoughnessTexture.Sample(AnisotropicSampler, IN.TexCoord).x * roughness);
	//xyz=Emissive w=gid
	pso.GExtra = float4(MaterialCB.Emissive.xyz, GameObjectIndex.index);
	
	return pso;
}