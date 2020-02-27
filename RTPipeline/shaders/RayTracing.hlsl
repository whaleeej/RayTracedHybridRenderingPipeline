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
}; // Total:                              16 * 6 = 96 bytes
ConstantBuffer<Camera> CameraCB : register(b0);
struct LightProperties
{
	uint NumPointLights;
	uint NumSpotLights;
	float2 Padding;
}; // Total:                              16 * 1 = 16 bytes
ConstantBuffer<LightProperties>LightPropertiesCB : register(b1);
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

///////////////////////////////////////////////////// Halton Sampling
struct HaltonState
{
	int dimension;
	int sequenceIndex;
};
int haltonIndex(int x, int y, int i, inout uint mIncrement);
void haltonInit(inout HaltonState hState, int x, int y, int path, int numPaths,
                int frameId, int loop, inout uint mIncrement)
{

	hState.dimension = 2;
	hState.sequenceIndex =
        haltonIndex(x, y, (frameId * numPaths + path) % (loop * numPaths), mIncrement);
}
float haltonSample(int dimension, int sampleIndex)
{
	int base = 0;

    // Use a prime number.
	switch (dimension)
	{
		case 0:
			base = 2;
			break;
		case 1:
			base = 3;
			break;
		case 2:
			base = 5;
			break;
		case 3:
			base = 7;
			break;
		case 4:
			base = 11;
			break;
		case 5:
			base = 13;
			break;
		case 6:
			base = 17;
			break;
		case 7:
			base = 19;
			break;
		case 8:
			base = 23;
			break;
		case 9:
			base = 29;
			break;
		case 10:
			base = 31;
			break;
		case 11:
			base = 37;
			break;
		case 12:
			base = 41;
			break;
		case 13:
			base = 43;
			break;
		case 14:
			base = 47;
			break;
		case 15:
			base = 53;
			break;
		case 16:
			base = 59;
			break;
		case 17:
			base = 61;
			break;
		case 18:
			base = 67;
			break;
		case 19:
			base = 71;
			break;
		case 20:
			base = 73;
			break;
		case 21:
			base = 79;
			break;
		case 22:
			base = 83;
			break;
		case 23:
			base = 89;
			break;
		case 24:
			base = 97;
			break;
		case 25:
			base = 101;
			break;
		case 26:
			base = 103;
			break;
		case 27:
			base = 107;
			break;
		case 28:
			base = 109;
			break;
		case 29:
			base = 113;
			break;
		case 30:
			base = 127;
			break;
		case 31:
			base = 131;
			break;
		default:
			base = 2;
			break;
	}

    // Compute the radical inverse.
	float a = 0.0;
	float invBase = 1.0 / float(base);

	for (float mult = invBase; sampleIndex != 0;
         sampleIndex /= base, mult *= invBase)
	{
		a += float(sampleIndex % base) * mult;
	}

	return a;
}
float haltonNext(inout HaltonState state, inout uint mIncrement)
{
	mIncrement = mIncrement < 1 ? 1 : mIncrement + 1;
	return haltonSample(state.dimension++, state.sequenceIndex);
}
int halton2Inverse(int index, int digits)
{
	index = (index << 16) | (index >> 16);
	index = ((index & 0x00ff00ff) << 8) | ((index & 0xff00ff00) >> 8);
	index = ((index & 0x0f0f0f0f) << 4) | ((index & 0xf0f0f0f0) >> 4);
	index = ((index & 0x33333333) << 2) | ((index & 0xcccccccc) >> 2);
	index = ((index & 0x55555555) << 1) | ((index & 0xaaaaaaaa) >> 1);
	return index >> (32 - digits);
}
int halton3Inverse(int index, int digits)
{
	int result = 0;
	for (int d = 0; d < digits; ++d)
	{
		result = result * 3 + index % 3;
		index /= 3;
	}
	return result;
}
int haltonIndex(int x, int y, int i, inout uint mIncrement)
{
	return ((halton2Inverse(x % 256, 8) * 76545 +
             halton3Inverse(y % 256, 6) * 110080) %
            mIncrement) +
           i * 186624;
}
///////////////////////////////////////////////////// Halton Sampling

//////////////////////////////////////////////////// Random Utility
// Create an initial random number for this thread
void RandomSeedInit(inout uint seed)
{
    // Thomas Wang hash 
    // Ref: http://www.burtleburtle.net/bob/hash/integer.html
	seed = (seed ^ 61) ^ (seed >> 16);
	seed *= 9;
	seed = seed ^ (seed >> 4);
	seed *= 0x27d4eb2d;
	seed = seed ^ (seed >> 15);
}
// Generate a random 32-bit integer
uint Random(inout uint state)
{
    // Xorshift algorithm from George Marsaglia's paper.
	state ^= (state << 13);
	state ^= (state >> 17);
	state ^= (state << 5);
	return state;
}
// Generate a random float in the range [0.0f, 1.0f)
float Random01(inout uint state)
{
	return asfloat(0x3f800000 | Random(state) >> 9) - 1.0;
}
// Generate a random float in the range [0.0f, 1.0f]
float Random01inclusive(inout uint state)
{
	return Random(state) / float(0xffffffff);
}
// Generate a random integer in the range [lower, upper]
uint Random(inout uint state, uint lower, uint upper)
{
	return lower + uint(float(upper - lower + 1) * Random01(state));
}
//////////////////////////////////////////////////// Random Utility

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
	//TraceRay(gRtScene,
	//	RAY_FLAG_CULL_BACK_FACING_TRIANGLES /*RayFlags*/, 0xFF /*InstanceInclusionMask*/,
	//	0 /* RayContributionToHitGroupIndex*/, 0 /*MultiplierForGeometryContributionToHitGroupIndex*/, 0 /*MissShaderIndex*/,
	
	uint3 launchIndex = DispatchRaysIndex();
	uint3 launchDimension = DispatchRaysDimensions();
	float hit = GPosition.Load(int3(launchIndex.x, launchIndex.y, 0)).w;
	if (hit == 0)
	{
		gOutput[launchIndex.xy] = float4(0, 0, 0, 1);
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
	
	uint seed = FrameIndexCB.FrameIndex % (launchIndex.x + (launchIndex.y * launchDimension.y));
	RandomSeedInit(seed);
	HaltonState hState;
	uint useed = Random(seed, 0, 255); /*[0,255]*/
	uint mIncrement = 23;
	haltonInit(hState, launchIndex.x, launchIndex.y, useed, 255, FrameIndexCB.FrameIndex, 145321, mIncrement);
	float rnd1 = frac(haltonNext(hState, mIncrement));
	float rnd2 = frac(haltonNext(hState, mIncrement));
	
	float3 sampleDestRadius = 2.0f;
	
	// shadow and lighting
	RayDesc ray;
	ray.TMin = 0.00f;
	RayPayload shadowPayload;
	uint i;
	float3 Lo = 0;
	for (i = 0; i < LightPropertiesCB.NumPointLights; ++i)
	{
		// area Point Light
		float3 dest = PointLights[i].PositionWS.xyz;
		float3 distan = distance(P, dest);
		float3 dir = (dest - P) / distan;
		float maxCosTheta = distan / sqrt(sampleDestRadius * sampleDestRadius + distan * distan);
		float3 distributedSampleAngleinTangentSpace = UniformSampleCone(float2(rnd1, rnd2), maxCosTheta);
		float3 distributedDir = SampleTan2W(distributedSampleAngleinTangentSpace, dir);
		ray.Origin = P;
		ray.Direction = distributedDir;
		ray.TMax = distan * length(distributedSampleAngleinTangentSpace) / distributedSampleAngleinTangentSpace.z;
		shadowPayload.hit = false;
		TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, 0, 0, 0, ray, shadowPayload);
		if (shadowPayload.hit == false)
		{
			Lo += DoPbrPointLight(PointLights[i], N, V, P, albedo, roughness, metallic);
		}
	}
	for (i = 0; i < LightPropertiesCB.NumSpotLights; ++i)
	{
		// area Point Light
		float3 dest = SpotLights[i].PositionWS.xyz;
		float3 distan = distance(P, dest);
		float3 dir = (dest - P) / distan;
		float maxCosTheta = distan / sqrt(sampleDestRadius * sampleDestRadius + distan * distan);
		float3 distributedSampleAngleinTangentSpace = UniformSampleCone(float2(rnd1, rnd2), maxCosTheta);
		float3 distributedDir = SampleTan2W(distributedSampleAngleinTangentSpace, dir);
		ray.Origin = P;
		ray.Direction = distributedDir;
		ray.TMax = distan * length(distributedSampleAngleinTangentSpace) / distributedSampleAngleinTangentSpace.z;
		shadowPayload.hit = false;
		TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, 0, 0, 0, ray, shadowPayload);
		if (shadowPayload.hit == false)
		{
			Lo += DoPbrSpotLight(SpotLights[i], N, V, P, albedo, roughness, metallic);
		}
	}
	
	float3 color = Lo;
	gOutput[launchIndex.xy] = float4(color, 1);
}

[shader("miss")]
void miss(inout RayPayload payload)
{
	//payload.hit = false;
}

[shader("closesthit")]
void chs(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
	payload.hit = true;
}
