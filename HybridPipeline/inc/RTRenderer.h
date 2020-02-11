#include <DirectXMath.h>

#include "DX12LibPCH.h"
#include "CommandList.h"
#include "CommandQueue.h"
#include "Application.h"
#include "Mesh.h"
#include "Texture.h"

class RTRenderer {
public:
	struct AccelerationStructureBuffers
	{
		ID3D12ResourcePtr pScratch;
		ID3D12ResourcePtr pResult;
		ID3D12ResourcePtr pInstanceDesc;    // Used only for top-level AS
	};
	RTRenderer(uint64_t width, uint64_t height);

	void createAccelerationStructures(std::vector<Mesh*> meshes);
	void createRtPipelineState();
	void createShaderResources(const Texture& normalTexture);
	void createShaderTable();

	void onRender(CommandList& commandList, std::vector<XMMATRIX> trans);

	Texture getOutputResource() { return *mpOutputTexture; };

private:
	// Acceleration Structure
	AccelerationStructureBuffers RTRenderer::createBottomLevelAS(CommandList& commandList, Mesh* mesh);
		//TODO: pass by reference
	void buildTopLevelAS(CommandList& commandList, std::vector<XMMATRIX> trans, bool update);
	std::vector<ID3D12ResourcePtr> mpBottomLevelAs;
	AccelerationStructureBuffers mTopLevelBuffers;
	uint64_t mTlasSize = 0;

	// PSO
	ID3D12StateObjectPtr mpPipelineState;
	ID3D12RootSignaturePtr mpEmptyRootSig;

	// Shader Table
	ID3D12ResourcePtr mpShaderTable;
	uint32_t mShaderTableEntrySize = 0;

	// mesh and instances
	uint32_t mMeshCount;

	// Resources
	ID3D12DescriptorHeapPtr mpSrvUavHeap;
	//ID3D12ResourcePtr mpOutputResource;
	std::shared_ptr<Texture> mpOutputTexture;

	// viewport
	uint64_t m_width;
	uint64_t m_height;
};