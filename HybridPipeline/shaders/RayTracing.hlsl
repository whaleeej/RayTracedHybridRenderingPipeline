/////////////////////////////////////////////////////////////////// definition
#define radian

struct RayPayload
{
	float3 color;
};

struct ShadowPayload
{
	bool hit;
};

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

struct LightProperties
{
	uint NumPointLights;
	uint NumSpotLights;
};

struct Camera
{
	float4 PositionWS;
	//----------------------------------- (16 byte boundary)
	//float4x4 InverseViewMatrix;
	////----------------------------------- (64 byte boundary)
	//float fov;
	//float3 padding;
	////----------------------------------- (16 byte boundary)
}; // Total:                              16 * 1 = 16 bytes


/////////////////////////////////////////////////////////////////// shaders
// scene
RaytracingAccelerationStructure gRtScene : register(t0);

//// inputs
//Texture2D<float4> PositionTexture : register(t1);
//Texture2D<float4> AlbedoTexture : register(t2);
//Texture2D<float4> MetallicTexture : register(t3);
Texture2D<float4> NormalTexture : register(t1);
//Texture2D<float4> RoughnessTexture : register(t5);

// outputs
RWTexture2D<float4> gOutput : register(u0);

//// lights
//ConstantBuffer<LightProperties> LightPropertiesCB : register(b0);
//StructuredBuffer<PointLight> PointLights : register(t6);
//StructuredBuffer<SpotLight> SpotLights : register(t7);

//// camera
//ConstantBuffer<Camera> CameraCB : register(b1);

float3 LinearToSRGB(float3 x)
{
// This is exactly the sRGB curve
//return x < 0.0031308 ? 12.92 * x : 1.055 * pow(abs(x), 1.0 / 2.4) - 0.055;

// This is cheaper but nearly equivalent
	return x < 0.0031308 ? 12.92 * x : 1.13005 * sqrt(abs(x - 0.00228)) - 0.13448 * x + 0.005719;
}

/////////////////////////////////////////////////////////////////// temporal
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


/////////////////////////////////////////////////////////////////// temporal



[shader("raygeneration")]
void rayGen()
{
	uint3 launchIndex = DispatchRaysIndex();
	gOutput[launchIndex.xy] = float4(0.9, 0.9, 0.9, 1);
	//uint3 launchDim = DispatchRaysDimensions();
	//float2 crd = float2(launchIndex.xy);
	//float2 dims = float2(launchDim.xy);
	//float2 d = ((crd / dims) * 2.f - 1.f);
	//float aspectRatio = dims.x / dims.y;
	
	//float4 hitPositionFlag = PositionTexture.Load(launchIndex).xyzw;
	//float3 hitPosition = hitPositionFlag.xyz;
	//float3 hitAlbedo = AlbedoTexture.Load(launchIndex).xyz;
	//float hitMetallic = MetallicTexture.Load(launchIndex).x;
	//float3 hitNormal = NormalTexture.Load(launchIndex).xyz;
	//float hitRoughness = RoughnessTexture.Load(launchIndex).x;

	//float3 cameraPositionWS = CameraCB.PositionWS.xyz;
	////float3 cameraDirectionWS = normalize(float3(d.x * aspectRatio, -d.y, 1 / tan(radians(CameraCB.fov / 2.0))));
	//float3 viewDirectionWS = normalize(hitPosition - cameraPositionWS);
	
	//if (hitPositionFlag.a == 0)
	//{
	//	//TODO: skybox
	//	gOutput[launchIndex.xy] = float4(0,0,0, 1);
	//	return;
	//}
	
	//gOutput[launchIndex.xy] = float4((hitNormal + 1.0)/2.0, 1);
	
	
	////TODO: bllin-phong model -> PBR sampling
	//uint i;
	//// Lighting is performed in world space.
	//float3 V = normalize(viewDirectionWS);
	//float3 N = hitNormal;
	//float3 P = hitPosition;
	//RayDesc ray;
	//ray.TMin = 0;
	//ray.TMax = 100000;
	//ShadowPayload shadowPayload;
	//LightResult totalResult = (LightResult) 0;
	//for (i = 0; i < LightPropertiesCB.NumPointLights; ++i)
	//{
	//	TraceRay(gRtScene, 0 /*rayFlags*/, 0xFF, 1 /* ray index*/, 2, 1, ray, shadowPayload);
	//	if (shadowPayload.hit)
	//	{
	//		LightResult result = DoPointLight(PointLights[i], V, P, N);

	//		totalResult.Diffuse += result.Diffuse;
	//		totalResult.Specular += result.Specular;
	//	}
			
	//}

	//for (i = 0; i < LightPropertiesCB.NumSpotLights; ++i)
	//{
	//	TraceRay(gRtScene, 0 /*rayFlags*/, 0xFF, 1 /* ray index*/, 2, 1, ray, shadowPayload);
	//	if (shadowPayload.hit)
	//	{
	//		LightResult result = DoSpotLight(SpotLights[i], V, P, N);

	//		totalResult.Diffuse += result.Diffuse;
	//		totalResult.Specular += result.Specular;
	//	}
	//}
	
	//float3 resultColor = hitAlbedo * totalResult.Diffuse.xyz + float3(1, 1, 1) * totalResult.Specular.xyz;
	
	//float3 col = LinearToSRGB(resultColor);
	//gOutput[launchIndex.xy] = float4(col, 1);
}

[shader("miss")]
void miss(inout RayPayload payload)
{
// TODO: skybox and GI
	payload.color = float3(0.4, 0.6, 0.2);
}

[shader("miss")]
void shadowMiss(inout ShadowPayload payload)
{
	payload.hit = false;
}


[shader("closesthit")]
void triangleChs(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
	//float hitT = RayTCurrent();
	//float3 rayDirW = WorldRayDirection();
	//float3 rayOriginW = WorldRayOrigin();

	//// Find the world-space hit position
	//float3 posW = rayOriginW + hitT * rayDirW;

	//// Fire a shadow ray. The direction is hard-coded here, but can be fetched from a constant-buffer
	//RayDesc ray;
	//ray.Origin = posW;
	//ray.Direction = normalize(float3(0.5, 0.5, -0.5));
	//ray.TMin = 0.01;
	//ray.TMax = 100000;
	//ShadowPayload shadowPayload;
	//TraceRay(gRtScene, 0 /*rayFlags*/, 0xFF, 1 /* ray index*/, 0, 1, ray, shadowPayload);

	//float factor = shadowPayload.hit ? 0.1 : 1.0;
	//payload.color = float4(0.9f, 0.9f, 0.9f, 1.0f) * factor;
	payload.color = float3(0, 0, 0);
}

[shader("closesthit")]
void shadowChs(inout ShadowPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
	payload.hit = true;
}


