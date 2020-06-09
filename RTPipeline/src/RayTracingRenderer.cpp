#include "RayTracingRenderer.h"
#include <Application.h>
#include <CommandQueue.h>
#include <CommandList.h>
#include <ASBuffer.h>
#include <ResourceStateTracker.h>

RayTracingRenderer::RayTracingRenderer(int w, int h):
	Renderer(w, h)
{
	m_generatorURNG.seed(1729);
}

RayTracingRenderer::~RayTracingRenderer()
{
}

void RayTracingRenderer::LoadResource(std::shared_ptr<Scene> scene, RenderResourceMap& resources)
{
	Renderer::LoadResource(scene, resources);

	m_Scene = scene;

	gPosition = *resources[RRD_POSITION];
	gAlbedoMetallic = *resources[RRD_ALBEDO_MATALLIC];
	gNormalRoughness = *resources[RRD_NORMAL_ROUGHNESS];
	gExtra = *resources[RRD_EXTRA];

	createAccelerationStructures();
	createRtPipelineState();
	createShaderResources();
	createSrvUavHeap();
	createShaderTable();
	
	resources.emplace(RRD_RT_SHADOW_SAMPLE, std::make_shared<Texture>(mRtShadowOutputTexture));
	resources.emplace(RRD_RT_INDIRECT_SAMPLE, std::make_shared<Texture>(mRtReflectOutputTexture));
}

void RayTracingRenderer::LoadPipeline()
{
}

void RayTracingRenderer::Update(UpdateEventArgs& e, std::shared_ptr<Scene> scene)
{
	// update the structure buffer of frameId
	{
		std::uniform_int_distribution<uint32_t> seedDistribution(0, UINT_MAX);
		FrameIndexRTCB fid;
		fid.FrameIndex = static_cast<uint32_t>(e.FrameNumber);
		fid.seed = seedDistribution(m_generatorURNG);
		void* pData;
		mpRTFrameIndexCB->Map(0, nullptr, (void**)&pData);
		memcpy(pData, &fid, sizeof(FrameIndexRTCB));
		mpRTFrameIndexCB->Unmap(0, nullptr);
	}

	// update the camera for rt
	{
		CameraRTCB mCameraCB;
		mCameraCB.PositionWS = scene->m_Camera.get_Translation();
		mCameraCB.InverseViewMatrix = scene->m_Camera.get_InverseViewMatrix();
		mCameraCB.fov = scene->m_Camera.get_FoV();
		void* pData;
		mpRTCameraCB->Map(0, nullptr, (void**)&pData);
		memcpy(pData, &mCameraCB, sizeof(CameraRTCB));
		mpRTCameraCB->Unmap(0, nullptr);
	}

	// update the light for rt
	{
		void* pData;
		mpRTPointLightCB->Map(0, nullptr, (void**)&pData);
		memcpy(pData, &(scene->m_PointLight), sizeof(PointLight));
		mpRTPointLightCB->Unmap(0, nullptr);
	}
}

void RayTracingRenderer::Render(RenderEventArgs& e, std::shared_ptr<Scene> scene, std::shared_ptr<CommandList> commandList)
{
	// Let's raytrace
	
	commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, mpSrvUavHeap.Get());
	//commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, mpSamplerHeap.Get());

	commandList->TransitionBarrier(mRtShadowOutputTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	commandList->TransitionBarrier(mRtReflectOutputTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	commandList->TransitionBarrier(gPosition, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	commandList->TransitionBarrier(gAlbedoMetallic, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	commandList->TransitionBarrier(gNormalRoughness, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	commandList->TransitionBarrier(gExtra, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	D3D12_DISPATCH_RAYS_DESC raytraceDesc = {};
	raytraceDesc.Width = m_Width;
	raytraceDesc.Height = m_Height;
	raytraceDesc.Depth = 1;

	// RayGen is the first entry in the shader-table
	raytraceDesc.RayGenerationShaderRecord.StartAddress = mpShaderTable->GetGPUVirtualAddress() + 0 * mShaderTableEntrySize;
	raytraceDesc.RayGenerationShaderRecord.SizeInBytes = mShaderTableEntrySize;

	// Miss is the second entry in the shader-table
	size_t missOffset = 1 * mShaderTableEntrySize;
	raytraceDesc.MissShaderTable.StartAddress = mpShaderTable->GetGPUVirtualAddress() + missOffset;
	raytraceDesc.MissShaderTable.StrideInBytes = mShaderTableEntrySize;
	raytraceDesc.MissShaderTable.SizeInBytes = mShaderTableEntrySize * 2;   // Only two

	int objectToRT = 0;
	for (auto it = m_Scene->gameObjectPool.begin(); it != m_Scene->gameObjectPool.end(); it++)
	{
		// 特殊处理剔除光源不加入光追
		std::string objIndex = it->first;
		if (objIndex.find("light") != std::string::npos) {
			continue;
		}
		objectToRT++;
		commandList->TransitionBarrier(*(it->second)->material.getAlbedoTexture(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->TransitionBarrier(*(it->second)->material.getMetallicTexture(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->TransitionBarrier(*(it->second)->material.getNormalTexture(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->TransitionBarrier(*(it->second)->material.getMetallicTexture(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->TransitionBarrier(MeshPool::Get().getPool()[it->second->mesh]->getVertexBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->TransitionBarrier(MeshPool::Get().getPool()[it->second->mesh]->getIndexBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}

	// Hit is the third entry in the shader-table
	size_t hitOffset = 3 * mShaderTableEntrySize;
	raytraceDesc.HitGroupTable.StartAddress = mpShaderTable->GetGPUVirtualAddress() + hitOffset;
	raytraceDesc.HitGroupTable.StrideInBytes = mShaderTableEntrySize;
	raytraceDesc.HitGroupTable.SizeInBytes = mShaderTableEntrySize * (2 * objectToRT);

	// Bind the empty root signature
	commandList->SetComputeRootSignature(mpEmptyRootSig);
	// Dispatch
	commandList->SetPipelineState1(mpPipelineState);
	commandList->DispatchRays(&raytraceDesc);
	
}

void RayTracingRenderer::createAccelerationStructures()
{
	const D3D12_HEAP_PROPERTIES kUploadHeapProps =
	{
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,
		0,
	};

	const D3D12_HEAP_PROPERTIES kDefaultHeapProps =
	{
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,
		0
	};

	auto pDevice = Application::Get().GetDevice();
	auto commandQueue = Application::Get().GetCommandQueue();
	auto commandList = commandQueue->GetCommandList();

	std::vector<AccelerationStructureBuffers> bottomLevelBuffers;
	mpBottomLevelASes.resize(MeshPool::Get().size());
	bottomLevelBuffers.resize(MeshPool::Get().size());//Before the commandQueue finishes its execution of AS creation, these resources' lifecycle have to be held
	std::map<MeshIndex, int> MeshIndex2blasSeqCache; // Record MeshIndex to the sequence of mesh in mpBottomLevelASes

	int blasSeqCount = 0;
	for (auto it = MeshPool::Get().getPool().begin(); it != MeshPool::Get().getPool().end(); it++) { //mpBottomLevelASes

		MeshIndex2blasSeqCache.emplace(it->first, blasSeqCount);
		auto pMesh = it->second;

		// set the index and vertex buffer to generic read status //Crucial!!!!!!!! // Make Sure the correct status // hey yeah
		commandList->TransitionBarrier(pMesh->getVertexBuffer(), D3D12_RESOURCE_STATE_GENERIC_READ);
		commandList->TransitionBarrier(pMesh->getIndexBuffer(), D3D12_RESOURCE_STATE_GENERIC_READ);

		D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
		geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geomDesc.Triangles.VertexBuffer.StartAddress = pMesh->getVertexBuffer().GetD3D12Resource()->GetGPUVirtualAddress();//Vertex Setup
		geomDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(VertexPositionNormalTexture);
		geomDesc.Triangles.VertexFormat = VertexPositionNormalTexture::InputElements[0].Format;
		geomDesc.Triangles.VertexCount = pMesh->getVertexCount();
		geomDesc.Triangles.IndexBuffer = pMesh->getIndexBuffer().GetD3D12Resource()->GetGPUVirtualAddress(); //Index Setup
		geomDesc.Triangles.IndexFormat = pMesh->getIndexBuffer().GetIndexBufferView().Format;
		geomDesc.Triangles.IndexCount = pMesh->getIndexCount();

		geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

		// Get the size requirements for the scratch and AS buffers
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
		inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
		inputs.NumDescs = 1;
		inputs.pGeometryDescs = &geomDesc;
		inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
		pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

		// Create the buffers. They need to support UAV, and since we are going to immediately use them, we create them with an unordered-access state
		bottomLevelBuffers[blasSeqCount].pScratch = createBuffer(pDevice, info.ScratchDataSizeInBytes,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kDefaultHeapProps);
		bottomLevelBuffers[blasSeqCount].pResult = createBuffer(pDevice, info.ResultDataMaxSizeInBytes,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, kDefaultHeapProps);
		// 成员变量持有，全局记录持久资源的状态
		ASBuffer asBuffer;
		mpBottomLevelASes[blasSeqCount] = bottomLevelBuffers[blasSeqCount].pResult;
		asBuffer.SetD3D12Resource(mpBottomLevelASes[blasSeqCount]);
		asBuffer.SetName(L"BottomLevelAS" + string_2_wstring(it->first));
		ResourceStateTracker::AddGlobalResourceState(mpBottomLevelASes[blasSeqCount].Get(), D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

		// Create the bottom-level AS
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
		asDesc.Inputs = inputs;
		asDesc.DestAccelerationStructureData = bottomLevelBuffers[blasSeqCount].pResult->GetGPUVirtualAddress();
		asDesc.ScratchAccelerationStructureData = bottomLevelBuffers[blasSeqCount].pScratch->GetGPUVirtualAddress();

		commandList->BuildRaytracingAccelerationStructure(&asDesc);

		// We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
		commandList->UAVBarrier(asBuffer, true);
		blasSeqCount++;
	}

	AccelerationStructureBuffers topLevelBuffers; //构建GameObject
	{ // topLevelBuffers
		uint32_t nonLightCount = 0;
		for (auto it = m_Scene->gameObjectPool.begin(); it != m_Scene->gameObjectPool.end(); it++)
		{
			std::string objIndex = it->first;
			if (objIndex.find("light") != std::string::npos) {
				continue;
			}
			nonLightCount++;
		}
		int gameObjectCount = m_Scene->gameObjectPool.size();
		// First, get the size of the TLAS buffers and create them
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
		inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
		inputs.NumDescs = nonLightCount;
		inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
		pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

		// Create the buffers
		topLevelBuffers.pScratch = createBuffer(pDevice, info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kDefaultHeapProps);
		topLevelBuffers.pResult = createBuffer(pDevice, info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, kDefaultHeapProps);
		mTlasSize = info.ResultDataMaxSizeInBytes;
		// 成员变量持有，全局记录持久资源的状态
		ASBuffer asBuffer;
		mpTopLevelAS = topLevelBuffers.pResult;
		asBuffer.SetD3D12Resource(mpTopLevelAS.Get());
		asBuffer.SetName(L"TopLevelAS");
		ResourceStateTracker::AddGlobalResourceState(mpTopLevelAS.Get(), D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);



		// The instance desc should be inside a buffer, create and map the buffer
		topLevelBuffers.pInstanceDesc = createBuffer(pDevice, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * nonLightCount, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
		D3D12_RAYTRACING_INSTANCE_DESC* pInstanceDesc;
		topLevelBuffers.pInstanceDesc->Map(0, nullptr, (void**)&pInstanceDesc);
		ZeroMemory(pInstanceDesc, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * nonLightCount);

		int tlasGoCount = 0;
		for (auto it = m_Scene->gameObjectPool.begin(); it != m_Scene->gameObjectPool.end(); it++)
		{
			// 特殊处理剔除光源不加入光追
			std::string objIndex = it->first;
			if (objIndex.find("light") != std::string::npos) {
				continue;
			}

			// Initialize the instance desc. We only have a single instance
			pInstanceDesc[tlasGoCount].InstanceID = tlasGoCount;                            // This value will be exposed to the shader via InstanceID()
			pInstanceDesc[tlasGoCount].InstanceContributionToHitGroupIndex = 2 * tlasGoCount;   // This is the offset inside the shader-table. 
			pInstanceDesc[tlasGoCount].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
			XMMATRIX m = XMMatrixTranspose(it->second->transform.ComputeModel());
			memcpy(pInstanceDesc[tlasGoCount].Transform, m.r, sizeof(pInstanceDesc[tlasGoCount].Transform));
			pInstanceDesc[tlasGoCount].AccelerationStructure =/* mpBottomLevelAS->GetGPUVirtualAddress();*/ mpBottomLevelASes[MeshIndex2blasSeqCache[it->second->mesh]]->GetGPUVirtualAddress();
			pInstanceDesc[tlasGoCount].InstanceMask = 0xFF;
			tlasGoCount++;
		}

		// Unmap
		topLevelBuffers.pInstanceDesc->Unmap(0, nullptr);

		// Create the TLAS
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
		asDesc.Inputs = inputs;
		asDesc.Inputs.InstanceDescs = topLevelBuffers.pInstanceDesc->GetGPUVirtualAddress();
		asDesc.DestAccelerationStructureData = topLevelBuffers.pResult->GetGPUVirtualAddress();
		asDesc.ScratchAccelerationStructureData = topLevelBuffers.pScratch->GetGPUVirtualAddress();

		commandList->BuildRaytracingAccelerationStructure(&asDesc);

		// We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
		commandList->UAVBarrier(asBuffer, true);
	}

	// Wait for the execution of commandlist on the direct command queue, then release scratch resources in topLevelBuffers and bottomLevelBuffers
	uint64_t mFenceValue = commandQueue->ExecuteCommandList(commandList);
	commandQueue->WaitForFenceValue(mFenceValue);
}

void RayTracingRenderer::createRtPipelineState()
{
	// Need 10 subobjects:
	//  1 for the DXIL library
	//  1 for hit-group
	//  2 for RayGen root-signature (root-signature and the subobject association)
	//  2 for the root-signature shared between miss and hit shaders (signature and association)
	//  2 for shader config (shared between all programs. 1 for the config, 1 for association)
	//  1 for pipeline config
	//  1 for the global root signature
	auto mpDevice = Application::Get().GetDevice();

	std::array<D3D12_STATE_SUBOBJECT, 15> subobjects;
	uint32_t index = 0;

	// Compile the shader
	auto pDxilLib = compileLibrary(L"RTPipeline/shaders/RayTracing.hlsl", L"lib_6_3");
	const WCHAR* entryPoints[] = { kRayGenShader, kShadowMissShader ,kShadwoClosestHitShader, kSecondaryMissShader , kSecondaryClosestHitShader };
	DxilLibrary dxilLib = DxilLibrary(pDxilLib, entryPoints, arraysize(entryPoints));
	subobjects[index++] = dxilLib.stateSubobject; // 0 Library

	HitProgram shadowHitProgram(0, kShadwoClosestHitShader, kShadowHitGroup);
	subobjects[index++] = shadowHitProgram.subObject; // 1 shadow Hit Group

	HitProgram secondaryHitProgram(0, kSecondaryClosestHitShader, kSecondaryHitGroup);
	subobjects[index++] = secondaryHitProgram.subObject; // 2 secondary Hit Group

	CD3DX12_STATIC_SAMPLER_DESC static_sampler_desc(0);

	// Create the ray-gen root-signature and association
	LocalRootSignature rgsRootSignature(mpDevice, createLocalRootDesc(2, 5, 1, &static_sampler_desc, 3, 0, 0).desc);
	subobjects[index] = rgsRootSignature.subobject; // 3 RayGen Root Sig

	uint32_t rgsRootIndex = index++; // 3
	ExportAssociation rgsRootAssociation(&kRayGenShader, 1, &(subobjects[rgsRootIndex]));
	subobjects[index++] = rgsRootAssociation.subobject; // 4 Associate Root Sig to RGS

	// Create the secondary root-signature and association
	LocalRootSignature secondaryRootSignature(mpDevice, createLocalRootDesc(0, 7, 1, &static_sampler_desc, 5, 0, 1).desc);
	subobjects[index] = secondaryRootSignature.subobject;// 5 Secondary chs sig

	uint32_t secondaryRootIndex = index++;//5
	ExportAssociation secondaryRootAssociation(&kSecondaryClosestHitShader, 1, &(subobjects[secondaryRootIndex]));
	subobjects[index++] = secondaryRootAssociation.subobject;//6 Associate secondary sig to secondarychs

	// Create the secondary miss root-signature and association
	LocalRootSignature secondaryMissRootSignature(mpDevice, createLocalRootDesc(0, 1, 1, &static_sampler_desc, 0, 0, 2).desc);
	subobjects[index] = secondaryMissRootSignature.subobject;// 7 Secondary miss sig

	uint32_t secondaryMissRootIndex = index++;//7
	ExportAssociation secondaryMissRootAssociation(&kSecondaryMissShader, 1, &(subobjects[secondaryMissRootIndex]));
	subobjects[index++] = secondaryMissRootAssociation.subobject;//8 Associate secondary sig to secondarychs

	// Create the miss- and hit-programs root-signature and association
	D3D12_ROOT_SIGNATURE_DESC emptyDesc = {};
	emptyDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	LocalRootSignature hitMissRootSignature(mpDevice, emptyDesc);
	subobjects[index] = hitMissRootSignature.subobject; // 9 Root Sig to be shared between Miss and shadowCHS

	uint32_t hitMissRootIndex = index++; // 9
	const WCHAR* missHitExportName[] = { kShadowMissShader, kShadwoClosestHitShader };
	ExportAssociation missHitRootAssociation(missHitExportName, arraysize(missHitExportName), &(subobjects[hitMissRootIndex]));
	subobjects[index++] = missHitRootAssociation.subobject; // 10 Associate Root Sig to Miss and CHS

	// Bind the payload size to the programs
	ShaderConfig shaderConfig(sizeof(float) * 2, sizeof(float) * 4);
	subobjects[index] = shaderConfig.subobject; // 11 Shader Config

	uint32_t shaderConfigIndex = index++; // 11 
	const WCHAR* shaderExports[] = { kRayGenShader, kShadowMissShader, kSecondaryMissShader, kShadwoClosestHitShader, kSecondaryClosestHitShader };
	ExportAssociation configAssociation(shaderExports, arraysize(shaderExports), &(subobjects[shaderConfigIndex]));
	subobjects[index++] = configAssociation.subobject; // 12 Associate Shader Config to Miss, shadowCHS, shadowRGS, secondaryCHS, secondaryRGS

	// Create the pipeline config
	PipelineConfig config(4);
	subobjects[index++] = config.subobject; // 13 configuration 

	// Create the global root signature and store the empty signature
	GlobalRootSignature root(mpDevice, {});
	mpEmptyRootSig = root.pRootSig;
	subobjects[index++] = root.subobject; // 14

	// Create the state
	D3D12_STATE_OBJECT_DESC desc;
	desc.NumSubobjects = index; // 15
	desc.pSubobjects = subobjects.data();
	desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

	ThrowIfFailed(mpDevice->CreateStateObject(&desc, IID_PPV_ARGS(&mpPipelineState)));
}

RootSignatureDesc RayTracingRenderer::createLocalRootDesc(int uav_num, int srv_num, int sampler_num,
	const D3D12_STATIC_SAMPLER_DESC* pStaticSamplers, int cbv_num, int c32_num, int space)
{
	// Create the root-signature 
	//******Layout*********/
	// Param[0] = DescriptorTable
	/////////////// |---------------range for UAV---------------| |-------------------range for SRV-------------------|
	// Param[1-n] = 
	RootSignatureDesc desc;
	int range_size = 0;
	if (uav_num > 0) range_size++;
	if (srv_num > 0) range_size++;

	desc.range.resize(range_size);
	int range_counter = 0;
	// UAV
	if (uav_num > 0) {
		desc.range[range_counter].BaseShaderRegister = 0;
		desc.range[range_counter].NumDescriptors = uav_num;
		desc.range[range_counter].RegisterSpace = space;
		desc.range[range_counter].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		desc.range[range_counter].OffsetInDescriptorsFromTableStart = 0;
		range_counter++;
	}
	// SRV
	if (srv_num > 0) {
		desc.range[range_counter].BaseShaderRegister = 0;
		desc.range[range_counter].NumDescriptors = srv_num;
		desc.range[range_counter].RegisterSpace = space;
		desc.range[range_counter].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		desc.range[range_counter].OffsetInDescriptorsFromTableStart = uav_num;
		range_counter++;
	}

	int descriptor_table_counter = 0;
	if (range_counter > 0) descriptor_table_counter++;
	//if (sample_range_counter > 0) descriptor_table_counter++;

	int i = 0;
	desc.rootParams.resize(descriptor_table_counter + cbv_num + c32_num);
	{

		if (range_counter > 0) {
			desc.rootParams[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			desc.rootParams[i].DescriptorTable.NumDescriptorRanges = range_size;
			desc.rootParams[i].DescriptorTable.pDescriptorRanges = desc.range.data();
			i++;
		}
		//if (sample_range_counter > 0) {
		//	desc.rootParams[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		//	desc.rootParams[i].DescriptorTable.NumDescriptorRanges = sample_range_size;
		//	desc.rootParams[i].DescriptorTable.pDescriptorRanges = sampleDesc.range.data();
		//	i++;
		//}
	}
	for (; i < cbv_num + descriptor_table_counter; i++) {
		desc.rootParams[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		desc.rootParams[i].Descriptor.RegisterSpace = space;
		desc.rootParams[i].Descriptor.ShaderRegister = i - descriptor_table_counter;
	}
	for (; i < cbv_num + c32_num + descriptor_table_counter; i++) {
		desc.rootParams[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		desc.rootParams[i].Constants.Num32BitValues = 1;
		desc.rootParams[i].Constants.RegisterSpace = space;
		desc.rootParams[i].Constants.ShaderRegister = i - descriptor_table_counter;
	}
	// Create the desc
	desc.desc.NumParameters = descriptor_table_counter + cbv_num + c32_num;
	desc.desc.pParameters = desc.rootParams.data();
	desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	if (sampler_num > 0) {
		desc.desc.NumStaticSamplers = sampler_num;
		desc.desc.pStaticSamplers = pStaticSamplers;
	}

	return desc;
}

void RayTracingRenderer::createShaderResources()
{
	const D3D12_HEAP_PROPERTIES kUploadHeapProps =
	{
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,
		0,
	};

	const D3D12_HEAP_PROPERTIES kDefaultHeapProps =
	{
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,
		0
	};

	auto pDevice = Application::Get().GetDevice();
	//****************************UAV Resource
	mRtShadowOutputTexture = Texture(CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R32G32B32A32_FLOAT, m_Width, m_Height, 1, 0, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_TEXTURE_LAYOUT_UNKNOWN, 0
	), nullptr, TextureUsage::IntermediateBuffer, L"mpRtShadowOutputTexture");
	mRtReflectOutputTexture = Texture(CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R32G32B32A32_FLOAT, m_Width, m_Height, 1, 0, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_TEXTURE_LAYOUT_UNKNOWN, 0
	), nullptr, TextureUsage::IntermediateBuffer, L"mRtReflectOutputTexture");


	//****************************CBV Resource
	mpRTPointLightCB = createBuffer(pDevice, sizeof(PointLight), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
	mpRTCameraCB = createBuffer(pDevice, sizeof(CameraRTCB), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
	mpRTFrameIndexCB = createBuffer(pDevice, sizeof(FrameIndexRTCB), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);

	//****************************CBV for per object 
	int objectToRT = 0;
	for (auto it = m_Scene->gameObjectPool.begin(); it != m_Scene->gameObjectPool.end(); it++)
	{
		// 特殊处理剔除光源不加入光追
		std::string objIndex = it->first;
		if (objIndex.find("light") != std::string::npos) {
			continue;
		}
		objectToRT++;
	}
	mpRTMaterialCBList.resize(objectToRT);
	mpRTPBRMaterialCBList.resize(objectToRT);
	mpRTGameObjectIndexCBList.resize(objectToRT);
	objectToRT = 0;
	for (auto it = m_Scene->gameObjectPool.begin(); it != m_Scene->gameObjectPool.end(); it++)
	{
		// 特殊处理剔除光源不加入光追
		std::string objIndex = it->first;
		if (objIndex.find("light") != std::string::npos) {
			continue;
		}

		void* pData = 0;
		// base material
		mpRTMaterialCBList[objectToRT] = createBuffer(pDevice, sizeof(Material::MaterialCB), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
		mpRTMaterialCBList[objectToRT]->Map(0, nullptr, (void**)&pData);
		memcpy(pData, &it->second->material.computeMaterialCB(), sizeof(Material::MaterialCB));
		mpRTMaterialCBList[objectToRT]->Unmap(0, nullptr);
		// pbr material
		mpRTPBRMaterialCBList[objectToRT] = createBuffer(pDevice, sizeof(PBRMaterial::PBRMaterialCB), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
		mpRTPBRMaterialCBList[objectToRT]->Map(0, nullptr, (void**)&pData);
		memcpy(pData, &it->second->material.computePBRMaterialCB(), sizeof(PBRMaterial::PBRMaterialCB));
		mpRTPBRMaterialCBList[objectToRT]->Unmap(0, nullptr);
		// rt go index
		mpRTGameObjectIndexCBList[objectToRT] = createBuffer(pDevice, sizeof(ObjectIndexRTCB), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
		mpRTGameObjectIndexCBList[objectToRT]->Map(0, nullptr, (void**)&pData);
		memcpy(pData, &it->second->gid, sizeof(float));
		mpRTGameObjectIndexCBList[objectToRT]->Unmap(0, nullptr);

		objectToRT++;
	}

}

void RayTracingRenderer::createSrvUavHeap() {
	auto pDevice = Application::Get().GetDevice();

	int objectToRT = 0;
	for (auto it = m_Scene->gameObjectPool.begin(); it != m_Scene->gameObjectPool.end(); it++)
	{
		// 特殊处理剔除光源不加入光追
		std::string objIndex = it->first;
		if (objIndex.find("light") != std::string::npos) {
			continue;
		}
		objectToRT++;
	}

	//****************************Descriptor heap

	// Create the uavSrvHeap and its handle
	uint32_t srvuavBias = 8;
	uint32_t srvuavPerHitSize = 7;
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = srvuavBias + objectToRT * srvuavPerHitSize;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mpSrvUavHeap)));
	D3D12_CPU_DESCRIPTOR_HANDLE uavSrvHandle = mpSrvUavHeap->GetCPUDescriptorHandleForHeapStart();

	// Create the UAV for GShadowOutput
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavGOutputDesc = {};
	uavGOutputDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	pDevice->CreateUnorderedAccessView(mRtShadowOutputTexture.GetD3D12Resource().Get(), nullptr, &uavGOutputDesc, uavSrvHandle);
	uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Create the UAV for GReflectOutput
	pDevice->CreateUnorderedAccessView(mRtReflectOutputTexture.GetD3D12Resource().Get(), nullptr, &uavGOutputDesc, uavSrvHandle);
	uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Create srv for TLAS
	D3D12_SHADER_RESOURCE_VIEW_DESC srvTLASDesc = {};
	srvTLASDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvTLASDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvTLASDesc.RaytracingAccelerationStructure.Location = mpTopLevelAS->GetGPUVirtualAddress();
	pDevice->CreateShaderResourceView(nullptr, &srvTLASDesc, uavSrvHandle);
	uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	RenderTarget m_GBuffer;
	m_GBuffer.AttachTexture(AttachmentPoint::Color0, gPosition);
	m_GBuffer.AttachTexture(AttachmentPoint::Color1, gAlbedoMetallic);
	m_GBuffer.AttachTexture(AttachmentPoint::Color2, gNormalRoughness);
	m_GBuffer.AttachTexture(AttachmentPoint::Color3, gExtra);

	//GBuffer
	for (int i = 0; i < 4; i++) {
		UINT pDestDescriptorRangeSizes[] = { 1 };
		pDevice->CopyDescriptors(1, &uavSrvHandle, pDestDescriptorRangeSizes,
			1, &m_GBuffer.GetTexture((AttachmentPoint)i).GetShaderResourceView(), pDestDescriptorRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	// skybox
	auto& skybox_texture = TexturePool::Get().getTexture("skybox_cubemap");
	D3D12_SHADER_RESOURCE_VIEW_DESC skyboxSrvDesc = {};
	skyboxSrvDesc.Format = skybox_texture->GetD3D12ResourceDesc().Format;
	skyboxSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	skyboxSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	skyboxSrvDesc.TextureCube.MipLevels = (UINT)-1; // Use all mips.
	// TODO: Need a better way to bind a cubemap.
	pDevice->CreateShaderResourceView(skybox_texture->GetD3D12Resource().Get(), &skyboxSrvDesc, uavSrvHandle);
	uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	objectToRT = 0;
	for (auto it = m_Scene->gameObjectPool.begin(); it != m_Scene->gameObjectPool.end(); it++)
	{
		// 特殊处理剔除光源不加入光追
		std::string objIndex = it->first;
		if (objIndex.find("light") != std::string::npos) {
			continue;
		}
		auto pLobject = it->second;

		UINT pDestDescriptorRangeSizes[] = { 1 };
		//pDevice->CopyDescriptors(1, &uavSrvHandle, pDestDescriptorRangeSizes,
		//	1,              , pDestDescriptorRangeSizes, 
		//	D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		//uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// indices 0
		pDevice->CreateShaderResourceView(nullptr, &srvTLASDesc, uavSrvHandle);
		uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// indices 1
		D3D12_SHADER_RESOURCE_VIEW_DESC indexSrvDesc = {};
		indexSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		indexSrvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		indexSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		indexSrvDesc.Buffer.NumElements = (UINT)((MeshPool::Get().getPool()[pLobject->mesh]->getIndexCount()/*+1*/)/*/2*/); // 修改为32bit的index
		indexSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
		pDevice->CreateShaderResourceView(MeshPool::Get().getPool()[pLobject->mesh]->getIndexBuffer().GetD3D12Resource().Get(), &indexSrvDesc, uavSrvHandle);
		uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		// vetices 2
		D3D12_SHADER_RESOURCE_VIEW_DESC vertexSrvDesc = {};
		vertexSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
		vertexSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		vertexSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		vertexSrvDesc.Buffer.FirstElement = 0;
		vertexSrvDesc.Buffer.NumElements = MeshPool::Get().getPool()[pLobject->mesh]->getVertexCount();
		vertexSrvDesc.Buffer.StructureByteStride = sizeof(VertexPositionNormalTexture);
		pDevice->CreateShaderResourceView(MeshPool::Get().getPool()[pLobject->mesh]->getVertexBuffer().GetD3D12Resource().Get(), &vertexSrvDesc, uavSrvHandle);
		uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		//albedo 3
		pDevice->CopyDescriptors(1, &uavSrvHandle, pDestDescriptorRangeSizes,
			1, &pLobject->material.getAlbedoTexture()->GetShaderResourceView(), pDestDescriptorRangeSizes,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		//metallic 4
		pDevice->CopyDescriptors(1, &uavSrvHandle, pDestDescriptorRangeSizes,
			1, &pLobject->material.getMetallicTexture()->GetShaderResourceView(), pDestDescriptorRangeSizes,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		//normal 5
		pDevice->CopyDescriptors(1, &uavSrvHandle, pDestDescriptorRangeSizes,
			1, &pLobject->material.getNormalTexture()->GetShaderResourceView(), pDestDescriptorRangeSizes,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		//roughness 6
		pDevice->CopyDescriptors(1, &uavSrvHandle, pDestDescriptorRangeSizes,
			1, &pLobject->material.getRoughnessTexture()->GetShaderResourceView(), pDestDescriptorRangeSizes,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		objectToRT++;
	}
}

void RayTracingRenderer::createShaderTable()
{
	const D3D12_HEAP_PROPERTIES kUploadHeapProps =
	{
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,
		0,
	};

	const D3D12_HEAP_PROPERTIES kDefaultHeapProps =
	{
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,
		0
	};

	auto mpDevice = Application::Get().GetDevice();

	int objectToRT = 0;
	for (auto it = m_Scene->gameObjectPool.begin(); it != m_Scene->gameObjectPool.end(); it++)
	{
		// 特殊处理剔除光源不加入光追
		std::string objIndex = it->first;
		if (objIndex.find("light") != std::string::npos) {
			continue;
		}
		objectToRT++;
	}

	// Calculate the size and create the buffer
	mShaderTableEntrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	mShaderTableEntrySize += 48; // The ray-gen's descriptor table
	mShaderTableEntrySize = Math::AlignUp(mShaderTableEntrySize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
	uint32_t shaderTableSize = mShaderTableEntrySize * (3 + objectToRT * 2);

	// For simplicity, we create the shader-table on the upload heap. You can also create it on the default heap
	mpShaderTable = createBuffer(mpDevice, shaderTableSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
	NAME_D3D12_OBJECT(mpShaderTable);
	ResourceStateTracker::AddGlobalResourceState(mpShaderTable.Get(), D3D12_RESOURCE_STATE_GENERIC_READ);

	// Map the buffer
	uint8_t* pData;
	ThrowIfFailed(mpShaderTable->Map(0, nullptr, (void**)&pData));

	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> pRtsoProps;
	mpPipelineState->QueryInterface(IID_PPV_ARGS(&pRtsoProps));

	// Entry 0 - ray-gen program ID and descriptor data
	memcpy(pData + mShaderTableEntrySize * 0, pRtsoProps->GetShaderIdentifier(kRayGenShader), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	D3D12_GPU_VIRTUAL_ADDRESS heapStart = mpSrvUavHeap->GetGPUDescriptorHandleForHeapStart().ptr;
	*(D3D12_GPU_VIRTUAL_ADDRESS*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8 * 0) = heapStart;
	*(D3D12_GPU_VIRTUAL_ADDRESS*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8 * 1) = mpRTPointLightCB->GetGPUVirtualAddress();
	*(D3D12_GPU_VIRTUAL_ADDRESS*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8 * 2) = mpRTCameraCB->GetGPUVirtualAddress();
	*(D3D12_GPU_VIRTUAL_ADDRESS*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8 * 3) = mpRTFrameIndexCB->GetGPUVirtualAddress();

	// Entry 1 - shadow miss program
	memcpy(pData + mShaderTableEntrySize * 1, pRtsoProps->GetShaderIdentifier(kShadowMissShader), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// entry 2 - secondary miss program
	memcpy(pData + mShaderTableEntrySize * 2, pRtsoProps->GetShaderIdentifier(kSecondaryMissShader), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	heapStart = mpSrvUavHeap->GetGPUDescriptorHandleForHeapStart().ptr + //绑定 skybox
		+mpDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 7;
	*(D3D12_GPU_VIRTUAL_ADDRESS*)(pData + mShaderTableEntrySize * 2 + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8 * 0) = heapStart;

	objectToRT = 0;
	uint32_t srvuavBias = 8;
	uint32_t srvuavPerHitSize = 7;
	for (auto it = m_Scene->gameObjectPool.begin(); it != m_Scene->gameObjectPool.end(); it++)
	{
		// 特殊处理剔除光源不加入光追
		std::string objIndex = it->first;
		if (objIndex.find("light") != std::string::npos) {
			continue;
		}
		// Entry 3+i*2 -  shadow hit program
		uint8_t* pHitEntry = pData + mShaderTableEntrySize * (3 + objectToRT * 2); // +2 skips the ray-gen and miss entries
		memcpy(pHitEntry, pRtsoProps->GetShaderIdentifier(kShadowHitGroup), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

		// entry 3+i*2+1 secondary hit program
		pHitEntry = pData + mShaderTableEntrySize * (3 + objectToRT * 2 + 1); // +2 skips the ray-gen and miss entries
		memcpy(pHitEntry, pRtsoProps->GetShaderIdentifier(kSecondaryHitGroup), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		D3D12_GPU_VIRTUAL_ADDRESS heapStart = mpSrvUavHeap->GetGPUDescriptorHandleForHeapStart().ptr
			+ mpDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * (srvuavBias + objectToRT * srvuavPerHitSize);
		*(D3D12_GPU_VIRTUAL_ADDRESS*)(pHitEntry + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8 * 0) = heapStart;
		*(D3D12_GPU_VIRTUAL_ADDRESS*)(pHitEntry + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8 * 1) = mpRTMaterialCBList[objectToRT]->GetGPUVirtualAddress();
		*(D3D12_GPU_VIRTUAL_ADDRESS*)(pHitEntry + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8 * 2) = mpRTPBRMaterialCBList[objectToRT]->GetGPUVirtualAddress();
		*(D3D12_GPU_VIRTUAL_ADDRESS*)(pHitEntry + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8 * 3) = mpRTGameObjectIndexCBList[objectToRT]->GetGPUVirtualAddress();
		*(D3D12_GPU_VIRTUAL_ADDRESS*)(pHitEntry + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8 * 4) = mpRTPointLightCB->GetGPUVirtualAddress();
		*(D3D12_GPU_VIRTUAL_ADDRESS*)(pHitEntry + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8 * 5) = mpRTFrameIndexCB->GetGPUVirtualAddress();

		objectToRT++;
	}
	// Unmap
	mpShaderTable->Unmap(0, nullptr);
}