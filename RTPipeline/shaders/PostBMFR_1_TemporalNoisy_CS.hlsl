#define BLOCK_SIZE_X 8
#define BLOCK_SIZE_Y 8

struct ComputeShaderInput
{
	uint3 GroupID : SV_GroupID; // 3D index of the thread group in the dispatch.
	uint3 GroupThreadID : SV_GroupThreadID; // 3D index of local thread ID in a thread group.
	uint3 DispatchThreadID : SV_DispatchThreadID; // 3D index of global thread ID in the dispatch.
	uint GroupIndex : SV_GroupIndex; // Flattened local index of the thread within a thread group.
};

#define Post_RootSignature \
    "RootFlags(0), " \
    "DescriptorTable( SRV(t0, numDescriptors = 1)," \
								"UAV(u0, numDescriptors = 1) )," \
    "CBV(b0)" 


Texture2D<float4> gPosition : register(t0);
RWTexture2D<float4> col_acc : register(u0);
struct ViewProjectMatrix_prev
{
	matrix viewProject_prev;
};
ConstantBuffer<ViewProjectMatrix_prev> viewProjectMatrix_prev : register(b0);


[RootSignature(Post_RootSignature)]
[numthreads(BLOCK_SIZE_X, BLOCK_SIZE_Y, 1)]
void main(ComputeShaderInput IN)
{
	int2 texCoord = (int2) IN.DispatchThreadID.xy;
	float resx;float resy;float mipLevels;
	gPosition.GetDimensions(0, resx, resy, mipLevels);
}