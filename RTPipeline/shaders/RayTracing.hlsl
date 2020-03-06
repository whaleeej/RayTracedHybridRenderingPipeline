RWTexture2D<float4> gOutput : register(u0);
RaytracingAccelerationStructure gRtScene : register(t0);
Texture2D<float4> GPosition : register(t1);
Texture2D<float4> GAlbedoMetallic : register(t2);
Texture2D<float4> GNormalRoughness : register(t3);
Texture2D<float4> GExtra : register(t4);
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
struct FrameIndex
{
	uint FrameIndex;
	float3 Padding;
}; // Total:                              16 * 1 = 16 bytes
ConstantBuffer<FrameIndex> FrameIndexCB : register(b2);

struct RayPayload
{
	bool hit;
};

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
//return (kD * albedo / PI ) * radiance * NdotL;
//return ( specular) * radiance * NdotL;
//return radiance * NdotL;
}
////////////////////////////////////////////////////// PBR workflow SchlickGGX

//////////////////////////////////////////////////// Random from svgf paper
unsigned int initRand(unsigned int val0, unsigned int val1, unsigned int backoff = 16)
{
	unsigned int v0 = val0, v1 = val1, s0 = 0;

	for (unsigned int n = 0; n < backoff; n++)
	{
		s0 += 0x9e3779b9;
		v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
		v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
	}
	return v0;
}
float nextRand(inout unsigned int s)
{
	s = (1664525u * s + 1013904223u);
	return float(s & 0x00FFFFFF) / float(0x01000000);
}
uint nextRandomRange(inout uint state, uint lower, uint upper) // [lower, upper]
{
	return lower + uint(float(upper - lower + 1) * nextRand(state));
}
//////////////////////////////////////////////////// Random

//////////////////////////////////////////////////// A low-discrepancy sequence: Hammersley Sequence
// ref: https://learnopengl.com/PBR/IBL/Specular-IBL
float RadicalInverse_VdC(uint bits) 
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
float2 Hammersley(uint i, uint N)
{
	return float2(float(i) / float(N), RadicalInverse_VdC(i));
}
//////////////////////////////////////////////////// A low-discrepancy sequence

//////////////////////////////////////////////////// Sampling method
float3 UniformSampleCone(float2 u, float cosThetaMax)
{
	float cosTheta = (1.0f - u.x) + u.x * cosThetaMax;
	float sinTheta = sqrt( 1.0f - cosTheta * cosTheta);
	float phi = u.y * 2 * 3.141592653f;
	return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}
float3 SampleTan2W(float3 H, float3 N)
{
	float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
	float3 tangent = normalize(cross(up, N));
	float3 bitangent = cross(N, tangent);
	
	float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return sampleVec;
}
//////////////////////////////////////////////////// Sampling method

[shader("raygeneration")]
void rayGen()
{
	// TraceRay(gRtScene,
	//	RAY_FLAG_CULL_BACK_FACING_TRIANGLES /*RayFlags*/, 0xFF /*InstanceInclusionMask*/,
	//	0 /* RayContributionToHitGroupIndex*/, 0 /*MultiplierForGeometryContributionToHitGroupIndex*/, 0 /*MissShaderIndex*/, 
	// ray, rayPayload)
	
	uint3 launchIndex = DispatchRaysIndex();
	uint3 launchDimension = DispatchRaysDimensions();
	float hit = GPosition.Load(int3(launchIndex.x, launchIndex.y, 0)).w;
	if (hit == 0)
	{
		gOutput[launchIndex.xy] = float4(0, 0, 0, 1);
		return;
	}
	float3 position = GPosition.Load(int3(launchIndex.x, launchIndex.y, 0)).xyz;
	float3 albedo = GAlbedoMetallic.Load(int3(launchIndex.x, launchIndex.y, 0)).xyz;
	float roughness = GNormalRoughness.Load(int3(launchIndex.x, launchIndex.y, 0)).w;
	float metallic = GAlbedoMetallic.Load(int3(launchIndex.x, launchIndex.y, 0)).w;
	float3 normal = GNormalRoughness.Load(int3(launchIndex.x, launchIndex.y, 0)).xyz;
	float3 emissive = GExtra.Load(int3(launchIndex.x, launchIndex.y, 0)).xyz;
	float gid = GExtra.Load(int3(launchIndex.x, launchIndex.y, 0)).w;
	
	// Lighting is performed in world space.
	float3 P = position;
	float3 V = normalize(CameraCB.PositionWS.xyz - position);
	float3 N = normalize(normal);
	
	uint seed = initRand((launchIndex.x + (launchIndex.y * launchDimension.y)), FrameIndexCB.FrameIndex, 16);
	float2 lowDiscrepSeq = Hammersley(nextRandomRange(seed, 0, 4095), 4096);
	float rnd1 = lowDiscrepSeq.x;
	float rnd2 = lowDiscrepSeq.y;

	//// shadow and lighting
	RayDesc ray;
	ray.TMin = 0.0f;
	RayPayload shadowPayload;
	float3 Lo = 0;

	//// area Point Light
	// low discrep sampling
	float bias = 1e-2f;
	float3 P_biased = P + N * bias;
	float3 dest = pointLight.PositionWS.xyz;
	float3 distan = distance(P_biased, dest);
	float3 dir = (dest - P_biased) / distan;
	float3 sampleDestRadius = pointLight.radius;
	float maxCosTheta = distan / sqrt(sampleDestRadius * sampleDestRadius + distan * distan);
	float3 distributedSampleAngleinTangentSpace = UniformSampleCone(float2(rnd1, rnd2), maxCosTheta);
	float3 distributedDir = SampleTan2W(distributedSampleAngleinTangentSpace, dir);
	// ray setup
	ray.Origin = P_biased;
	ray.Direction = distributedDir;
	ray.TMax = distan * length(distributedSampleAngleinTangentSpace) / distributedSampleAngleinTangentSpace.z;
	shadowPayload.hit = false;
	// ray tracing
	TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, 0, 0, 0, ray, shadowPayload);
	if (shadowPayload.hit == false)
	{
		Lo += 1.0f;
		//Lo += DoPbrPointLight(pointLight, N, V, P, albedo, roughness, metallic);
	}

	// output
	float3 color = Lo;
	gOutput[launchIndex.xy] = float4(color, 1);
}

[shader("miss")]
void miss(inout RayPayload payload)
{
	payload.hit = false;
}

[shader("closesthit")]
void chs(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
	payload.hit = true;
}
