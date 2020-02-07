//#include "RTRenderer.h"
//#include "Application.h"
//
//void RTRenderer::createAccelerationStructures(CommandList& commmandList, std::vector<Mesh*> meshes)
//{
//	std::vector<AccelerationStructureBuffers> bottomLevelBuffers;
//	for (auto pMesh : meshes) {
//
//	}
//}
//
//RTRenderer::AccelerationStructureBuffers RTRenderer::createBottomLevelAS(CommandList& pCmdList, Mesh* pMesh)
//{
//	auto pDevice = Application::Get().GetDevice();
//	
//	uint32_t indicesCount = pMesh->m_IndexCount;
//	assert(indicesCount % 3 == 0 && "Invalid Triangular Indices, not times of thress");
//	const uint32_t geometryCount = indicesCount / 3;
//
//	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geomDesc;
//	geomDesc.resize(geometryCount);
//
//	for (uint32_t i = 0; i < geometryCount; i++)
//	{
//		// vertex buffer
//		geomDesc[i].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
//		geomDesc[i].Triangles.VertexBuffer.StartAddress = pMesh->m_VertexBuffer.GetVertexBufferView().BufferLocation;
//		geomDesc[i].Triangles.VertexBuffer.StrideInBytes = pMesh->m_VertexBuffer.GetVertexStride();
//		geomDesc[i].Triangles.VertexCount = pMesh->m_VertexBuffer.GetNumVertices();
//		geomDesc[i].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
//		// indices buffer
//		geomDesc[i].Triangles.IndexBuffer = pMesh->m_IndexBuffer.GetIndexBufferView().BufferLocation;
//		geomDesc[i].Triangles.IndexCount = pMesh->m_IndexCount;
//		geomDesc[i].Triangles.IndexFormat = pMesh->m_IndexBuffer.GetIndexFormat;
//		geomDesc[i].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
//	}
//
//	// Get the size requirements for the scratch and AS buffers
//	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
//	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
//	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
//	inputs.NumDescs = geometryCount;
//	inputs.pGeometryDescs = geomDesc.data();
//	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
//
//	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
//	pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);
//
//	// Create the buffers. They need to support UAV, and since we are going to immediately use them, we create them with an unordered-access state
//	AccelerationStructureBuffers buffers;
//	buffers.pScratch = createBuffer(pDevice, info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, kDefaultHeapProps);
//	buffers.pResult = createBuffer(pDevice, info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, kDefaultHeapProps);
//
//	// Create the bottom-level AS
//	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
//	asDesc.Inputs = inputs;
//	asDesc.DestAccelerationStructureData = buffers.pResult->GetGPUVirtualAddress();
//	asDesc.ScratchAccelerationStructureData = buffers.pScratch->GetGPUVirtualAddress();
//
//	pCmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);
//
//	// We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
//	D3D12_RESOURCE_BARRIER uavBarrier = {};
//	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
//	uavBarrier.UAV.pResource = buffers.pResult;
//	pCmdList->ResourceBarrier(1, &uavBarrier);
//
//	return buffers;
//}
