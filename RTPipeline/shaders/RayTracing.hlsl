RWTexture2D<float4> gOutput : register(u0);

RaytracingAccelerationStructure gRtScene : register(t0);
Texture2D<float4> GPosition : register(t1);
Texture2D<float4> GAlbedo : register(t2);
Texture2D<float4> GMetallic : register(t3);
Texture2D<float4> GNormal : register(t4);
Texture2D<float4> GRoughness : register(t5);
struct PointLight
{
	float4 PositionWS;
	float4 Color;
	float Intensity;
	float Attenuation;
	float2 Padding; // Pad to 16 bytes
}; // Total:                              16 * 3 = 48 bytes
StructuredBuffer<PointLight> PointLights : register(t6);
struct SpotLight
{
	float4 PositionWS;
	float4 DirectionWS;
	float4 Color;
	float Intensity;
	float SpotAngle;
	float Attenuation;
	float Padding;
}; // Total:                              16 * 4 = 64 bytes
StructuredBuffer<SpotLight> SpotLights : register(t7);
struct Camera
{
	float4 PositionWS;
	float4x4 InverseViewMatrix;
	float fov;
	float3 padding;
}; // Total:                              16 * 6 = 64 bytes
ConstantBuffer<Camera> CameraCB : register(b0);
struct LightProperties
{
	uint NumPointLights;
	uint NumSpotLights;
	float2 Padding;
};
ConstantBuffer<LightProperties>LightPropertiesCB : register(b1);
struct RayPayload
{
	bool hit;
};

////////////////////////////////////////////////////// PBR workflow SchlickGGX
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
float3 DoPbrPointLight(PointLight light, float3 N, float3 V, float3 P, float3 albedo, float roughness, float metallic)
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
	return (kD * albedo / PI + specular) * radiance * NdotL;
}

float3 DoPbrSpotLight(SpotLight light, float3 N, float3 V, float3 P, float3 albedo, float roughness, float metallic)
{
	const float PI = 3.14159265359;
	float3 F0 = float3(0.04, 0.04, 0.04);
	F0 = lerp(F0, albedo, metallic);
	
	float3 L = normalize(light.PositionWS.xyz - P);
	float3 H = normalize(V + L);
	float distance = length(light.PositionWS.xyz - P);
	float spotIntensity = DoSpotCone(light.DirectionWS.xyz, L, light.SpotAngle);
	float attenuation = DoAttenuation(light.Attenuation, distance);
	float3 radiance = light.Color.xyz * attenuation * spotIntensity;

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
	float3 kD = float3(1.0,1.0,1.0) - kS;
    // multiply kD by the inverse metalness such that only non-metals 
    // have diffuse lighting, or a linear blend if partly metal (pure metals
    // have no diffuse light).
	kD *= 1.0 - metallic;

    // scale light by NdotL
	float NdotL = max(dot(N, L), 0.0);

    // add to outgoing radiance Lo
	return (kD * albedo / PI + specular) * radiance * NdotL;
}
////////////////////////////////////////////////////// PBR workflow SchlickGGX

[shader("raygeneration")]
void rayGen()
{
    uint3 launchIndex = DispatchRaysIndex();
	
	// //primary ray simulation
	//uint3 launchDim = DispatchRaysDimensions();
	//float2 crd = float2(launchIndex.xy);
	//float2 dims = float2(launchDim.xy);
	//float2 d = ((crd / dims) * 2.f - 1.f);
	//float aspectRatio = dims.x / dims.y;
	//float3 cameraPositionWS = CameraCB.PositionWS.xyz;
	//float3 cameraDirectionWS = normalize(float3(d.x * aspectRatio, -d.y, 1 / tan(radians(CameraCB.fov / 2.0))));
	//float3 viewDirectionWS = mul((float3x3) CameraCB.InverseViewMatrix, cameraDirectionWS);
	//RayDesc ray;
	//ray.Origin = cameraPositionWS;
	//ray.Direction = viewDirectionWS;
	//ray.TMin = 0;
	//ray.TMax = 100000;
	//RayPayload payload;
	//TraceRay(gRtScene,
	//	RAY_FLAG_CULL_BACK_FACING_TRIANGLES /*RayFlags*/, 0xFF /*InstanceInclusionMask*/,
	//	0 /* RayContributionToHitGroupIndex*/, 0 /*MultiplierForGeometryContributionToHitGroupIndex*/, 0 /*MissShaderIndex*/,
	//	ray, payload
	//);
	//float3 color = GAlbedo.Load(int3(crd.x, crd.y, 0)).rgb;
	//gOutput[launchIndex.xy] = float4(color, 1);

	//TODO: bllin-phong model -> PBR sampling
	float hit = GPosition.Load(int3(launchIndex.x, launchIndex.y, 0)).w;
	if (hit == 0)
	{
		gOutput[launchIndex.xy] = float4(0,0,0,1);
		return;
	}
	float3 position = GPosition.Load(int3(launchIndex.x, launchIndex.y, 0)).xyz;
	float3 albedo = GAlbedo.Load(int3(launchIndex.x, launchIndex.y, 0)).xyz;
	float roughness = GRoughness.Load(int3(launchIndex.x, launchIndex.y, 0)).x;
	float metallic = GMetallic.Load(int3(launchIndex.x, launchIndex.y, 0)).x;
	float3 normal = GNormal.Load(int3(launchIndex.x, launchIndex.y, 0)).xyz;
	
	
	// Lighting is performed in world space.
	float3 V = normalize(CameraCB.PositionWS.xyz - position);
	float3 N = normalize(normal);
	float3 P = position;
	RayDesc ray;
	ray.TMin = 0.01f;
	RayPayload shadowPayload;
	
	uint i;
	float3 Lo = 0;
	for (i = 0; i < LightPropertiesCB.NumPointLights; ++i)
	{
		float shadow = 0.0f;
		float bias = 0.1f;
		float samples = 4.0f;
		float offset = 0.1f;
		for (float x = -offset; x < offset; x += offset / (samples * 0.5))
		{
			for (float y = -offset; y < offset; y += offset / (samples * 0.5))
			{
				for (float z = -offset; z < offset; z += offset / (samples * 0.5))
				{
					float3 dest = PointLights[i].PositionWS.xyz + float3(x, y, z);
					ray.Origin = P;
					ray.Direction = normalize(dest - P);
					ray.TMax = distance(dest, P);
					TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, ray, shadowPayload);
					if (shadowPayload.hit == false)
					{
						shadow = shadow + 1.0f;
					}
				}
			}
		}
		Lo += shadow * DoPbrPointLight(PointLights[i], N, V, P, albedo, roughness, metallic) / (samples * samples * samples);
	}

	for (i = 0; i < LightPropertiesCB.NumSpotLights; ++i)
	{
		float shadow = 0.0f;
		float bias = 0.1f;
		float samples = 4.0f;
		float offset = 0.1f;
		for (float x = -offset; x < offset; x += offset / (samples * 0.5))
		{
			for (float y = -offset; y < offset; y += offset / (samples * 0.5))
			{
				for (float z = -offset; z < offset; z += offset / (samples * 0.5))
				{
					float3 dest = SpotLights[i].PositionWS.xyz + float3(x, y, z);
					ray.Origin = P;
					ray.Direction = normalize(dest - P);
					ray.TMax = distance(dest, P);
					TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, ray, shadowPayload);
					if (shadowPayload.hit == false)
					{
						shadow = shadow + 1.0f;
					}
				}
			}
		}
		Lo += shadow * DoPbrSpotLight(SpotLights[i], N, V, P, albedo, roughness, metallic) / (samples * samples * samples);
	}
	
	float3 ambient = 0.01f * albedo;
	float3 color = ambient + Lo;
	gOutput[launchIndex.xy] = float4(color.rgb, 1);
}

[shader("miss")]
void miss(inout RayPayload payload)
{
	payload.hit = false;
}

[shader("anyhit")]
void ahs(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
	//AcceptHitAndEndSearch();
}

[shader("closesthit")]
void chs(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
	payload.hit = true;
}
