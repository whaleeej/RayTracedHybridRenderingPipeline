// temporal define
#define NORMAL_LIMIT_SQUARED 0.1f
#define BLEND_ALPHA 0.1f
// parallel workset define
#define LOCAL_WIDTH 8
#define LOCAL_HEIGHT 8
#define BLOCK_EDGE_LENGTH 32
#define BLOCK_PIXELS BLOCK_EDGE_LENGTH*BLOCK_EDGE_LENGTH
#define BLOCK_EDGE_HALF (BLOCK_EDGE_LENGTH / 2)
#define WORKSET_WIDTH (BLOCK_EDGE_LENGTH * ((IMAGE_WIDTH + BLOCK_EDGE_LENGTH - 1) / BLOCK_EDGE_LENGTH))
#define WORKSET_HEIGHT (BLOCK_EDGE_LENGTH *  ((IMAGE_HEIGHT + BLOCK_EDGE_LENGTH - 1) / BLOCK_EDGE_LENGTH))
#define WORKSET_WITH_MARGINS_WIDTH (WORKSET_WIDTH + BLOCK_EDGE_LENGTH)
#define WORKSET_WITH_MARGINS_HEIGHT (WORKSET_HEIGHT + BLOCK_EDGE_LENGTH)
#define OUTPUT_SIZE (WORKSET_WIDTH * WORKSET_HEIGHT)
#define LOCAL_SIZE 256
#define FITTER_GLOBAL (LOCAL_SIZE * ((WORKSET_WITH_MARGINS_WIDTH / BLOCK_EDGE_LENGTH) * (WORKSET_WITH_MARGINS_HEIGHT / BLOCK_EDGE_LENGTH)))
// feature buffer define
#define BUFFER_COUNT 15
#define FEATURES_NOT_SCALED 6
#define FEATURES_SCALED 6
#define NOT_SCALED_FEATURE_BUFFERS \
1.f,\
normal.x,\
normal.y,\
normal.z,\
metallic.x,\
roughness.x,
#define SCALED_FEATURE_BUFFERS \
world_position.x,\
world_position.y,\
world_position.z,\
world_position.x*world_position.x,\
world_position.y*world_position.y,\
world_position.z*world_position.z
#define FEATURE_BUFFERS NOT_SCALED_FEATURE_BUFFERS SCALED_FEATURE_BUFFERS
// block offset define
#define BLOCK_OFFSETS_COUNT 16
static const int2 BLOCK_OFFSETS[BLOCK_OFFSETS_COUNT] =
{
	{ -14, -14 },
	{ 4, -6 },
	{ -8, 14 },
	{ 8, 0 },
	{ -10, -8 },
	{ 2, 12 },
	{ 12, -12 },
	{ -10, 0 },
	{ 12, 14 },
	{ -8, -16 },
	{ 6, 6 },
	{ -2, -2 },
	{ 6, -14 },
	{ -16, 12 },
	{ 14, -4 },
	{ -6, 4 }
};

struct ComputeShaderInput
{
	uint3 GroupID : SV_GroupID; // 3D index of the thread group in the dispatch.
	uint3 GroupThreadID : SV_GroupThreadID; // 3D index of local thread ID in a thread group.
	uint3 DispatchThreadID : SV_DispatchThreadID; // 3D index of global thread ID in the dispatch.
	uint GroupIndex : SV_GroupIndex; // Flattened local index of the thread within a thread group.
};

#define Post_RootSignature \
    "RootFlags(0), " \
    "DescriptorTable( SRV(t0, numDescriptors = 5)," \
								"UAV(u0, numDescriptors = 1) )," \
    "RootConstants(b0, num32BitConstants = 4)" 


Texture2D<float4> filtered_frame : register(t0);
Texture2D<float2> in_prev_frame_pixel : register(t1);
Texture2D<uint> accept_bools : register(t2);
Texture2D<uint> current_spp : register(t3);
Texture2D<float4> accumulated_prev_frame : register(t4);

RWTexture2D<float4> accumulated_frame : register(u0);

struct FrameIndex
{
	uint FrameIndex;
	float3 Padding;
};
ConstantBuffer<FrameIndex> frameIndexCB : register(b0);


[RootSignature(Post_RootSignature)]
[numthreads(LOCAL_WIDTH, LOCAL_HEIGHT, 1)]
void main(ComputeShaderInput IN)
{
	// dimension parameters
	int2 gid = (int2) IN.DispatchThreadID.xy;
	float IMAGE_WIDTH;
	float IMAGE_HEIGHT;
	float mipLevels;
	accept_bools.GetDimensions(0, IMAGE_WIDTH, IMAGE_HEIGHT, mipLevels);
	
	const int2 pixel = gid;
	if (pixel.x >= IMAGE_WIDTH || pixel.y >= IMAGE_HEIGHT)
		return;
	
	float3 filtered_color = filtered_frame.Load(int3(pixel, 0));
	float3 prev_color = float3(0.f, 0.f, 0.f);
	float blend_alpha = 1.f;
	
	if (frameIndexCB.FrameIndex > 0)
	{
		uint accept = accept_bools.Load(int3(pixel, 0));
		float2 prev_frame_pixel_f = in_prev_frame_pixel.Load(int3(pixel, 0));
		int2 prev_frame_pixel = int2(floor(prev_frame_pixel_f.x), floor(prev_frame_pixel_f.y));
		
		// These are needed for  the bilinear sampling
		int2 offsets[4];
		offsets[0] = int2 (0, 0);
		offsets[1] = int2 (1, 0);
		offsets[2] = int2 (0, 1);
		offsets[3] = int2 (1, 1);
		float2 prev_pixel_fract = prev_frame_pixel_f - float2(prev_frame_pixel);
		float2 one_minus_prev_pixel_fract = 1.f - prev_pixel_fract;
		float weights[4];
		weights[0] = one_minus_prev_pixel_fract.x * one_minus_prev_pixel_fract.y;
		weights[1] = prev_pixel_fract.x * one_minus_prev_pixel_fract.y;
		weights[2] = one_minus_prev_pixel_fract.x * prev_pixel_fract.y;
		weights[3] = prev_pixel_fract.x * prev_pixel_fract.y;
		float total_weight = 0.0f;
		// to bilinear sample
		if (accept & 0x01)
		{
			float weight = weights[0];
			total_weight += weight;
			prev_color += weight * accumulated_prev_frame.Load(int3(prev_frame_pixel+offsets[0], 0)).xyz;
		}
		if (accept & 0x02)
		{
			float weight = weights[1];
			total_weight += weight;
			prev_color += weight * accumulated_prev_frame.Load(int3(prev_frame_pixel + offsets[1], 0)).xyz;
		}
		if (accept & 0x04)
		{
			float weight = weights[2];
			total_weight += weight;
			prev_color += weight * accumulated_prev_frame.Load(int3(prev_frame_pixel + offsets[2], 0)).xyz;
		}
		if (accept & 0x08)
		{
			float weight = weights[3];
			total_weight += weight;
			prev_color += weight * accumulated_prev_frame.Load(int3(prev_frame_pixel + offsets[3], 0)).xyz;
		}
		if (total_weight > 0.00)
		{
			// Blend_alpha is dymically decided so that the result is average
			 // of all samples until the cap defined by BLEND_ALPHA is reached
			blend_alpha = 1.f / (current_spp.Load(int3(pixel,0)) + 1.f);
			blend_alpha = max(blend_alpha, BLEND_ALPHA);
		}
	}
	float3 accumulated_color = blend_alpha * filtered_color + (1.f - blend_alpha) * prev_color;
	accumulated_frame[pixel] = float4(accumulated_color, 1);
}