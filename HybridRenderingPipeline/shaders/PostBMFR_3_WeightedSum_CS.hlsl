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
    "DescriptorTable( SRV(t0, numDescriptors = 6)," \
								"UAV(u0, numDescriptors = 1) )," \
    "RootConstants(b0, num32BitConstants = 4)" 


// (WORKSET_WITH_MARGINS_WIDTH/BLOCK_EDGE_LENGTH, WORKSET_WITH_MARGINS_HEIGHT/BLOCK_EDGE_LENGTH, BUFFER_COUNT-3)
Texture3D<float4> weights : register(t0);
// (WORKSET_WITH_MARGINS_WIDTH/BLOCK_EDGE_LENGTH, WORKSET_WITH_MARGINS_HEIGHT/BLOCK_EDGE_LENGTH, 6)
Texture3D<float4> mins_maxs : register(t1);
Texture2D<float4> current_normals : register(t2);
Texture2D<float4> current_positions : register(t3);
Texture2D<float4> current_noisy : register(t4);
Texture2D<float4> current_metallic : register(t5);
RWTexture2D<float4> output : register(u0);
struct FrameIndex
{
	uint FrameIndex;
	float3 Padding;
};
ConstantBuffer<FrameIndex> frameIndexCB : register(b0);

inline float scale(float value, float min, float max)
{
	if (abs(max - min) > 1.0f)
	{
		return (value - min) / (max - min);
	}
	return value - min;
}

[RootSignature(Post_RootSignature)]
[numthreads(LOCAL_WIDTH, LOCAL_HEIGHT, 1)]
void main(ComputeShaderInput IN)
{
	int2 gid = (int2) IN.DispatchThreadID.xy;
	float IMAGE_WIDTH;
	float IMAGE_HEIGHT;
	float mipLevels;
	current_positions.GetDimensions(0, IMAGE_WIDTH, IMAGE_HEIGHT, mipLevels);
	
	const int2 pixel = gid;
	// bias the actual image into the workset_margin with a frameIndex-related offset
	const int2 group_index = (gid + BLOCK_EDGE_HALF
      - BLOCK_OFFSETS[frameIndexCB.FrameIndex % BLOCK_OFFSETS_COUNT]) / int2(BLOCK_EDGE_LENGTH, BLOCK_EDGE_LENGTH);
	// Load feature buffers
	float3 world_position = current_positions.Load(int3(pixel,0));
	float3 normal = current_normals.Load(int3(pixel, 0))/2.0+0.5;
	float3 metallic = current_metallic.Load(int3(pixel, 0)).w;
	float3 roughness = current_normals.Load(int3(pixel, 0)).w;
	float features[BUFFER_COUNT - 3] =
	{
      FEATURE_BUFFERS
	};
	// Weighted sum of the feature buffers
	float3 color = (float3) (0.f, 0.f, 0.f);
	for (int feature_buffer = 0; feature_buffer < BUFFER_COUNT - 3; feature_buffer++)
	{
		float feature = features[feature_buffer];
		// Scale world position buffers
		if (feature_buffer >= FEATURES_NOT_SCALED){
			const int4 min_max_index = int4(group_index, feature_buffer - FEATURES_NOT_SCALED, 0);
			feature = scale(feature, mins_maxs.Load(min_max_index).x, mins_maxs.Load(min_max_index).y);
		}
		float3 weight = weights.Load(int4(group_index, feature_buffer, 0));
		color += weight * feature;
	}
	// Store output
	color = float3(
		color.x < 0.0f ? 0.0f : color.x,
		color.y < 0.0f ? 0.0f : color.y,
		color.z < 0.0f ? 0.0f : color.z
	);
	output[gid] = float4(color, 1.0f);
}