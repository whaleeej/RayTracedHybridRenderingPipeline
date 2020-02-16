RaytracingAccelerationStructure gRtScene : register(t0);
RWTexture2D<float4> gOutput : register(u0);
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

struct RayPayload
{
    float3 color;
};

[shader("raygeneration")]
void rayGen()
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 launchDim = DispatchRaysDimensions();

    float2 crd = float2(launchIndex.xy);
    float2 dims = float2(launchDim.xy);

    float2 d = ((crd/dims) * 2.f - 1.f);
    float aspectRatio = dims.x / dims.y;
	
	float3 cameraPositionWS = CameraCB.PositionWS.xyz;
	float3 cameraDirectionWS = normalize(float3(d.x * aspectRatio, -d.y, 1 / tan(radians(CameraCB.fov / 2.0))));
	float3 viewDirectionWS = mul((float3x3) CameraCB.InverseViewMatrix, cameraDirectionWS);

    RayDesc ray;
	ray.Origin = cameraPositionWS;
	ray.Direction = viewDirectionWS;

    ray.TMin = 0;
    ray.TMax = 100000;

    RayPayload payload;
	TraceRay(gRtScene, 
		RAY_FLAG_CULL_BACK_FACING_TRIANGLES /*RayFlags*/, 0xFF /*InstanceInclusionMask*/,
		0 /* RayContributionToHitGroupIndex*/, 0 /*MultiplierForGeometryContributionToHitGroupIndex*/, 0 /*MissShaderIndex*/,
		ray, payload
	);
    float3 col =payload.color;
    gOutput[launchIndex.xy] = float4(col, 1);
}

[shader("miss")]
void miss(inout RayPayload payload)
{
    payload.color = float3(0.4, 0.6, 0.2);
}

[shader("closesthit")]
void chs(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);

    const float3 A = float3(1, 0, 0);
    const float3 B = float3(0, 1, 0);
    const float3 C = float3(0, 0, 1);

	payload.color = A * barycentrics.x + B * barycentrics.y + C * barycentrics.z;
	//payload.color = float3(1, 0, 0);
}
