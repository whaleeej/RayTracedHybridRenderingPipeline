//#include "DX12LibPCH.h"
//#include "Mesh.h"
//#include "CommandList.h"
//
//class RTRenderer {
//public:
//	struct AccelerationStructureBuffers
//	{
//		ID3D12ResourcePtr pScratch;
//		ID3D12ResourcePtr pResult;
//		ID3D12ResourcePtr pInstanceDesc;    // Used only for top-level AS
//	};
//
//
//private:
//	// Acceleration Structure
//	void createAccelerationStructures(CommandList& commmandList, std::vector<Mesh*> meshes);
//	RTRenderer::AccelerationStructureBuffers RTRenderer::createBottomLevelAS(CommandList& pCmdList, Mesh* pMesh);
//	std ::vector<ID3D12ResourcePtr> mpBottomLevelAs;
//	AccelerationStructureBuffers mTopLevelBuffers;
//	uint64_t mTlasSize = 0;
//
//};