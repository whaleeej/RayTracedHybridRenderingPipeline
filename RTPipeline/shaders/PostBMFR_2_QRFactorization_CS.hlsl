#define NOISE_AMOUNT 1e-2
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
#define BUFFER_COUNT 13
#define FEATURES_NOT_SCALED 4
#define FEATURES_SCALED 6
#define NOT_SCALED_FEATURE_BUFFERS \
1.f,\
normal.x,\
normal.y,\
normal.z,
#define SCALED_FEATURE_BUFFERS \
world_position.x,\
world_position.y,\
world_position.z,\
world_position.x*world_position.x,\
world_position.y*world_position.y,\
world_position.z*world_position.z
#define FEATURE_BUFFERS NOT_SCALED_FEATURE_BUFFERS SCALED_FEATURE_BUFFERS
// locale define
#define INFINITY 1.0f/1.0f
#define HORIZONTAL_BLOCKS (WORKSET_WIDTH / BLOCK_EDGE_LENGTH)
#define BLOCK_INDEX_X (group_id % (HORIZONTAL_BLOCKS + 1))
#define BLOCK_INDEX_Y (group_id / (HORIZONTAL_BLOCKS + 1))
#define IN_BLOCK_INDEX_X  ((256*sub_vector+id)%BLOCK_EDGE_LENGTH)
#define IN_BLOCK_INDEX_Y  ((256*sub_vector+id)/BLOCK_EDGE_LENGTH)
#define IN_ACCESS int3(BLOCK_INDEX_X*BLOCK_EDGE_LENGTH+IN_BLOCK_INDEX_X,\
										BLOCK_INDEX_Y*BLOCK_EDGE_LENGTH+IN_BLOCK_INDEX_Y,\
										feature_buffer)
#define R_EDGE BUFFER_COUNT-2 
#define R_ACCESS (x * R_EDGE + y)
// Here - means unused value
// Note: "unused" values are still set to 0 so some operations can be done to
// every element in a row or column
//    0  1  2  3  4  5 x
// 0 00 01 02 03 04 05
// 1  - 11 12 13 14 15
// 2  -  - 22 23 24 25
// 3  -  -  - 33 34 35
// 4  -  -  -  - 44 45
// 5  -  -  -  -  - 55
// y
struct ComputeShaderInput
{
	uint3 GroupID : SV_GroupID; // 3D index of the thread group in the dispatch.
	uint3 GroupThreadID : SV_GroupThreadID; // 3D index of local thread ID in a thread group.
	uint3 DispatchThreadID : SV_DispatchThreadID; // 3D index of global thread ID in the dispatch.
	uint GroupIndex : SV_GroupIndex; // Flattened local index of the thread within a thread group.
};

#define Post_RootSignature \
    "RootFlags(0), " \
    "DescriptorTable( UAV(u0, numDescriptors = 3) )," \
    "RootConstants(b0, num32BitConstants = 4)" 


// (WORKSET_WITH_MARGINS_WIDTH/BLOCK_EDGE_LENGTH, WORKSET_WITH_MARGINS_HEIGHT/BLOCK_EDGE_LENGTH, BUFFER_COUNT-3)
RWTexture3D<float3> weights : register(u0); 
// (WORKSET_WITH_MARGINS_WIDTH/BLOCK_EDGE_LENGTH, WORKSET_WITH_MARGINS_HEIGHT/BLOCK_EDGE_LENGTH, 6)
RWTexture3D<float2> mins_maxs : register(u1);
// (WORKSET_WITH_MARGINS_WIDTH, WORKSET_WITH_MARGINS_HEIGH, BUFFER_COUNT)
RWTexture3D<float> tmp_data : register(u2);

struct FrameIndex
{
	uint FrameIndex;
	float3 Padding;
};
ConstantBuffer<FrameIndex> frameIndexCB : register(b0);


groupshared float sum_vec[LOCAL_SIZE];
groupshared float u_vec[BLOCK_PIXELS];
groupshared float3 r_mat[(BUFFER_COUNT - 2) * (BUFFER_COUNT - 2)];
groupshared float3 divider;

// Random generator from here http://asgerhoedt.dk/?p=323
static inline float random(unsigned int a)
{
	a = (a + 0x7ed55d16) + (a << 12);
	a = (a ^ 0xc761c23c) ^ (a >> 19);
	a = (a + 0x165667b1) + (a << 5);
	a = (a + 0xd3a2646c) ^ (a << 9);
	a = (a + 0xfd7046c5) + (a << 3);
	a = (a ^ 0xb55a4f09) ^ (a >> 16);

	return float(a) / float(0xffffffff);
}
static inline float add_random(
      const float value,
      const int id,
      const int sub_vector,
      const int feature_buffer,
      const int frame_number)
{
	return value + NOISE_AMOUNT * 2.f * (random(id + sub_vector * LOCAL_SIZE +
      feature_buffer * BLOCK_EDGE_LENGTH * BLOCK_EDGE_LENGTH +
      frame_number * BUFFER_COUNT * BLOCK_EDGE_LENGTH * BLOCK_EDGE_LENGTH) - 0.5f);
}
static inline float3 load_r_mat(
       inout float3 Rmat[(BUFFER_COUNT - 2) * (BUFFER_COUNT - 2)],
       const int x,
       const int y)
{
	return Rmat[R_ACCESS];
}

static inline void store_r_mat(
      inout float3 Rmat[(BUFFER_COUNT - 2) * (BUFFER_COUNT - 2)],
      const int x,
      const int y,
      const float3 value)
{
	Rmat[R_ACCESS] = value;
}
static inline void store_r_mat_broadcast(
      inout float3 Rmat[(BUFFER_COUNT - 2) * (BUFFER_COUNT - 2)],
      const int x,
      const int y,
      const float value)
{
	Rmat[R_ACCESS] = value;
}
static inline void store_r_mat_channel(
      inout float3 Rmat[(BUFFER_COUNT - 2) * (BUFFER_COUNT - 2)],
      const int x,
      const int y,
      const int channel,
      const float value)
{
	float3 tmp = Rmat[R_ACCESS];
	if (channel == 0)
		tmp.x = value;
	else if (channel == 1)
		tmp.y = value;
	else // channel == 2
		tmp.z = value;
	Rmat[R_ACCESS] = tmp;
}
static inline void parallel_reduction_sum(out float result, inout float sum_vec[LOCAL_SIZE], int id)
{
	if (id < 64)
		sum_vec[id] += sum_vec[id + 64] + sum_vec[id + 128] + sum_vec[id + 192];
	DeviceMemoryBarrierWithGroupSync();
	if (id < 8)
		sum_vec[id] += sum_vec[id + 8] + sum_vec[id + 16] + sum_vec[id + 24] +
         sum_vec[id + 32] + sum_vec[id + 40] + sum_vec[id + 48] + sum_vec[id + 56];
	DeviceMemoryBarrierWithGroupSync();
	if (id == 0)
	{
		result = sum_vec[0] + sum_vec[1] + sum_vec[2] + sum_vec[3] +
         sum_vec[4] + sum_vec[5] + sum_vec[6] + sum_vec[7];
	}
	DeviceMemoryBarrierWithGroupSync();
}
static inline void parallel_reduction_min(out float result, inout float sum_vec[LOCAL_SIZE], int id)
{
	if(id < 64)
      sum_vec[id] = min(min(min(sum_vec[id], sum_vec[id+ 64]), 
		sum_vec[id + 128]), sum_vec[id + 192]);
	DeviceMemoryBarrierWithGroupSync();
    if(id < 8)
      sum_vec[id] = min(min(min(min(min(min(min(sum_vec[id], sum_vec[id + 8]),
         sum_vec[id + 16]), sum_vec[id + 24]), sum_vec[id + 32]), sum_vec[id + 40]),
         sum_vec[id + 48]), sum_vec[id + 56]);
	DeviceMemoryBarrierWithGroupSync();
	if(id == 0){
		result = min(min(min(min(min(min(min(sum_vec[0], sum_vec[1]), sum_vec[2]),
			sum_vec[3]), sum_vec[4]), sum_vec[5]), sum_vec[6]), sum_vec[7]);
	}
	DeviceMemoryBarrierWithGroupSync();
}
static inline void parallel_reduction_max(out float result, inout float sum_vec[LOCAL_SIZE], int id)
{
   if(id < 64)
      sum_vec[id] = max(max(max(sum_vec[id], sum_vec[id+ 64]),
         sum_vec[id + 128]), sum_vec[id + 192]);
	DeviceMemoryBarrierWithGroupSync();
   if(id < 8)
      sum_vec[id] = max(max(max(max(max(max(max(sum_vec[id], sum_vec[id + 8]),
         sum_vec[id + 16]), sum_vec[id + 24]), sum_vec[id + 32]), sum_vec[id + 40]),
         sum_vec[id + 48]), sum_vec[id + 56]);
	DeviceMemoryBarrierWithGroupSync();
   if(id == 0){
      result = max(max(max(max(max(max(max(sum_vec[0], sum_vec[1]), sum_vec[2]),
         sum_vec[3]), sum_vec[4]), sum_vec[5]), sum_vec[6]), sum_vec[7]);
   }
	DeviceMemoryBarrierWithGroupSync();
}
static inline float scale(float value, float min, float max)
{
	if (abs(max - min) > 1.0f)
	{
		return (value - min) / (max - min);
	}
	return value - min;
}

[RootSignature(Post_RootSignature)]
[numthreads(LOCAL_SIZE, 1, 1)]
void main(ComputeShaderInput IN)
{
	int IMAGE_WIDTH = 960;
	int IMAGE_HEIGHT = 960;
	
	float u_length_squared, dot, block_min, block_max, vec_length;
	float prod = 0.0f;
	
	const int group_id = IN.GroupIndex;
	const int id = IN.GroupThreadID.x;
	const int buffers = BUFFER_COUNT;
	
	// Normalization non scaled buffer
	for (int feature_buffer = FEATURES_NOT_SCALED; feature_buffer < buffers - 3; ++feature_buffer)
	{
		float tmp_max = -INFINITY;
		float tmp_min = INFINITY;
		for (int sub_vector = 0; sub_vector < BLOCK_PIXELS / LOCAL_SIZE; ++sub_vector)
		{
			float value = tmp_data.Load(int4(IN_ACCESS, 0));
			tmp_max = max(value, tmp_max);
			tmp_min = min(value, tmp_min);
		}
		// reduce the max
		sum_vec[id] = tmp_max;
		DeviceMemoryBarrierWithGroupSync();
		parallel_reduction_max(block_max, sum_vec, id);
		// reduce the min
		sum_vec[id] = tmp_min;
		DeviceMemoryBarrierWithGroupSync();
		parallel_reduction_min(block_min, sum_vec, id);
		// store the min_max for the thread group
		if (id == 0) {
			int3 index = int3(BLOCK_INDEX_X, BLOCK_INDEX_Y, feature_buffer - FEATURES_NOT_SCALED);
			mins_maxs[index] = float2(block_min, block_max);
		}
		DeviceMemoryBarrierWithGroupSync();
		// normalization
		for (int sub_vector = 0; sub_vector < BLOCK_PIXELS / LOCAL_SIZE; ++sub_vector)
		{
			float scaled_value = scale(tmp_data.Load(int4(IN_ACCESS, 0)), block_min, block_max);
			tmp_data[IN_ACCESS] = scaled_value;
		}
	}
	
	// require to process all columns
	int limit = buffers == BLOCK_PIXELS ? buffers - 1 : buffers;
	
	// compute R
	for (int col = 0; col < limit; col++)
	{
		int col_limited = min(col, buffers - 3);
		// Load new column into memory
		int feature_buffer = col;
		float tmp_sum_value = 0.f;
		for (int sub_vector = 0; sub_vector < BLOCK_PIXELS / LOCAL_SIZE; ++sub_vector)
		{
			float tmp = tmp_data.Load(int4(IN_ACCESS, 0));

			const int index = id + sub_vector * LOCAL_SIZE;
			u_vec[index] = tmp;
			if (index >= col_limited + 1)
			{
				tmp_sum_value += tmp * tmp;
			}
		}
		DeviceMemoryBarrierWithGroupSync();
		
		// Find length of vector in A's column with reduction sum function
		sum_vec[id] = tmp_sum_value;
		DeviceMemoryBarrierWithGroupSync();
		parallel_reduction_sum( vec_length, sum_vec, id);
		
		float r_value;
		if (id < col)
		{
            // Copy u_vec value
			r_value = u_vec[id];
		}
		else if (id == col)
		{
			u_length_squared = vec_length; // y2^2+y3^2+...+yn^2
			vec_length = sqrt(vec_length + u_vec[col_limited] * u_vec[col_limited]); // sqrt(y1^2+y2^2+y3^2+...+yn^2)
			u_vec[col_limited] -= vec_length; // y1 - sqrt(y1^2+y2^2+y3^2+...+yn^2) = w1
			u_length_squared += u_vec[col_limited] * u_vec[col_limited]; // 2*dot(w,y)
            // (u_length_squared is now updated length squared)
			r_value = vec_length;
		}
		else if (id > col)
		{
			r_value = 0.0f;
		}
		
		// store R matrix
		int id_limited = min(id, buffers - 3);
		if (col < buffers - 3)
			store_r_mat_broadcast(r_mat, col_limited, id_limited, r_value);
		else
			store_r_mat_channel(r_mat, col_limited, id_limited, col - buffers + 3, r_value);
		DeviceMemoryBarrierWithGroupSync();
		
		// Transform columns of A
		for (int feature_buffer = col_limited + 1; feature_buffer < buffers; feature_buffer++)
		{
			float tmp_data_private_cache[(BLOCK_EDGE_LENGTH * BLOCK_EDGE_LENGTH) / LOCAL_SIZE];
			float tmp_sum_value = 0.f;
			for (int sub_vector = 0; sub_vector < BLOCK_PIXELS / LOCAL_SIZE; ++sub_vector)
			{
				const int index = id + sub_vector * LOCAL_SIZE;
				if (index >= col_limited)
				{
					float tmp = tmp_data.Load(int4(IN_ACCESS, 0));
					// Add noise on the first time values are loaded
					// (does not add noise to constant buffer and noisy image data)
					if (col == 0 && feature_buffer < buffers - 3)
					{
						tmp = add_random(tmp, id, sub_vector, feature_buffer, frameIndexCB.FrameIndex);
					}
					tmp_data_private_cache[sub_vector] = tmp;
					tmp_sum_value += tmp * u_vec[index];
				}
			}
			sum_vec[id] = tmp_sum_value;
			DeviceMemoryBarrierWithGroupSync();
			parallel_reduction_sum( dot, sum_vec, id);

			for (int sub_vector = 0; sub_vector < BLOCK_PIXELS / LOCAL_SIZE; ++sub_vector)
			{
				const int index = id + sub_vector * LOCAL_SIZE;
				if (index >= col_limited)
				{
					float store_value = tmp_data_private_cache[sub_vector];
					store_value -= 2 * u_vec[index] * dot / u_length_squared;
					tmp_data[IN_ACCESS]=store_value;
				}
			}
			DeviceMemoryBarrierWithGroupSync();
		}	
	}
	
	// Back substitution
	for (int i = R_EDGE - 2; i >= 0; i--)
	{
		// divider
		if (id == 0)
			divider = load_r_mat(r_mat, i, i);
		DeviceMemoryBarrierWithGroupSync();
		// scale with divider
		if (id < R_EDGE)
		{
			float3 value = load_r_mat(r_mat, id, i);
			store_r_mat(r_mat, id, i, value / divider);
		}
		DeviceMemoryBarrierWithGroupSync();
		 //Optimization proposal: parallel reduction to calculate Xn
		if (id == 0)
			for (int j = i + 1; j < R_EDGE - 1; j++)
			{
				float3 value = load_r_mat(r_mat, R_EDGE - 1, i);
				float3 value2 = load_r_mat(r_mat, j, i);
				store_r_mat(r_mat, R_EDGE - 1, i, value - value2);
			}
		DeviceMemoryBarrierWithGroupSync();
		// scale R with Xn
		if (id < R_EDGE)
		{
			float3 value = load_r_mat(r_mat, i, id);
			float3 value2 = load_r_mat(r_mat, R_EDGE - 1, i);
			store_r_mat(r_mat, i, id, value * value2);
		}
		DeviceMemoryBarrierWithGroupSync();
	}
	
	// Store weights
	if (id < buffers - 3) {
		int3 index = int3(BLOCK_INDEX_X, BLOCK_INDEX_Y, id);
		float3 weight = load_r_mat(r_mat, R_EDGE - 1, id);
		weights[index] = weight;
	}
}