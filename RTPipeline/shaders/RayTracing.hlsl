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
    //----------------------------------- (16 byte boundary)
	float4 Color;
    //----------------------------------- (16 byte boundary)
	float Intensity;
	float Attenuation;
	float2 Padding; // Pad to 16 bytes
    //----------------------------------- (16 byte boundary)  
}; // Total:                              16 * 3 = 48 bytes
StructuredBuffer<PointLight> PointLights : register(t6);
struct SpotLight
{
	float4 PositionWS;
    //----------------------------------- (16 byte boundary)
	float4 DirectionWS;
    //----------------------------------- (16 byte boundary)
	float4 Color;
    //----------------------------------- (16 byte boundary)
	float Intensity;
	float SpotAngle;
	float Attenuation;
	float Padding;
    //----------------------------------- (16 byte boundary) 
}; // Total:                              16 * 4 = 64 bytes
StructuredBuffer<SpotLight> SpotLights : register(t7);

struct Camera
{
	float4 PositionWS;
	//----------------------------------- (16 byte boundary)
	float4x4 InverseViewMatrix;
	//----------------------------------- (64 byte boundary)
	float fov;
	float3 padding;
	//----------------------------------- (16 byte boundary)
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
struct LightResult
{
	float4 Diffuse;
	float4 Specular;
};

float DoDiffuse(float3 N, float3 L)
{
	return max(0, dot(N, L));
}

float DoSpecular(float3 V, float3 N, float3 L)
{
	float3 R = normalize(reflect(-L, N));
	float RdotV = max(0, dot(R, V));

	return pow(RdotV, 8);
}

float DoAttenuation(float attenuation, float distance)
{
	return 1.0f / (1.0f + attenuation * distance * distance);
}

float DoSpotCone(float3 spotDir, float3 L, float spotAngle)
{
	float minCos = cos(spotAngle);
	float maxCos = (minCos + 1.0f) / 2.0f;
	float cosAngle = dot(spotDir, -L);
	return smoothstep(minCos, maxCos, cosAngle);
}

LightResult DoPointLight(PointLight light, float3 V, float3 P, float3 N)
{
	LightResult result;
	float3 L = (light.PositionWS.xyz - P);
	float d = length(L);
	L = L / d;

	float attenuation = DoAttenuation(light.Attenuation, d);

	result.Diffuse = DoDiffuse(N, L) * attenuation * light.Color * light.Intensity;
	result.Specular = DoSpecular(V, N, L) * attenuation * light.Color * light.Intensity;

	return result;
}

LightResult DoSpotLight(SpotLight light, float3 V, float3 P, float3 N)
{
	LightResult result;
	float3 L = (light.PositionWS.xyz - P);
	float d = length(L);
	L = L / d;

	float attenuation = DoAttenuation(light.Attenuation, d);

	float spotIntensity = DoSpotCone(light.DirectionWS.xyz, L, light.SpotAngle);

	result.Diffuse = DoDiffuse(N, L) * attenuation * spotIntensity * light.Color * light.Intensity;
	result.Specular = DoSpecular(V, N, L) * attenuation * spotIntensity * light.Color * light.Intensity;

	return result;
}

[shader("raygeneration")]
void rayGen()
{
    uint3 launchIndex = DispatchRaysIndex();
    //uint3 launchDim = DispatchRaysDimensions();

	//float2 crd = float2(launchIndex.xy);
 //   float2 dims = float2(launchDim.xy);

 //   float2 d = ((crd/dims) * 2.f - 1.f);
 //   float aspectRatio = dims.x / dims.y;
	
	//float3 cameraPositionWS = CameraCB.PositionWS.xyz;
	//float3 cameraDirectionWS = normalize(float3(d.x * aspectRatio, -d.y, 1 / tan(radians(CameraCB.fov / 2.0))));
	//float3 viewDirectionWS = mul((float3x3) CameraCB.InverseViewMatrix, cameraDirectionWS);

 //   RayDesc ray;
	//ray.Origin = cameraPositionWS;
	//ray.Direction = viewDirectionWS;

 //   ray.TMin = 0;
 //   ray.TMax = 100000;

 //   RayPayload payload;
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
	float3 V = normalize((float3) CameraCB.PositionWS - position);
	float3 N = normalize(normal);
	float3 P = position;
	RayDesc ray;
	ray.TMin = 0;
	RayPayload shadowPayload;
	LightResult totalResult = (LightResult) 0;
	uint i;
	for (i = 0; i < LightPropertiesCB.NumPointLights; ++i)
	{
		ray.Origin = P;
		ray.Direction = normalize((float3) PointLights[i].PositionWS - P);
		float dis = distance((float3) PointLights[i].PositionWS, P);
		ray.TMax = dis;
		TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, ray, shadowPayload);
		if (shadowPayload.hit == false)
		{
			LightResult result = DoPointLight(PointLights[i], V, P, N);
			totalResult.Diffuse += result.Diffuse;
			totalResult.Specular += result.Specular;
		}
	}

	for (i = 0; i < LightPropertiesCB.NumSpotLights; ++i)
	{
		ray.Origin = P;
		ray.Direction = normalize((float3) SpotLights[i].PositionWS - P);
		float dis = distance((float3) SpotLights[i].PositionWS, P);
		ray.TMax = dis;
		TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, ray, shadowPayload);
		if (shadowPayload.hit == false)
		{
			LightResult result = DoSpotLight(SpotLights[i], V, P, N);
			totalResult.Diffuse += result.Diffuse;
			totalResult.Specular += result.Specular;
		}
	}
	
	float3 resultColor = albedo * totalResult.Diffuse.xyz + float3(1.0f, 1.0f, 1.0f) * totalResult.Specular.xyz;
	gOutput[launchIndex.xy] = float4(resultColor, 1);
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
