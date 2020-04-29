#define BLOCK_SIZE 16

struct ComputeShaderInput
{
    uint3 GroupID           : SV_GroupID;           // 3D index of the thread group in the dispatch.
    uint3 GroupThreadID     : SV_GroupThreadID;     // 3D index of local thread ID in a thread group.
    uint3 DispatchThreadID  : SV_DispatchThreadID;  // 3D index of global thread ID in the dispatch.
    uint  GroupIndex        : SV_GroupIndex;        // Flattened local index of the thread within a thread group.
};

struct CubeMapToIrradianceMap
{
    uint CubemapSize;
	uint padding[3];
};

ConstantBuffer<CubeMapToIrradianceMap> CubeMapToIrradianceMapCB : register(b0);

TextureCube<float4> SkyboxCubaMap : register(t0);

RWTexture2DArray<float4> DstMip : register(u0);

// Linear repeat sampler.
SamplerState LinearRepeatSampler : register(s0);

#define GenerateMips_RootSignature \
    "RootFlags(0), " \
    "RootConstants(b0, num32BitConstants = 1), " \
    "DescriptorTable( SRV(t0, numDescriptors = 1) )," \
    "DescriptorTable( UAV(u0, numDescriptors = 1) )," \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_WRAP," \
        "addressV = TEXTURE_ADDRESS_WRAP," \
        "addressW = TEXTURE_ADDRESS_WRAP," \
        "filter = FILTER_MIN_MAG_LINEAR_MIP_POINT )"


// 1 / PI
static const float PI = 3.1415926535897;
static const float InvPI = 0.31830988618379067153776752674503f;

[RootSignature(GenerateMips_RootSignature)]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void main( ComputeShaderInput IN )
{
    // Cubemap texture coords.
    uint3 texCoord = IN.DispatchThreadID;

    // First check if the thread is in the cubemap dimensions.
	if (texCoord.x >= CubeMapToIrradianceMapCB.CubemapSize || texCoord.y >= CubeMapToIrradianceMapCB.CubemapSize)
		return;

    // Compute the worldspace direction vector from the dispatch thread ID.
	float3 dir = float3(texCoord.xy / float(CubeMapToIrradianceMapCB.CubemapSize) - 0.5f, 0.0f);

    // The Z component of the texture coordinate represents the cubemap face that is being generated.
    switch (texCoord.z)
    {
        // +X
        case 0:
            dir = normalize(float3(0.5f, -dir.y, -dir.x));
            break;
        // -X
        case 1:
            dir = normalize(float3(-0.5f, -dir.y, dir.x));
            break;
        // +Y
        case 2:
            dir = normalize(float3(dir.x, 0.5f, dir.y));
            break;
        // -Y
        case 3:
            dir = normalize(float3(dir.x, -0.5f, -dir.y));
            break;
        // +Z
        case 4:
            dir = normalize(float3(dir.x, -dir.y, 0.5f));
            break;
        // -Z
        case 5:
            dir = normalize(float3(-dir.x, -dir.y, -0.5f));
            break;
    }
	float3 normal = normalize(dir);
	float3 irradiance = float3(0.0,0.0,0.0);

	float3 up = float3(0.0, 1.0, 0.0);
	float3 right = cross(up, normal);
	up = cross(normal, right);

	float sampleDelta = PI/50.0f;
	float nrSamples = 0.0f;
	for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
	{
		for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
		{
            // spherical to cartesian (in tangent space)
			float3 tangentSample = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            // tangent space to world
			float3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;
			uint status;
			float3 sample = float3(
            SkyboxCubaMap.GatherRed(LinearRepeatSampler, sampleVec, status).x,
            SkyboxCubaMap.GatherGreen(LinearRepeatSampler, sampleVec, status).x,
            SkyboxCubaMap.GatherBlue(LinearRepeatSampler, sampleVec, status).x);
            
			irradiance += sample * cos(theta) * sin(theta);
			nrSamples++;
		}
	}
	irradiance = PI * irradiance * (1.0 / float(nrSamples));
	
	DstMip[texCoord] = float4(irradiance, 1.0);
}