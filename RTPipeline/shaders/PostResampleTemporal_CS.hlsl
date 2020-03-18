#define BLOCK_SIZE_X 8
#define BLOCK_SIZE_Y 8

struct ComputeShaderInput
{
	uint3 GroupID : SV_GroupID; // 3D index of the thread group in the dispatch.
	uint3 GroupThreadID : SV_GroupThreadID; // 3D index of local thread ID in a thread group.
	uint3 DispatchThreadID : SV_DispatchThreadID; // 3D index of global thread ID in the dispatch.
	uint GroupIndex : SV_GroupIndex; // Flattened local index of the thread within a thread group.
};

#define PostResampleTemporal_RootSignature \
    "RootFlags(0), " \
    "DescriptorTable( SRV(t0, numDescriptors = 11)," \
								"UAV(u0, numDescriptors = 2) )," \
    "CBV(b0)" 

RWTexture2D<float4> radiance_acc : register(u0); //uav // used x
RWTexture2D<float4> his_length : register(u1); //uav // used x

Texture2D<float4> gPosition : register(t0); //srv
Texture2D<float4> gAlbedoMetallic : register(t1); //srv
Texture2D<float4> gNormalRoughness : register(t2); //srv
Texture2D<float4> gExtra : register(t3); //srv

Texture2D<float4> gPosition_prev : register(t4); //srv
Texture2D<float4> gAlbedoMetallic_prev : register(t5); //srv
Texture2D<float4> gNormalRoughness_prev : register(t6); //srv
Texture2D<float4> gExtra_prev : register(t7); //srv

Texture2D<float4> radiance_acc_prev : register(t8); //srv // used x
Texture2D<float4> his_length_prev : register(t9); //srv // used x

Texture2D<float4> radiance_input : register(t10);

struct ViewProjectMatrix_prev
{
	matrix viewProject_prev;
};
ConstantBuffer<ViewProjectMatrix_prev> viewProjectMatrix_prev : register(b0);

// from paper svgf paper 2017
bool isReprojValid(float2 res, float2 curr_coord, float2 prev_coord)
{
    // reject if the pixel is outside the screen
	if (prev_coord.x < 0 || prev_coord.x >= res.x || prev_coord.y < 0 || prev_coord.y >= res.y)
		return false;
    // reject the pixel is not hit / hit another geometry
	if (gPosition_prev.Load(int3(prev_coord.x, prev_coord.y, 0)).w == 0.0 || 
		(gExtra_prev.Load(int3(prev_coord.x, prev_coord.y, 0)).w - gExtra.Load(int3(curr_coord.x, curr_coord.y, 0)).w) >= 0.1)
		return false;
    // reject if the normal deviation is not acceptable
	if (distance(gNormalRoughness.Load(int3(curr_coord.x, curr_coord.y, 0)).xyz, gNormalRoughness_prev.Load(int3(prev_coord.x, prev_coord.y, 0)).xyz) > 0.1)
		return false;
	return true;
}

float3 backProject(float x, float y, float2 res, float col_alpha)
{
	int N = his_length_prev.Load(int3(x, y, 0)).x;
	float3 sample = radiance_input.Load(int3(x, y, 0)).xyz;
	float luminance = 0.2126 * sample.x + 0.7152 * sample.y + 0.0722 * sample.z;
	
	if (N > 0 && gPosition.Load(int3(x, y, 0)).w != 0.0)
	{
		float4 clip_position = mul(viewProjectMatrix_prev.viewProject_prev, float4(gPosition.Load(int3(x, y, 0)).xyz, 1.0f));
		float ndcx = clip_position.x / clip_position.w * 0.5 + 0.5;
		float ndcy = -clip_position.y / clip_position.w * 0.5 + 0.5;
		float prevx = ndcx * res.x - 0.5;
		float prevy = ndcy * res.y - 0.5;
	
		//  | ，  ， |  
		//  | ，  ， |
		bool v[4];
		float floorx = floor(prevx);
		float floory = floor(prevy);
		float fracx = prevx - floorx;
		float fracy = prevy - floory;
		bool valid = (floorx >= 0 && floory >= 0 && floorx < res.x && floory < res.y);
	
		// bilinear filtering
		float2 offset[4] = { float2(0, 0), float2(1, 0), float2(0, 1), float2(1, 1) };
		// check 4 sampling validation
		for (int sampleIdx = 0; sampleIdx < 4; sampleIdx++)
		{
			float2 loc = float2(floorx, floory) + offset[sampleIdx];
			v[sampleIdx] = isReprojValid(res, float2(x, y), loc);
			valid = valid && v[sampleIdx];
		}
	
		float3 prevColor = 0.0f;
		float2 prevMoments = float2(0.0f, 0.0f);
		float prevHistoryLength = 0.0f;
		if (valid)
		{
			// interpolating four point
			float sumw = 0.0f;
			float w[4] =
			{
				(1 - fracx) * (1 - fracy),
            fracx * (1 - fracy),
            (1 - fracx) * fracy,
            fracx * fracy
			};

			for (int sampleIdx = 0; sampleIdx < 4; sampleIdx++)
			{
				float2 loc = float2(floorx, floory) + offset[sampleIdx];
				if (v[sampleIdx])
				{
					prevColor += w[sampleIdx] * radiance_acc_prev.Load(int3(loc, 0)).xyz;
					prevHistoryLength += w[sampleIdx] * his_length_prev.Load(int3(loc, 0)).x;
					sumw += w[sampleIdx];
				}
			}
			if (sumw >= 0.01)
			{
				prevColor /= sumw;
				prevMoments /= sumw;
				prevHistoryLength /= sumw;
				valid = true;
			}
		}
		// find suitable samples in a 3*3 kernel
		if (!valid)
		{
			float cnt = 0.0f;
			const int radius = 1;

			for (int yy = -radius; yy <= radius; yy++)
			{
				for (int xx = -radius; xx <= radius; xx++)
				{
					float2 loc = float2(floorx, floory) + float2(xx, yy);
					if (isReprojValid(res, float2(x, y), loc))
					{
						prevColor += radiance_acc_prev.Load(int3(loc, 0)).xyz;
						prevHistoryLength += his_length_prev.Load(int3(loc, 0)).x;
						cnt += 1.0f;
					}
				}
			}

			if (cnt > 0.0f)
			{
				prevColor /= cnt;
				prevMoments /= cnt;
				prevHistoryLength /= cnt;
				valid = true;
			}
		}
		// accumulate and varaince estimation
		if (valid)
		{
			// calculate alpha values that controls fade
			float color_alpha = max(1.0f / (float) (N + 1.0), col_alpha);
			// incresase history length
			his_length[int2(x, y)] = float4((int) prevHistoryLength + 1, 0, 0, 0);
			// color accumulation 
			radiance_acc[int2(x, y)] = float4(sample * color_alpha + prevColor * (1.0f - color_alpha), 1);
			return 0.0f;
		}
	}
	
	his_length[int2(x, y)] = float4(1.0, 0.0, 0.0, 0.0);
	radiance_acc[int2(x, y)] = float4(sample, 1);
	return 100.0f;
}

[RootSignature(PostResampleTemporal_RootSignature)]
[numthreads(BLOCK_SIZE_X, BLOCK_SIZE_Y, 1)]
void main(ComputeShaderInput IN)
{
	float col_alpha = 0.08;
	
	int2 texCoord = (int2) IN.DispatchThreadID.xy;
	float resx;
	float resy;
	float mipLevels;
	gPosition.GetDimensions(0, resx, resy, mipLevels);
	
	 backProject(texCoord.x, texCoord.y, float2(resx, resy), col_alpha);
}