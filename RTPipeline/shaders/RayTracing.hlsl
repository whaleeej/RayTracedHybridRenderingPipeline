struct ShadowRayPayload
{
	bool hit;
};
struct SecondaryPayload
{
	float4 color;
};

//**********************************************************************************************************************//
//**************************************************for raygen shadow*************************************************//
//**********************************************************************************************************************//
RWTexture2D<float4> shadowOutput : register(u0);
RWTexture2D<float4> reflectOutput : register(u1);
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
}; ConstantBuffer<PointLight> pointLight : register(b0);
struct Camera
{
	float4 PositionWS;
	float4x4 InverseViewMatrix;
	float fov;
	float3 padding;
}; ConstantBuffer<Camera>cameraCB : register(b1);
struct FrameIndex
{
	uint FrameIndex;
	float3 Padding;
}; ConstantBuffer<FrameIndex> frameIndexCB : register(b2);

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
float3 DoPbrRadiance(float3 radiance, float3 L, float3 N, float3 V, float3 P, float3 albedo, float roughness, float metallic)
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
// from PBRT
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
// from https://learnopengl.com/PBR/IBL/Specular-IBL based on Disney's research on PBRT
float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
	float PI = 3.141592653;
	float a = roughness * roughness;
	
	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	
    // from spherical coordinates to cartesian coordinates
	float3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;
	
	return normalize(SampleTan2W(H,N));
}
float3 UniformSample(float2 Xi, float3 N)
{
	float PI = 3.141592653;

	float phi = 2.0 * PI * Xi.x;
	float theta = 0.5 * PI * Xi.y;
	
    // from spherical coordinates to cartesian coordinates
	float3 R;
	R.x = cos(phi) * sin(theta);
	R.y = sin(phi) * sin(theta);
	R.z =  cos(theta);
	
	return normalize(SampleTan2W(R, N));
}
float ImportanceSamplePdf(float roughness, float cosTheta)
{
	float PI = 3.1415926536;
	float a = roughness * roughness;
	float a_sq = a * a;
	float sinThera = sqrt(1 - cosTheta * cosTheta);
	float demon = ((1 - a_sq) * cosTheta * cosTheta - 1);
	float demon_sq_pi = demon * demon * 2 * PI;
	float pdf = (2 * a_sq * sinThera * cosTheta) / demon_sq_pi;
	return pdf;
}
//////////////////////////////////////////////////// Sampling method

[shader("raygeneration")]
void rayGen()
{
	// TraceRay(gRtScene,
	//	RAY_FLAG_CULL_BACK_FACING_TRIANGLES /*RayFlags*/, 0xFF /*InstanceInclusionMask*/,
	//	0 /* RayContributionToHitGroupIndex*/, 0 /*MultiplierForGeometryContributionToHitGroupIndex*/, 0 /*MissShaderIndex*/, 
	// ray, ShadowRayPayload)
	
	uint3 launchIndex = DispatchRaysIndex();
	uint3 launchDimension = DispatchRaysDimensions();
	float hit = GPosition.Load(int3(launchIndex.x, launchIndex.y, 0)).w;
	if (hit == 0)
	{
		shadowOutput[launchIndex.xy] = float4(0, 0, 0, 1);
		reflectOutput[launchIndex.xy] = float4(0, 0, 0, 1);
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
	float3 V = normalize(cameraCB.PositionWS.xyz - position);
	float3 N = normalize(normal);
	
	uint seed = initRand((launchIndex.x + (launchIndex.y * launchDimension.y)), frameIndexCB.FrameIndex, 16);
	float2 lowDiscrepSeq = Hammersley(nextRandomRange(seed, 0, 4095), 4096);
	float rnd1 = lowDiscrepSeq.x;
	float rnd2 = lowDiscrepSeq.y;

	//// shadow and lighting
	RayDesc ray;
	ray.TMin = 0.0f;
	ShadowRayPayload shadowPayload;
	float Lo = 0;
	float3 Lr = 0;

	//// area Point Light
	// low discrep sampling
	float bias = 1e-2f;
	float3 P_biased = P + N * bias;
	float3 dest = pointLight.PositionWS.xyz;
	float distan = distance(P_biased, dest);
	float3 dir = (dest - P_biased) / distan;
	float sampleDestRadius = pointLight.radius;
	float maxCosTheta = distan / sqrt(sampleDestRadius * sampleDestRadius + distan * distan);
	float3 distributedSampleAngleinTangentSpace = UniformSampleCone(float2(rnd1, rnd2), maxCosTheta);
	float3 distributedDir = SampleTan2W(distributedSampleAngleinTangentSpace, dir);
	// ray setup
	ray.Origin = P_biased;
	ray.Direction = distributedDir;
	ray.TMax = distan * length(distributedSampleAngleinTangentSpace) / distributedSampleAngleinTangentSpace.z;
	shadowPayload.hit = false;
	// ray tracing
	TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 
	0xFF, 0, 0, 0, ray, shadowPayload);
	if (shadowPayload.hit == false)
	{
		Lo += 1.0f;
	}
	
	// reflection
	float3 H = ImportanceSampleGGX(float2(rnd1, rnd2), N, roughness); // importance sample the half vector
	RayDesc raySecondary;
	raySecondary.TMin = 0.01f;
	raySecondary.Origin = P;
	raySecondary.Direction = reflect(-V, H);
	raySecondary.TMax = 10000.0f;
	SecondaryPayload secondaryPayload;
	secondaryPayload.color = float4(0, 0, 0, 0);
	float pdf = 1;
	if (dot(raySecondary.Direction, N) >= 0) // trace reflected ray only if the ray is not below the surface
	{
		TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
		0xFF, 1, 0, 1, raySecondary, secondaryPayload);
		pdf= ImportanceSamplePdf(roughness, dot(N, H));
	}
	else
	{
		raySecondary.Direction = UniformSample(float2(rnd1, rnd2), N);
		TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
		0xFF, 1, 0, 1, raySecondary, secondaryPayload);
		pdf = 1.0f;
	}
	if(secondaryPayload.color.z != 0.0)
	{
		Lr = DoPbrRadiance(secondaryPayload.color.xyz, reflect(-V, H), N, V, P, albedo, roughness, metallic) / max(min(pdf, 2.0),0.3);
		//Lr =secondaryPayload.color.xyz;
	}
	// output
	shadowOutput[launchIndex.xy] = float4(Lo, 0,0,0);
	reflectOutput[launchIndex.xy] = float4(Lr, 0);
}
//**********************************************************************************************************************//
//**********************************************************************************************************************//
//**********************************************************************************************************************//





//**********************************************************************************************************************//
//************************************************** for shadow chs***************************************************//
//**********************************************************************************************************************//
[shader("miss")]
void shadowMiss(inout ShadowRayPayload payload)
{
	payload.hit = false;
}

[shader("closesthit")]
void shadowChs(inout ShadowRayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
	payload.hit = true;
}
//**********************************************************************************************************************//
//**********************************************************************************************************************//
//**********************************************************************************************************************//






//**********************************************************************************************************************//
//************************************************** for reflection chs**************************************************//
//**********************************************************************************************************************//
RaytracingAccelerationStructure localRtScene : register(t0, space1);
ByteAddressBuffer Indices : register(t1, space1);
struct Vertex
{
	float3 position;
	float3 normal;
	float2 textureCoordinate;
};StructuredBuffer<Vertex> Vertices : register(t2, space1);
Texture2D AlbedoTexture : register(t3, space1);
Texture2D MetallicTexture : register(t4, space1);
Texture2D NormalTexture : register(t5, space1);
Texture2D RoughnessTexture : register(t6, space1);
struct Material
{
	float4 Emissive;
	float4 Ambient;
	float4 Diffuse;
	float4 Specular;
	float SpecularPower;
	float3 Padding;
}; ConstantBuffer<Material> MaterialCB : register(b0, space1);
struct PBRMaterial
{
	float Metallic;
	float Roughness;
	float2 Padding;
}; ConstantBuffer<PBRMaterial> PBRMaterialCB : register(b1, space1);
struct ObjectIndex
{
	float index;
	float3 padding;
};
ConstantBuffer<ObjectIndex> GameObjectIndex : register(b2, space1);
ConstantBuffer<PointLight> localPointLight : register(b3, space1);
ConstantBuffer<FrameIndex> localFrameIndexCB : register(b4, space1);
SamplerState AnisotropicSampler : register(s0);

float3 LinearToSRGB(float3 x)
{
    // This is exactly the sRGB curve
	return x < 0.0031308 ? 12.92 * x : 1.055 * pow(abs(x), 1.0 / 2.4) - 0.055;
}

float3 HitWorldPosition()
{
	return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

// Load three 16 bit indices from a byte addressed buffer.
uint3 Load3x16BitIndices(uint offsetBytes)
{
	uint3 indices;

    // ByteAdressBuffer loads must be aligned at a 4 byte boundary.
    // Since we need to read three 16 bit indices: { 0, 1, 2 } 
    // aligned at a 4 byte boundary as: { 0 1 } { 2 0 } { 1 2 } { 0 1 } ...
    // we will load 8 bytes (~ 4 indices { a b | c d }) to handle two possible index triplet layouts,
    // based on first index's offsetBytes being aligned at the 4 byte boundary or not:
    //  Aligned:     { 0 1 | 2 - }
    //  Not aligned: { - 0 | 1 2 }
	const uint dwordAlignedOffset = offsetBytes & ~3;
	const uint2 four16BitIndices = Indices.Load2(dwordAlignedOffset);
 
    // Aligned: { 0 1 | 2 - } => retrieve first three 16bit indices
	if (dwordAlignedOffset == offsetBytes)
	{
		indices.x = four16BitIndices.x & 0xffff;
		indices.y = (four16BitIndices.x >> 16) & 0xffff;
		indices.z = four16BitIndices.y & 0xffff;
	}
	else // Not aligned: { - 0 | 1 2 } => retrieve last three 16bit indices
	{
		indices.x = (four16BitIndices.x >> 16) & 0xffff;
		indices.y = four16BitIndices.y & 0xffff;
		indices.z = (four16BitIndices.y >> 16) & 0xffff;
	}

	return indices;
}

float3 getFromNormalMapping(float3 sampledNormal, float3 NormalWS, float3 vertexPosition[3], float2 vertexUV[3], float3x3 R)
{
	float3 tangentNormal = sampledNormal.xyz * 2.0 - 1.0; // [-1, 1]
	
	float3 deltaXYZ_1 = mul(R, vertexPosition[1]) - mul(R, vertexPosition[0]);
	float3 deltaXYZ_2 = mul(R, vertexPosition[2]) - mul(R, vertexPosition[0]);
	float2 deltaUV_1 = vertexUV[1] - vertexUV[0];
	float2 deltaUV_2 = vertexUV[2] - vertexUV[0];
	
	float3 N = normalize(NormalWS);
	float3 T = normalize(deltaUV_2.y * deltaXYZ_1 - deltaUV_1.y * deltaXYZ_2);
	float3 B = normalize(cross(N, T));
	float3x3 TBN = float3x3(T, B, N);
	
	return normalize(mul(tangentNormal, TBN)); // [-1, 1]
}

float3 HitAttribute3(float3 vertexAttribute[3], BuiltInTriangleIntersectionAttributes attr)
{
	return vertexAttribute[0] +
        attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
        attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}

float2 HitAttribute2(float2 vertexAttribute[3], BuiltInTriangleIntersectionAttributes attr)
{
	return vertexAttribute[0] +
        attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
        attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}

[shader("closesthit")]
void secondaryChs(inout SecondaryPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
	float3 hitPosition = HitWorldPosition();

    // Get the base index of the triangle's first 16 bit index.
	uint indexSizeInBytes = 2;
	uint indicesPerTriangle = 3;
	uint triangleIndexStride = indicesPerTriangle * indexSizeInBytes;
	uint baseIndex = PrimitiveIndex() * triangleIndexStride;

    // Load up 3 16 bit indices for the triangle.
	const uint3 indices = Load3x16BitIndices(baseIndex);

    // Retrieve corresponding vertex normals for the triangle vertices.
	float3 vertexPositions[3] =
	{
		Vertices[indices[0]].position,
        Vertices[indices[1]].position,
        Vertices[indices[2]].position 
	};
	
	float3 vertexNormals[3] =
	{
		Vertices[indices[0]].normal,
        Vertices[indices[1]].normal,
        Vertices[indices[2]].normal 
	};
	
	float2 vertexTexCoord[3] =
	{
		Vertices[indices[0]].textureCoordinate,
        Vertices[indices[1]].textureCoordinate,
        Vertices[indices[2]].textureCoordinate 
	};

    // Compute the triangle's normal and textureCoord
	float3 triangleNormal = HitAttribute3(vertexNormals, attribs);
	float2 triangleTexCoord = HitAttribute2(vertexTexCoord, attribs);
	
	float3 worldNormal = normalize(mul((float3x3) ObjectToWorld3x4(), (triangleNormal)));

	float3 albedo = AlbedoTexture.SampleLevel(AnisotropicSampler, triangleTexCoord, 0).xyz * MaterialCB.Diffuse.xyz;
	float metallic =  MetallicTexture.SampleLevel(AnisotropicSampler, triangleTexCoord, 0).x * PBRMaterialCB.Metallic;
	float3 normal = NormalTexture.SampleLevel(AnisotropicSampler, triangleTexCoord, 0).xyz;
	float roughness = RoughnessTexture.SampleLevel(AnisotropicSampler, triangleTexCoord, 0).x * PBRMaterialCB.Roughness;
	
	float3 P = hitPosition;
	float3 N = getFromNormalMapping(normal, worldNormal, vertexPositions, vertexTexCoord, (float3x3) ObjectToWorld3x4());
	float3 V = normalize(-WorldRayDirection());
	
	// direct lighting
	uint seed = initRand((triangleTexCoord.x * 200 + (triangleTexCoord.y * 200*200)), localFrameIndexCB.FrameIndex, 16);
	float2 lowDiscrepSeq = Hammersley(nextRandomRange(seed, 0, 4095), 4096);
	float rnd1 = lowDiscrepSeq.x;
	float rnd2 = lowDiscrepSeq.y;
	//// shadow and lighting
	RayDesc ray;
	ray.TMin = 0.0f;
	ShadowRayPayload shadowPayload;
	//// area Point Light
	// low discrep sampling
	float bias = 1e-4f;
	float3 P_biased = P + N * bias;
	float3 dest = localPointLight.PositionWS.xyz;
	float distan = distance(P_biased, dest);
	float3 dir = (dest - P_biased) / distan;
	float sampleDestRadius = localPointLight.radius;
	float maxCosTheta = distan / sqrt(sampleDestRadius * sampleDestRadius + distan * distan);
	float3 distributedSampleAngleinTangentSpace = UniformSampleCone(float2(rnd1, rnd2), maxCosTheta);
	float3 distributedDir = SampleTan2W(distributedSampleAngleinTangentSpace, dir);
	// ray setup
	ray.Origin = P_biased;
	ray.Direction = distributedDir;
	ray.TMax = distan * length(distributedSampleAngleinTangentSpace) / distributedSampleAngleinTangentSpace.z;
	shadowPayload.hit = false;
	//shadow ray tracing
	TraceRay(localRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
	0xFF, 0, 0, 0, ray, shadowPayload);
	float3 color = 0.000;
	if (shadowPayload.hit == false)
	{
		color +=DoPbrPointLight(localPointLight, N, V, P, albedo, roughness, metallic);
	}
	
	payload.color = float4(color, 1);
}

[shader("miss")]
void secondaryMiss(inout SecondaryPayload payload)
{
	payload.color = float4(0, 0, 0, 0);
}
//**********************************************************************************************************************//
//**********************************************************************************************************************//
//**********************************************************************************************************************//