Texture2D<float4> gPosition : register(t0); //srv
Texture2D<float4> gAlbedoMetallic : register(t1); //srv
Texture2D<float4> gNormalRoughness : register(t2); //srv
Texture2D<float4> gExtra : register(t3); //srv

RWTexture2D<float4> col_acc : register(u0); //uav // used x
RWTexture2D<float4> moment_acc : register(u1); //uav // used xy
RWTexture2D<float4> his_length : register(u2); //uav // used x

Texture2D<float4> gPosition_prev : register(t4); //srv
Texture2D<float4> gAlbedoMetallic_prev : register(t5); //srv
Texture2D<float4> gNormalRoughness_prev : register(t6); //srv
Texture2D<float4> gExtra_prev : register(t7); //srv
Texture2D<float4> col_acc_prev : register(t8); //srv // used x
Texture2D<float4> moment_acc_prev : register(t9); //srv // used xy
Texture2D<float4> his_length_prev : register(t10); //srv // used x

Texture2D<float4> rt_input : register(t11);

struct ViewProjectMatrix_prev
{
	matrix viewProject_prev;
};
ConstantBuffer<ViewProjectMatrix_prev> viewProjectMatrix_prev : register(b0);

float3 LinearToSRGB(float3 x)
{
    // This is exactly the sRGB curve
	return x < 0.0031308 ? 12.92 * x : 1.055 * pow(abs(x), 1.0 / 2.4) - 0.055;

    // This is cheaper but nearly equivalent
	//return x < 0.0031308 ? 12.92 * x : 1.13005 * sqrt(abs(x - 0.00228)) - 0.13448 * x + 0.005719;
}

float3 simpleToneMapping(float3 c)
{
	return float3(c.x / (c.x + 1.0f), c.y / (c.y + 1.0f), c.z / (c.z + 1.0f));
}

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

float3 backProject(float x, float y, float2 res, float col_alpha, float mom_alpha)
{
	int N = his_length_prev.Load(int3(x, y, 0)).x;
	float sample = rt_input.Load(int3(x, y, 0)).x;
	
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
	
		float prevColor = 0.0f;
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
					prevColor += w[sampleIdx] * col_acc_prev.Load(int3(loc, 0)).x;
					prevMoments += w[sampleIdx] * moment_acc_prev.Load(int3(loc, 0)).xy;
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
						prevColor += col_acc_prev.Load(int3(loc, 0)).x;
						prevMoments += moment_acc_prev.Load(int3(loc, 0)).xy;
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
			float moment_alpha = max(1.0f / (float) (N + 1.0), mom_alpha);
			// incresase history length
			his_length[int2(x, y)] = float4((int) prevHistoryLength + 1, 0, 0, 0);
			// color accumulation 
			col_acc[int2(x, y)] = float4(sample * color_alpha + prevColor * (1.0f - color_alpha), 0, 0, 0);
			// moment accumulation
			float first_moment = moment_alpha * prevMoments.x + (1.0f - moment_alpha) * sample;
			float second_moment = moment_alpha * prevMoments.y + (1.0f - moment_alpha) * sample * sample;
			moment_acc[int2(x, y)] = float4(first_moment, second_moment, 0, 0);

			// calculate variance from moments
			float variance = second_moment - first_moment * first_moment;
			return variance > 0.0f ? variance : 0.0f;
		}
	}
	
	his_length[int2(x, y)] = float4(1.0, 0.0, 0.0, 0.0);
	col_acc[int2(x, y)] = float4(sample, 0, 0, 0);
	moment_acc[int2(x, y)] = float4(sample, sample * sample, 0.0, 0.0);
	return 100.0f;
}


float4 main( float4 Position : SV_Position ) : SV_Target0
{
	float col_alpha = 0.1;
	float mom_alpha = 0.1;
	
    int2 texCoord = ( int2 )Position.xy;
	float resx;
	float resy;
	float mipLevels;
	gPosition.GetDimensions(0, resx, resy, mipLevels);
	
	float variance = backProject(texCoord.x, texCoord.y, float2(resx, resy), col_alpha, mom_alpha);
	return float4(col_acc.Load(int3(texCoord, 0)).x, 0, 0, 1.0);
}

// testing
//return float4(moment_acc.Load(int3(texCoord, 0)).xyz / 1.0, 1.0);
//return float4(rt_input.Load(int3(texCoord.x, texCoord.y, 0)).xyz, 1);
//return float4(color.x / 10.0f, color.y / 10.0f, color.z / 10.0f, 1); // position testing
//return float4(color.r, color.r, color.r, 1); // metallic and roughness testing
//return float4(((color) + 1.0f) / 2.0f, 1); // normal testing from [-1 1] -> [0 1]
//return float4(normalize(color), 1); // normal testing //  [0 1] space
//return float4((color), 1); // albedo testing
//return float4(LinearToSRGB(simpleToneMapping(color)), 1); // albedo testing