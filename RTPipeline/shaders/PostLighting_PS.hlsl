Texture2D<float4> GPosition : register(t0);
Texture2D<float4> GAlbedoMetallic : register(t1);
Texture2D<float4> GNormalRoughness : register(t2);
Texture2D<float4> GExtra : register(t3);
Texture2D<float4> GSampleShadow : register(t4);
Texture2D<float4> GSampleReflect : register(t5);
Texture3D<float> A_tmp_data : register(t6);
struct PointLight
{
	float4 PositionWS;
	float4 Color;
	float Intensity;
	float Attenuation;
	float radius;
	float Padding;
}; // Total:                              16 * 3 = 48 bytes
ConstantBuffer<PointLight> pointLight : register(b0);
struct Camera
{
	float4 PositionWS;
	float4x4 InverseViewMatrix;
	float fov;
	float3 padding;
}; // Total:                              16 * 6 = 96 bytes
ConstantBuffer<Camera> CameraCB : register(b1);

float3 LinearToSRGB(float3 x)
{
    // This is exactly the sRGB curve
	return x < 0.0031308 ? 12.92 * x : 1.055 * pow(abs(x), 1.0 / 2.4) - 0.055;
}
float3 simpleToneMapping(float3 c)
{
	return float3(c.x / (c.x + 1.0f), c.y / (c.y + 1.0f), c.z / (c.z + 1.0f));
}

////////////////////////////////////////////////////// PBR workflow Cook-Torrance
float DoAttenuation(float attenuation, float distance)
{
	
	float atte = 1.0 / (1.0 + attenuation * distance * distance);
	return atte;
}
float DoSpotCone(float3 spotDir, float3 L, float spotAngle)
{
	float minCos = cos(spotAngle);
	float maxCos = (minCos + 1.0f) / 2.0f;
	float cosAngle = dot(spotDir, -L);
	return smoothstep(minCos, maxCos, cosAngle);
}
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
float3 DoPbrPointLight(PointLight light, float3 N, float3 V, float3 P, float3 albedo, float roughness, float metallic, float visibility)
{
	const float PI = 3.14159265359;
	float3 F0 = float3(0.04, 0.04, 0.04);
	F0 = lerp(F0, albedo, metallic);
	float3 L = normalize(light.PositionWS.xyz - P);
	float3 H = normalize(V + L);
	float distance = length(light.PositionWS.xyz - P);
	float attenuation = DoAttenuation(light.Attenuation, distance);
	float3 radiance = light.Color.xyz * attenuation;

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
	return visibility * (kD * albedo / PI + specular) * radiance * NdotL;
}
////////////////////////////////////////////////////// PBR workflow SchlickGGX

float4 main(float4 Position : SV_Position) : SV_TARGET0
{
	int2 texCoord = (int2) Position.xy;
	
	float hit = GPosition.Load(int3(texCoord.x, texCoord.y, 0)).w;
	float3 position = GPosition.Load(int3(texCoord.x, texCoord.y, 0)).xyz;
	float3 albedo = GAlbedoMetallic.Load(int3(texCoord.x, texCoord.y, 0)).xyz;
	float roughness = GNormalRoughness.Load(int3(texCoord.x, texCoord.y, 0)).w;
	float metallic = GAlbedoMetallic.Load(int3(texCoord.x, texCoord.y, 0)).w;
	float3 normal = GNormalRoughness.Load(int3(texCoord.x, texCoord.y, 0)).xyz;
	float3 emissive = GExtra.Load(int3(texCoord.x, texCoord.y, 0)).xyz;
	float gid = GExtra.Load(int3(texCoord.x, texCoord.y, 0)).w;
	float visibility = GSampleShadow.Load(int3(texCoord.x, texCoord.y, 0)).x;
	float3 reflectivity = GSampleReflect.Load(int3(texCoord.x, texCoord.y, 0)).xyz;
	if (hit == 0.0)
	{
		return float4(emissive, 1.0f);
	}
	
	float3 P = position;
	float3 V = normalize(CameraCB.PositionWS.xyz - position);
	float3 N = normalize(normal);
	
	// ambient
	float3 color0 = 0 * albedo;
	
	// direct
	float3 color1 =0 * DoPbrPointLight(pointLight, N, V, P, albedo, roughness, metallic, visibility);
	
	// indirect
	float3 color2 = reflectivity;
	
	// compact
	float3 color = LinearToSRGB(simpleToneMapping((color0 + color1 + color2)));
	return float4(color, 1);
	
	// test
	//return float4(visibility, visibility, visibility, 0);
}