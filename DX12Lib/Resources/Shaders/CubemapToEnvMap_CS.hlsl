#define BLOCK_SIZE 16

struct ComputeShaderInput
{
    uint3 GroupID           : SV_GroupID;           // 3D index of the thread group in the dispatch.
    uint3 GroupThreadID     : SV_GroupThreadID;     // 3D index of local thread ID in a thread group.
    uint3 DispatchThreadID  : SV_DispatchThreadID;  // 3D index of global thread ID in the dispatch.
    uint  GroupIndex        : SV_GroupIndex;        // Flattened local index of the thread within a thread group.
};

struct CubeMapToEnvMap
{
    uint CubemapSize;
	uint NumMips;
	uint padding[2];
};

ConstantBuffer<CubeMapToEnvMap> CubeMapToEnvMapCB : register(b0);

TextureCube<float4> SkyboxCubaMap : register(t0);

RWTexture2DArray<float4> DstMip1 : register(u0);
RWTexture2DArray<float4> DstMip2 : register(u1);
RWTexture2DArray<float4> DstMip3 : register(u2);
RWTexture2DArray<float4> DstMip4 : register(u3);
RWTexture2DArray<float4> DstMip5 : register(u4);

// Linear repeat sampler.
SamplerState LinearRepeatSampler : register(s0);

#define GenerateMips_RootSignature \
    "RootFlags(0), " \
    "RootConstants(b0, num32BitConstants = 1), " \
    "DescriptorTable( SRV(t0, numDescriptors = 1) )," \
    "DescriptorTable( UAV(u0, numDescriptors = 5) )," \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_WRAP," \
        "addressV = TEXTURE_ADDRESS_WRAP," \
        "addressW = TEXTURE_ADDRESS_WRAP," \
        "filter = FILTER_MIN_MAG_LINEAR_MIP_POINT )"


// 1 / PI
static const float PI = 3.1415926535897;
static const float InvPI = 0.31830988618379067153776752674503f;

float RadicalInverse_VdC(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
// ----------------------------------------------------------------------------
float2 Hammersley(uint i, uint N)
{
	return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

float VanDerCorpus(uint n, uint base)
{
	float invBase = 1.0 / float(base);
	float denom = 1.0;
	float result = 0.0;

	for (uint i = 0u; i < 32u; ++i)
	{
		if (n > 0u)
		{
			denom = float(n%2);
			result += denom * invBase;
			invBase = invBase / 2.0;
			n = uint(float(n) / 2.0);
		}
	}

	return result;
}
// ----------------------------------------------------------------------------
float2 HammersleyNoBitOps(uint i, uint N)
{
	return float2(float(i) / float(N), VanDerCorpus(i, 2u));
}
float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
	float a = roughness * roughness;
	
	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	
    // from spherical coordinates to cartesian coordinates
	float3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;
	
    // from tangent-space vector to world-space sample vector
	float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
	float3 tangent = normalize(cross(up, N));
	float3 bitangent = cross(N, tangent);
	
	float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}


[RootSignature(GenerateMips_RootSignature)]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void main( ComputeShaderInput IN )
{
    // Cubemap texture coords.
    uint3 texCoord = IN.DispatchThreadID;

    // First check if the thread is in the cubemap dimensions.
	if (texCoord.x >= CubeMapToEnvMapCB.CubemapSize || texCoord.y >= CubeMapToEnvMapCB.CubemapSize)
		return;

    // Compute the worldspace direction vector from the dispatch thread ID.
	float3 dir = float3(texCoord.xy / float(CubeMapToEnvMapCB.CubemapSize) - 0.5f, 0.0f);

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
	float3 N = normalize(normal);
	float3 R = N;
	float3 V = R;

	int bitsets[5] = {0x00, 0x11,0x33,0x77,0xFF };
	int divsets[5] = { 1,2,4,8,16};
	for (uint mip = 0; mip < min(CubeMapToEnvMapCB.NumMips,5); mip++)
	{
		if ((IN.GroupIndex & bitsets[mip]) == 0)
		{
			const uint SAMPLE_COUNT = 1024u;
			float totalWeight = 0.0;
			float3 prefilteredColor = float3(0.0, 0.0, 0.0);
			for (uint i = 0u; i < SAMPLE_COUNT; ++i)
			{
				float2 Xi = Hammersley(i, SAMPLE_COUNT);
				float3 H = ImportanceSampleGGX(Xi, N, mip * 1.0 / ((float) (CubeMapToEnvMapCB.NumMips - 1)));
				float3 L = normalize(2.0 * dot(V, H) * H - V);

				float NdotL = max(dot(N, L), 0.0);
				if (NdotL > 0.0)
				{
					uint status;
					float3 sample = float3(
				 SkyboxCubaMap.GatherRed(LinearRepeatSampler, L, status).x,
				 SkyboxCubaMap.GatherGreen(LinearRepeatSampler, L, status).x,
				 SkyboxCubaMap.GatherBlue(LinearRepeatSampler, L, status).x);
					prefilteredColor += sample * NdotL;
					totalWeight += NdotL;
				}
			}
			prefilteredColor = prefilteredColor / totalWeight;
		
			if (mip == 0)
				DstMip1[uint3(texCoord.xy / divsets[mip], texCoord.z)] = float4(prefilteredColor, 1.0);
			else if (mip == 1)
				DstMip2[uint3(texCoord.xy / divsets[mip], texCoord.z)] = float4(prefilteredColor, 1.0);
			else if (mip == 2)
				DstMip3[uint3(texCoord.xy / divsets[mip], texCoord.z)] = float4(prefilteredColor, 1.0);
			else if (mip == 3)
				DstMip4[uint3(texCoord.xy / divsets[mip], texCoord.z)] = float4(prefilteredColor, 1.0);
			else if (mip == 4)
				DstMip5[uint3(texCoord.xy / divsets[mip], texCoord.z)] = float4(prefilteredColor, 1.0);
		}
	}
}