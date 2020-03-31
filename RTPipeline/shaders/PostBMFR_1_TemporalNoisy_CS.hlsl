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
    { -14, -14}, {   4,  -6 },   {  -8,  14 },  {   8,   0 },
    { -10,  -8 }, {   2,  12 },  {  12, -12 }, { -10,   0 },
    {  12,  14 }, {  -8, -16 }, {   6,   6 },   {  -2,  -2 },
    {   6, -14 }, { -16,  12 }, {  14,  -4 },  {  -6,   4 }
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
    "DescriptorTable( SRV(t0, numDescriptors = 10)," \
								"UAV(u0, numDescriptors = 5) )," \
    "CBV(b0), "\
    "RootConstants(b1, num32BitConstants = 4)" 


Texture2D<float4> current_normals : register(t0);
Texture2D<float4> previous_normals : register(t1);
Texture2D<float4> current_positions : register(t2);
Texture2D<float4> previous_positions : register(t3);
Texture2D<float4> current_extra : register(t4);
Texture2D<float4> previous_extra : register(t5);

Texture2D<float4> previous_noisy : register(t6);
Texture2D<uint> previous_spp : register(t7);

Texture2D<float4> noisy_input : register(t8);

Texture2D<float4> current_metallic : register(t9);

RWTexture2D<float4> current_noisy : register(u0);
RWTexture2D<uint> current_spp : register(u1);

RWTexture2D<float2> out_prev_frame_pixel : register(u2);
RWTexture2D<uint> accept_bools : register(u3);

RWTexture3D<float> tmp_data : register(u4);

struct ViewProjectMatrix_prev
{
	matrix viewProject_prev;
};
ConstantBuffer<ViewProjectMatrix_prev> viewProjectMatrix_prev : register(b0);
struct FrameIndex
{
	uint FrameIndex;
	float3 Padding;
};
ConstantBuffer<FrameIndex> frameIndexCB : register(b1);


// clamp the biased workset into image dim
int mirror(int index, int size)
{
	if (index < 0)
		index = abs(index) - 1;
	else if (index >= size)
		index = 2 * size - index - 1;
	return index;
}
int2 mirror2(int2 index, int2 size)
{
	index.x = mirror(index.x, size.x);
	index.y = mirror(index.y, size.y);
	return index;
}
bool isReprojValid(int2 res, int2 curr_coord, int2 prev_coord)
{
    // reject if the pixel is outside the screen
	if (curr_coord.x < 0 || curr_coord.x >= res.x || curr_coord.y < 0 || curr_coord.y >= res.y)
		return false;
	if (prev_coord.x < 0 || prev_coord.x >= res.x || prev_coord.y < 0 || prev_coord.y >= res.y)
		return false;
    // reject the pixel is not hit / hit another geometry
	if (previous_positions.Load(int3(prev_coord.x, prev_coord.y, 0)).w == 0.0 ||
		abs(previous_extra.Load(int3(prev_coord.x, prev_coord.y, 0)).w - current_extra.Load(int3(curr_coord.x, curr_coord.y, 0)).w) >= 0.1)
		return false;
 //   // reject if the normal deviation is not acceptable
	if (distance(current_normals.Load(int3(curr_coord.x, curr_coord.y, 0)).xyz, previous_normals.Load(int3(prev_coord.x, prev_coord.y, 0)).xyz) > NORMAL_LIMIT_SQUARED)
		return false;
	return true;
}



[RootSignature(Post_RootSignature)]
[numthreads(LOCAL_WIDTH, LOCAL_HEIGHT, 1)]
void main(ComputeShaderInput IN)
{
	// parallel dimension = [ upper(w/pis)*pis+pis, upper(h/pis)*pis+pis ]
	// dimension parameters
	int2 gid = (int2) IN.DispatchThreadID.xy;
	float IMAGE_WIDTH;
	float IMAGE_HEIGHT;
	float mipLevels;
	current_positions.GetDimensions(0, IMAGE_WIDTH, IMAGE_HEIGHT, mipLevels);
	
	// bias the actual image into the workset_margin with a frameIndex-related offset
	const int2 pixel_without_mirror = gid - BLOCK_EDGE_HALF
      + BLOCK_OFFSETS[frameIndexCB.FrameIndex % BLOCK_OFFSETS_COUNT];
	const int2 pixel = mirror2(pixel_without_mirror, int2(IMAGE_WIDTH, IMAGE_HEIGHT));
	
	float4 world_position = current_positions.Load(int3(pixel, 0));
	float3 normal = current_normals.Load(int3(pixel, 0)).xyz;
	float3 metallic = current_metallic.Load(int3(pixel, 0)).w;
	float3 roughness = current_normals.Load(int3(pixel, 0)).w;
	float3 current_color = noisy_input.Load(int3(pixel, 0)).xyz;
	
	// Default previous frame pixel is the same pixel
	float2 prev_frame_pixel_f = float2(pixel);

    // This is changed to non zero if previous frame is not discarded completely
	uint store_accept = 0x00;

    // Blend_alpha 1.f means that only current frame color is used.
	float blend_alpha = 1.0f;
	float3 previous_color = 0.0f;
	float sample_spp = 0.0f;
	float total_weight = 0.0f;
	float weights[4] = { 0.0, 0.0, 0.0, 0.0 };
	int2 prev_frame_pixel = 0;
	
	if (previous_spp.Load(int3(pixel, 0)) > 0)
	{
		if (pixel_without_mirror.x >= 0 && pixel_without_mirror.x < IMAGE_WIDTH &&
                pixel_without_mirror.y >= 0 && pixel_without_mirror.y < IMAGE_HEIGHT)
		{
			int tmp_spp = previous_spp.Load(int3(pixel, 0));
			current_spp[pixel] = tmp_spp + 1;
			current_noisy[pixel] = float4((previous_noisy.Load(int3(pixel, 0)).xyz * tmp_spp + noisy_input.Load(int3(pixel, 0)).xyz) / (tmp_spp + 1),0);
		}
	}
	else
	{
		if (pixel_without_mirror.x >= 0 && pixel_without_mirror.x < IMAGE_WIDTH &&
                pixel_without_mirror.y >= 0 && pixel_without_mirror.y < IMAGE_HEIGHT)
		{
			current_spp[pixel] = 1;
			current_noisy[pixel] = noisy_input.Load(int3(pixel, 0)).xyzw;
		}
	}
}