Texture2D<float4> gPosition : register(t0); //srv
Texture2D<float4> gAlbedoMetallic : register(t1); //srv
Texture2D<float4> gNormalRoughness : register(t2); //srv
Texture2D<float4> gExtra : register(t3); //srv

Texture2D<float4> color_in : register(t4); // only x
Texture2D<float4> variance_in : register(t5); // only x
RWTexture2D<float4> color_out : register(u0); // only x
RWTexture2D<float4> variance_out : register(u1);// only x

struct LevelLast
{
	uint level;
	float3 padding;
};
ConstantBuffer<LevelLast> LevelLastCB : register(b0);

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

void ATrousFilter(float x, float y, float2 res, int level,
                              float sigma_c, float sigma_n, float sigma_x, bool blur_variance)
{
    // 5x5 A-Trous kernel
	float h[25] =
	{
		1.0 / 256.0, 1.0 / 64.0, 3.0 / 128.0, 1.0 / 64.0, 1.0 / 256.0,
        1.0 / 64.0, 1.0 / 16.0, 3.0 / 32.0, 1.0 / 16.0, 1.0 / 64.0,
        3.0 / 128.0, 3.0 / 32.0, 9.0 / 64.0, 3.0 / 32.0, 3.0 / 128.0,
        1.0 / 64.0, 1.0 / 16.0, 3.0 / 32.0, 1.0 / 16.0, 1.0 / 64.0,
        1.0 / 256.0, 1.0 / 64.0, 3.0 / 128.0, 1.0 / 64.0, 1.0 / 256.0
	};
    // 3x3 Gaussian kernel
	float gaussian[9] =
	{
		1.0 / 16.0, 1.0 / 8.0, 1.0 / 16.0,
        1.0 / 8.0, 1.0 / 4.0, 1.0 / 8.0,
        1.0 / 16.0, 1.0 / 8.0, 1.0 / 16.0
	};

	if (gPosition.Load(int3(x,y,0)).w == 0) {
		color_out[int2(x,y)] = color_in.Load(int3(x, y, 0));
		variance_out[int2(x, y)] = variance_in.Load(int3(x, y, 0));
	}
	else if (x < res.x && y < res.y) {
		int step = 1 << level;
		
		float var;
        // perform 3x3 gaussian blur on variance
		if (blur_variance) {
			float sum = 0.0f;
			float sumw = 0.0f;
			int2 g[9] = {
				int2(-1, -1), int2(0, -1), int2(1, -1),
                int2(-1, 0), int2(0, 0), int2(1, 0),
                int2(-1, 1), int2(0, 1), int2(1, 1)
			};
			for (int sampleIdx = 0; sampleIdx < 9; sampleIdx++) {
				int2 loc = int2(x, y) + g[sampleIdx];
				if (loc.x >= 0 && loc.y >= 0 && loc.x < res.x && loc.y < res.y)
				{
					sum += gaussian[sampleIdx] * variance_in.Load(int3(loc.x, loc.y, 0)).x;
					sumw += gaussian[sampleIdx];
				}
			}
			var = max(sum / sumw, 0.0f);
		}
		else {
			var = max(variance_in.Load(int3(x, y, 0)).x, 0.0f);
		}
        
        // Load pixel p data
		int3 p = int3(x, y, 0);
		float lp = color_in.Load(p).x;
		float3 np = gNormalRoughness.Load(p).xyz;
		float3 pp = gPosition.Load(p).xyz;

		float color_sum = 0.0f;
		float variance_sum = 0.0f;
		float weights_sum = 0;
		float weights_squared_sum = 0;

		for (int i = -2; i <= 2; i++) {
			for (int j = -2; j <= 2; j++) {
				int xq = x + step * i;
				int yq = y + step * j;
				if (xq >= 0 && xq < res.x && yq >= 0 && yq < res.y)
				{
                    // Load pixel q data
					int3 q = int3(xq, yq, 0);
					float lq = color_in.Load(q).x;
					float3 nq = gNormalRoughness.Load(q).xyz;
					float3 pq = gPosition.Load(q).xyz;
					float wq = variance_in.Load(q).x;
                    
                    // Edge-stopping weights
					float wl = exp(-distance(lp, lq) / (sqrt(var) * sigma_c + 1e-6));
					float wn = min(1.0f, exp(-distance(np, nq) / (sigma_n + 1e-6)));
					float wx = min(1.0f, exp(-distance(pp, pq) / (sigma_x + 1e-6)));

                    // filter weights
					int k = (2 + i) + (2 + j) * 5;
					float weight = h[k] * wl * wn * wx;
					weights_sum += weight;
					weights_squared_sum += weight * weight;
					color_sum += (lq * weight);
					variance_sum += (wq * weight * weight);
				}
			}
		}
        // update color and variance
		if (weights_sum > 10e-6)
		{
			color_out[p.xy] = float4(color_sum / weights_sum, 0, 0, 0);
			variance_out[p.xy] = float4(variance_sum / weights_squared_sum, 0, 0, 0);
		}
		else
		{
			color_out[p.xy] = float4(color_sum / weights_sum, 0, 0, 0);
			variance_out[p.xy] = variance_in.Load(p);
		}
	}
}

float4 main(float4 Position : SV_Position) : SV_TARGET0
{
	float sigma_c = 0.7;
	float sigma_n = 0.2;
	float sigma_x = 0.35;
	float varGauss_blur = true;
	
	int2 texCoord = (int2) Position.xy;
	float resx;
	float resy;
	float mipLevels;
	gPosition.GetDimensions(0, resx, resy, mipLevels);
	
	ATrousFilter(texCoord.x, texCoord.y, float2(resx, resy), LevelLastCB.level, sigma_c, sigma_n, sigma_x, varGauss_blur);
	return float4(0,0,0, 1.0f);
}