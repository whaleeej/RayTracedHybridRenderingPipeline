Texture2D<float4> gPosition : register(t0); //srv
Texture2D<float4> gAlbedoMetallic : register(t1); //srv
Texture2D<float4> gNormalRoughness : register(t2); //srv
Texture2D<float4> gExtra : register(t3); //srv
Texture2D<float4> gShadowSecondaryDir : register(t4);
Texture2D<float4> gRadiancePdf : register(t5);

struct Camera
{
	float4 PositionWS;
	float4x4 InverseViewMatrix;
	float fov;
	float3 padding;
};
ConstantBuffer<Camera> CameraCB : register(b0);


float DistributionGGX(float3 N, float3 H, float roughness)
{
	const float PI = 3.14159265359;
	float a = roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return nom / denom; // prevent divide by zero for roughness=0.0 and NdotH=1.0
}
float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;
}
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);
	return ggx1 * ggx2;
}
float3 fresnelSchlick(float cosTheta, float3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
float3 DoPbrPointLight(float3 radiance, float3 L, float3 N, float3 V, float3 P, float3 albedo, float roughness, float metallic)
{
	const float PI = 3.14159265359;
	float3 F0 = float3(0.04, 0.04, 0.04);
	F0 = lerp(F0, albedo, metallic);
	float3 H = normalize(V + L);

    // Cook-Torrance BRDF
	float NDF = DistributionGGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness);
	float3 F = fresnelSchlick(clamp(dot(N, V), 0.0, 1.0), F0);
	
	float3 nominator = NDF * G * F;
	float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
	float3 specular = nominator / max(denominator, 0.001); // prevent divide by zero for NdotV=0.0 or NdotL=0.0
        
     // kS is equal to Fresnel
	float3 kS = F;
    // for energy conservation, the diffuse and specular light can't
    // be above 1.0 (unless the surface emits light); to preserve this
    // relationship the diffuse component (kD) should equal 1.0 - kS.
	float3 kD = float3(1.0, 1.0, 1.0) - kS;
    // multiply kD by the inverse metalness such that only non-metals 
    // have diffuse lighting, or a linear blend if partly metal (pure metals
    // have no diffuse light).
	kD *= 1.0 - metallic;

    // scale light by NdotL
	float NdotL = max(dot(N, L), 0.0);

    // add to outgoing radiance Lo
	return (kD * albedo / PI + specular) * radiance * NdotL;
}

// from paper svgf paper 2017
bool isResampleValid(float2 res, float2 curr_coord, float2 sample_coord)
{
    // reject if the pixel is outside the screen
	if (sample_coord.x < 0 || sample_coord.x >= res.x || sample_coord.y < 0 || sample_coord.y >= res.y)
		return false;
    // reject the pixel is not hit / hit another geometry
	if (gPosition.Load(int3(sample_coord.x, sample_coord.y, 0)).w == 0.0 ||
		(gExtra.Load(int3(curr_coord.x, curr_coord.y, 0)).w - gExtra.Load(int3(sample_coord.x, sample_coord.y, 0)).w) >= 0.1)
		return false;

    // reject if the normal deviation is not acceptable
	if (distance(gNormalRoughness.Load(int3(curr_coord.x, curr_coord.y, 0)).xyz, gNormalRoughness.Load(int3(sample_coord.x, sample_coord.y, 0)).xyz) > 0.1)
		return false;
	
	if (distance(gNormalRoughness.Load(int3(curr_coord.x, curr_coord.y, 0)).w, gNormalRoughness.Load(int3(sample_coord.x, sample_coord.y, 0)).w) > 0.1)
		return false;
	return true;
}

float3 spatialResample(float x, float y, float2 res)
{
	 // 3x3 Gaussian kernel
	float gaussian[9] =
	{
		1.0 / 16.0, 1.0 / 8.0, 1.0 / 16.0,
        1.0 / 8.0, 1.0 / 4.0, 1.0 / 8.0,
        1.0 / 16.0, 1.0 / 8.0, 1.0 / 16.0
	};
	
	float3 P = gPosition.Load(int3(x, y, 0)).xyz;
	float3 V = normalize(CameraCB.PositionWS.xyz - P);
	float3 N = gNormalRoughness.Load(int3(x, y, 0)).xyz;
	
	float3 albedo     = gAlbedoMetallic.Load(int3(x, y, 0)).xyz;
	float roughness = gNormalRoughness.Load(int3(x, y, 0)).w;
	float metallic     = gAlbedoMetallic.Load(int3(x, y, 0)).w;
	
	int radius = lerp(1, 3, roughness * roughness);
	float3 result_sum = 0;
	float weight_sum = 0;
	int gaus_count = 0;
	for (int yy = -radius; yy <= radius; yy++)
	{
		for (int xx = -radius; xx <= radius; xx++)
		{
			float2 sample_loc = float2(x, y) + float2(xx, yy);
			if (isResampleValid(res, float2(x, y), sample_loc))
			{
				float3 radiance = gRadiancePdf.Load(int3(sample_loc, 0)).xyz;
				float pdf = gRadiancePdf.Load(int3(sample_loc, 0)).w;
				float3 L = gShadowSecondaryDir.Load(int3(sample_loc, 0)).yzw;
				
				//float gaussian_operator = gaussian[gaus_count];
				
				result_sum += 1 * DoPbrPointLight(radiance, L, N, V, P, albedo, roughness, metallic) / max(0.6, min(pdf, 2.0));
				weight_sum += 1;
				
				gaus_count++;
			}
		}
	}
	weight_sum = max(weight_sum, 1.0);
	return result_sum/weight_sum;
}


float4 main(float4 Position : SV_Position) : SV_Target0
{
	float col_alpha = 0.1;
	float mom_alpha = 0.9;
	
	int2 texCoord = (int2) Position.xy;
	float resx;
	float resy;
	float mipLevels;
	gPosition.GetDimensions(0, resx, resy, mipLevels);
	
	float3 resampled = spatialResample(texCoord.x, texCoord.y, float2(resx, resy));
	return float4(resampled, 1.0);
}