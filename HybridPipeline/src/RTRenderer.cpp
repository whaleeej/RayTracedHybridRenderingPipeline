#include "RTRenderer.h"

#define arraySize(a) (sizeof(a)/sizeof(a[0]))
#define align_to(_alignment, _val) (((_val + _alignment - 1) / _alignment) * _alignment)

static dxc::DxcDllSupport gDxcDllHelper;

static const D3D12_HEAP_PROPERTIES kUploadHeapProps =
{
	D3D12_HEAP_TYPE_UPLOAD,
	D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	D3D12_MEMORY_POOL_UNKNOWN,
	0,
	0,
};

static const D3D12_HEAP_PROPERTIES kDefaultHeapProps =
{
	D3D12_HEAP_TYPE_DEFAULT,
	D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	D3D12_MEMORY_POOL_UNKNOWN,
	0,
	0
};

static const WCHAR* kRayGenShader = L"rayGen";
static const WCHAR* kMissShader = L"miss";
static const WCHAR* kShadowMiss = L"shadowMiss";
static const WCHAR* kTriangleChs = L"triangleChs";
static const WCHAR* kTriHitGroup = L"TriHitGroup";
static const WCHAR* kShadowChs	= L"shadowChs";
static const WCHAR* kShadowHitGroup = L"ShadowHitGroup";

struct DxilLibrary
{
	DxilLibrary(ID3DBlobPtr pBlob, const WCHAR* entryPoint[], uint32_t entryPointCount) : pShaderBlob(pBlob)
	{
		stateSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
		stateSubobject.pDesc = &dxilLibDesc;

		dxilLibDesc = {};
		exportDesc.resize(entryPointCount);
		exportName.resize(entryPointCount);
		if (pBlob)
		{
			dxilLibDesc.DXILLibrary.pShaderBytecode = pBlob->GetBufferPointer();
			dxilLibDesc.DXILLibrary.BytecodeLength = pBlob->GetBufferSize();
			dxilLibDesc.NumExports = entryPointCount;
			dxilLibDesc.pExports = exportDesc.data();

			for (uint32_t i = 0; i < entryPointCount; i++)
			{
				exportName[i] = entryPoint[i];
				exportDesc[i].Name = exportName[i].c_str();
				exportDesc[i].Flags = D3D12_EXPORT_FLAG_NONE;
				exportDesc[i].ExportToRename = nullptr;
			}
		}
	};

	DxilLibrary() : DxilLibrary(nullptr, nullptr, 0) {}

	D3D12_DXIL_LIBRARY_DESC dxilLibDesc = {};
	D3D12_STATE_SUBOBJECT stateSubobject{};
	ID3DBlobPtr pShaderBlob;
	std::vector<D3D12_EXPORT_DESC> exportDesc;
	std::vector<std::wstring> exportName;
};

ID3D12RootSignaturePtr createRootSignature(ID3D12Device5Ptr pDevice, const D3D12_ROOT_SIGNATURE_DESC& desc)
{
	ID3DBlobPtr pSigBlob;
	ID3DBlobPtr pErrorBlob;
	HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &pSigBlob, &pErrorBlob);
	if (FAILED(hr))
	{
		//std::string msg = convertBlobToString(pErrorBlob.GetInterfacePtr());
		//msgBox(msg);
		ThrowIfFailed(hr);
		return nullptr;
	}
	ID3D12RootSignaturePtr pRootSig;
	ThrowIfFailed(pDevice->CreateRootSignature(0, pSigBlob->GetBufferPointer(), pSigBlob->GetBufferSize(), IID_PPV_ARGS(&pRootSig)));
	return pRootSig;
}

struct RootSignatureDesc
{
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	std::vector<D3D12_DESCRIPTOR_RANGE> range;
	std::vector<D3D12_ROOT_PARAMETER> rootParams;
};

RootSignatureDesc createRayGenRootDesc()
{
	// Create the root-signature
	RootSignatureDesc desc;
	desc.range.resize(2);
	// gOutput
	desc.range[0].BaseShaderRegister = 0;
	desc.range[0].NumDescriptors = 1;
	desc.range[0].RegisterSpace = 0;
	desc.range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	desc.range[0].OffsetInDescriptorsFromTableStart = 0;

	// gRtScene and Gbuffer
	desc.range[1].BaseShaderRegister = 0;
	desc.range[1].NumDescriptors = 2;
	desc.range[1].RegisterSpace = 0;
	desc.range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[1].OffsetInDescriptorsFromTableStart = 1;

	desc.rootParams.resize(1);
	desc.rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	desc.rootParams[0].DescriptorTable.NumDescriptorRanges = 2;
	desc.rootParams[0].DescriptorTable.pDescriptorRanges = desc.range.data();

	// Create the desc
	desc.desc.NumParameters = 1;
	desc.desc.pParameters = desc.rootParams.data();
	desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	return desc;
}

RootSignatureDesc createTriangleHitRootDesc()
{
	RootSignatureDesc desc;
	desc.rootParams.resize(1);
	desc.rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	desc.rootParams[0].Descriptor.RegisterSpace = 0;
	desc.rootParams[0].Descriptor.ShaderRegister = 0;

	desc.desc.NumParameters = 1;
	desc.desc.pParameters = desc.rootParams.data();
	desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	return desc;
}

struct HitProgram
{
	HitProgram(LPCWSTR ahsExport, LPCWSTR chsExport, const std::wstring& name) : exportName(name)
	{
		desc = {};
		desc.AnyHitShaderImport = ahsExport;
		desc.ClosestHitShaderImport = chsExport;
		desc.HitGroupExport = exportName.c_str();

		subObject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
		subObject.pDesc = &desc;
	}

	std::wstring exportName;
	D3D12_HIT_GROUP_DESC desc;
	D3D12_STATE_SUBOBJECT subObject;
};

struct ExportAssociation
{
	ExportAssociation(const WCHAR* exportNames[], uint32_t exportCount, const D3D12_STATE_SUBOBJECT* pSubobjectToAssociate)
	{
		association.NumExports = exportCount;
		association.pExports = exportNames;
		association.pSubobjectToAssociate = pSubobjectToAssociate;

		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		subobject.pDesc = &association;
	}

	D3D12_STATE_SUBOBJECT subobject = {};
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION association = {};
};

struct LocalRootSignature
{
	LocalRootSignature(ID3D12Device5Ptr pDevice, const D3D12_ROOT_SIGNATURE_DESC& desc)
	{
		pRootSig = createRootSignature(pDevice, desc);
		pInterface = pRootSig.GetInterfacePtr();
		subobject.pDesc = &pInterface;
		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	}

	ID3D12RootSignaturePtr pRootSig;
	ID3D12RootSignature* pInterface = nullptr;
	D3D12_STATE_SUBOBJECT subobject = {};
};

struct GlobalRootSignature
{
	GlobalRootSignature(ID3D12Device5Ptr pDevice, const D3D12_ROOT_SIGNATURE_DESC& desc)
	{
		pRootSig = createRootSignature(pDevice, desc);
		pInterface = pRootSig.GetInterfacePtr();
		subobject.pDesc = &pInterface;
		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	}

	ID3D12RootSignaturePtr pRootSig;
	ID3D12RootSignature* pInterface = nullptr;
	D3D12_STATE_SUBOBJECT subobject = {};
};

struct ShaderConfig
{
	ShaderConfig(uint32_t maxAttributeSizeInBytes, uint32_t maxPayloadSizeInBytes)
	{
		shaderConfig.MaxAttributeSizeInBytes = maxAttributeSizeInBytes;
		shaderConfig.MaxPayloadSizeInBytes = maxPayloadSizeInBytes;

		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
		subobject.pDesc = &shaderConfig;
	}

	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
	D3D12_STATE_SUBOBJECT subobject = {};
};

struct PipelineConfig
{
	PipelineConfig(uint32_t maxTraceRecursionDepth)
	{
		config.MaxTraceRecursionDepth = maxTraceRecursionDepth;

		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
		subobject.pDesc = &config;
	}

	D3D12_RAYTRACING_PIPELINE_CONFIG config = {};
	D3D12_STATE_SUBOBJECT subobject = {};
};

ID3DBlobPtr compileLibrary(const WCHAR* filename, const WCHAR* targetString)
{
	// Initialize the helper
	ThrowIfFailed(gDxcDllHelper.Initialize());
	IDxcCompilerPtr pCompiler;
	IDxcLibraryPtr pLibrary;
	ThrowIfFailed(gDxcDllHelper.CreateInstance(CLSID_DxcCompiler, &pCompiler));
	ThrowIfFailed(gDxcDllHelper.CreateInstance(CLSID_DxcLibrary, &pLibrary));

	// Open and read the file
	std::ifstream shaderFile(filename);
	if (shaderFile.good() == false)
	{
		ThrowIfFailed(-1);
		return nullptr;
	}
	std::stringstream strStream;
	strStream << shaderFile.rdbuf();
	std::string shader = strStream.str();

	// Create blob from the string
	IDxcBlobEncodingPtr pTextBlob;
	ThrowIfFailed(pLibrary->CreateBlobWithEncodingFromPinned((LPBYTE)shader.c_str(), (uint32_t)shader.size(), 0, &pTextBlob));

	// Compile
	IDxcOperationResultPtr pResult;
	ThrowIfFailed(pCompiler->Compile(pTextBlob, filename, L"", targetString, nullptr, 0, nullptr, 0, nullptr, &pResult));

	// Verify the result
	HRESULT resultCode;
	ThrowIfFailed(pResult->GetStatus(&resultCode));
	if (FAILED(resultCode))
	{
		ThrowIfFailed(-1);
		/*IDxcBlobEncodingPtr pError;
		ThrowIfFailed(pResult->GetErrorBuffer(&pError));
		std::string log = convertBlobToString(pError.GetInterfacePtr());
		assert(false, "Assert: shader file not good");*/
		return nullptr;
	}

	MAKE_SMART_COM_PTR(IDxcBlob);
	IDxcBlobPtr pBlob;
	ThrowIfFailed(pResult->GetResult(&pBlob));
	return pBlob;
}

DxilLibrary createDxilLibrary()
{
	// Compile the shader
	ID3DBlobPtr pRayGenShader = compileLibrary(L"HybridPipeline/shaders/RayTracing.hlsl", L"lib_6_3");
	const WCHAR* entryPoints[] = { kRayGenShader, kMissShader, kTriangleChs, kShadowMiss, kShadowChs };
	return DxilLibrary(pRayGenShader, entryPoints, arraySize(entryPoints));
}

RTRenderer::RTRenderer(uint64_t width, uint64_t height)
{
	m_width = width;
	m_height = height;
	mMeshCount = 0;
}

void RTRenderer::createAccelerationStructures(std::vector<Mesh*> meshes)
{
	auto commandQueue = Application::Get().GetCommandQueue(); // default list_type_direct
	auto commandList = commandQueue->GetCommandList();
	std::vector<AccelerationStructureBuffers> blasBuffers;
	blasBuffers.resize(meshes.size());

	//TODO: instancing
	for (size_t i = 0; i < meshes.size(); i++) {
		blasBuffers[i]= createBottomLevelAS(*commandList, meshes[i]);
		this->mpBottomLevelAs.push_back(blasBuffers[i].pResult);
	}

	// create  the TLAS
	std::vector<XMMATRIX> t_trans;
	for (size_t i = 0; i < meshes.size(); i++) {
		XMMATRIX m  = {1.0f ,1.0f , 1.0f , 1.0f , 
									1.0f , 1.0f , 1.0f , 1.0f , 
									1.0f , 1.0f , 1.0f , 1.0f , 
									1.0f , 1.0f , 1.0f , 1.0f };
		t_trans.push_back(m);
	}
	buildTopLevelAS(*commandList, t_trans, false);

	// execute on commandQueue and wait for execution end
	uint64_t fenceValue = commandQueue->ExecuteCommandList(commandList);
	commandQueue->WaitForFenceValue(fenceValue);
}

void RTRenderer::createRtPipelineState()
{
	auto mpDevice = Application::Get().GetDevice();
	// total is 13
	//  1 for DXIL library    
	//  2 for the hit-groups (triangle hit group, shadow-hit group)
	//  2 for RayGen root-signature (root-signature and the subobject association)
	//  2 for triangle hit-program root-signature (root-signature and the subobject association)
	//  2 for shadow-program and miss root-signature (root-signature and the subobject association)
	//  2 for shader config (shared between all programs. 1 for the config, 1 for association)
	//  1 for pipeline config
	//  1 for the global root signature

	std::array<D3D12_STATE_SUBOBJECT, 13> subobjects;
	uint32_t index = 0;

	// Create the DXIL library
	DxilLibrary dxilLib = createDxilLibrary();
	subobjects[index++] = dxilLib.stateSubobject; // 0 Library

	// Create the triangle HitProgram
	HitProgram triHitProgram(nullptr, kTriangleChs, kTriHitGroup);
	subobjects[index++] = triHitProgram.subObject; // 1 Triangle Hit Group

	// Create the shadow-ray hit group
	HitProgram shadowHitProgram(nullptr, kShadowChs, kShadowHitGroup);
	subobjects[index++] = shadowHitProgram.subObject; // 2 Shadow Hit Group

	// Create the ray-gen root-signature and association
	LocalRootSignature rgsRootSignature(mpDevice.Get(), createRayGenRootDesc().desc);
	subobjects[index] = rgsRootSignature.subobject; // 3 Ray Gen Root Sig

	uint32_t rgsRootIndex = index++; // 3
	ExportAssociation rgsRootAssociation(&kRayGenShader, 1, &(subobjects[rgsRootIndex]));
	subobjects[index++] = rgsRootAssociation.subobject; // 4 Associate Root Sig to RGS

	// Create the tri hit root-signature and association
	D3D12_ROOT_SIGNATURE_DESC emptyDesc = {};
	emptyDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	LocalRootSignature triHitRootSignature(mpDevice.Get(), emptyDesc);
	subobjects[index] = triHitRootSignature.subobject; // 5 Triangle Hit Root Sig

	uint32_t triHitRootIndex = index++; // 5
	ExportAssociation triHitRootAssociation(&kTriangleChs, 1, &(subobjects[triHitRootIndex]));
	subobjects[index++] = triHitRootAssociation.subobject; // 6 Associate Triangle Root Sig to Triangle Hit Group

	// Create the empty root-signature and associate it with the primary miss-shader and the shadow programs
	LocalRootSignature emptyRootSignature(mpDevice.Get(), emptyDesc);
	subobjects[index] = emptyRootSignature.subobject; // 7 Empty Root Sig for Plane Hit Group and Miss

	uint32_t emptyRootIndex = index++; // 7
	const WCHAR* emptyRootExport[] = { kMissShader, kShadowChs, kShadowMiss };
	ExportAssociation emptyRootAssociation(emptyRootExport, arraySize(emptyRootExport), &(subobjects[emptyRootIndex]));
	subobjects[index++] = emptyRootAssociation.subobject; // 8 Associate empty root sig to Plane Hit Group and Miss shader

	// Bind the payload size to all programs
	ShaderConfig primaryShaderConfig(sizeof(float) * 2, sizeof(float) * 3);
	subobjects[index] = primaryShaderConfig.subobject; // 9

	uint32_t primaryShaderConfigIndex = index++; //9
	const WCHAR* primaryShaderExports[] = { kRayGenShader, kMissShader, kTriangleChs, kShadowMiss, kShadowChs };
	ExportAssociation primaryConfigAssociation(primaryShaderExports, arraySize(primaryShaderExports), &(subobjects[primaryShaderConfigIndex]));
	subobjects[index++] = primaryConfigAssociation.subobject; // 10 Associate shader config to all programs

	// Create the pipeline config
	PipelineConfig config(0); // maxRecursionDepth - 1 TraceRay() from the ray-gen, 1 TraceRay() from the primary hit-shader
	subobjects[index++] = config.subobject; // 11

	// Create the global root signature and store the empty signature
	GlobalRootSignature root(mpDevice.Get(), {});
	mpEmptyRootSig = root.pRootSig;
	mpEmptyRootSig->SetName(std::wstring(L"mpEmptyRootSig").c_str());
	subobjects[index++] = root.subobject; // 12

	// Create the state
	D3D12_STATE_OBJECT_DESC desc;
	desc.NumSubobjects = index; // 13
	desc.pSubobjects = subobjects.data();
	desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

	ThrowIfFailed(mpDevice->CreateStateObject(&desc, IID_PPV_ARGS(&mpPipelineState)));
	mpPipelineState->SetName(std::wstring(L"mpPipelineState").c_str());
}

void RTRenderer::createShaderResources(const Texture& normalTexture)
{
	auto mpDevice = Application::Get().GetDevice();

	// Create the output resource. The dimensions and format should match the swap-chain
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.DepthOrArraySize = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // The backbuffer is actually DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, but sRGB formats can't be used with UAVs. We will convert to sRGB ourselves in the shader
	resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	resDesc.Width = m_width;
	resDesc.Height = m_height;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	//ThrowIfFailed(mpDevice->CreateCommittedResource(&kDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, IID_PPV_ARGS(&mpOutputResource))); // Starting as copy-source to simplify onFrameRender()
	mpOutputTexture = std::make_shared<Texture>(resDesc, nullptr, TextureUsage::RenderTarget, L"OutputTexture");

	// Create an SRV/UAV descriptor heap. Need 3, 0 for UAV as the gOutput, 1&2 as the tlas&gbuffer texture
	D3D12_DESCRIPTOR_HEAP_DESC desc_heap_desc = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 3, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE };
	ThrowIfFailed(mpDevice->CreateDescriptorHeap(&desc_heap_desc, IID_PPV_ARGS(&mpSrvUavHeap)));
	mpSrvUavHeap->SetName(std::wstring(L"mpSrvUavHeap").c_str());

	// Create the UAV. Based on the root signature we created it should be the first entry
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	mpDevice->CreateUnorderedAccessView(mpOutputTexture->GetD3D12Resource().Get(), nullptr, &uavDesc, mpSrvUavHeap->GetCPUDescriptorHandleForHeapStart());

	// tlas
	// Create the TLAS SRV and gbuffer srv
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc_tlas = {};
	srvDesc_tlas.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc_tlas.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc_tlas.RaytracingAccelerationStructure.Location = mTopLevelBuffers.pResult->GetGPUVirtualAddress();
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = mpSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
	srvHandle.ptr += mpDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	mpDevice->CreateShaderResourceView(nullptr, &srvDesc_tlas, srvHandle);

	// gbuffer
	srvHandle.ptr += mpDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_SHADER_RESOURCE_VIEW_DESC optDesc = {};
	optDesc.Format = normalTexture.GetD3D12ResourceDesc().Format;
	optDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	optDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	optDesc.Texture2D.MipLevels = 1;
	optDesc.Texture2D.MostDetailedMip = 0;
	optDesc.Texture2D.PlaneSlice = 0;
	optDesc.Texture2D.ResourceMinLODClamp = 0;
	mpDevice->CreateShaderResourceView(normalTexture.GetD3D12Resource().Get(), &optDesc, srvHandle);
}

void RTRenderer::createShaderTable()
{
	/** The shader-table layout is as follows:
		Entry 0 - Ray-gen program
		Entry 1 - Miss program for the primary ray
		Entry 2 - Miss program for the shadow ray
		Entries i*2+2,i*2+3 - Hit programs for triangle 0 (primary followed by shadow)
		All entries in the shader-table must have the same size, so we will choose it base on the largest required entry.
		The triangle primary-ray hit program requires the largest entry - sizeof(program identifier) + 8 bytes for the constant-buffer root descriptor.
		The entry size must be aligned up to D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT
	*/
	//TODO: 
	uint32_t desc_max_size = 4;
	uint32_t ray_gen_count = 1;
	uint32_t hit_type_count = 2;
	uint32_t miss_type_count = hit_type_count;

	// Calculate the size and create the buffer
	mShaderTableEntrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	mShaderTableEntrySize += desc_max_size; // The hit shader constant-buffer descriptor
	mShaderTableEntrySize = align_to(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, mShaderTableEntrySize);
	uint32_t shaderTableSize = mShaderTableEntrySize * (ray_gen_count + miss_type_count + hit_type_count * mMeshCount);

	// For simplicity, we create the shader-table on the upload heap. You can also create it on the default heap
	mpShaderTable = CommandList::createBuffer(shaderTableSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
	mpShaderTable->SetName(std::wstring(L"mpShaderTable").c_str());

	// Map the buffer
	uint8_t* pData;
	ThrowIfFailed(mpShaderTable->Map(0, nullptr, (void**)& pData));

	MAKE_SMART_COM_PTR(ID3D12StateObjectProperties);
	ID3D12StateObjectPropertiesPtr pRtsoProps;
	mpPipelineState->QueryInterface(IID_PPV_ARGS(&pRtsoProps));

	uint32_t ray_bias_count = 0;

	// Entry 0 - ray-gen program ID and descriptor data
	memcpy(pData + mShaderTableEntrySize * (ray_bias_count++), pRtsoProps->GetShaderIdentifier(kRayGenShader), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	uint64_t heapStart = mpSrvUavHeap->GetGPUDescriptorHandleForHeapStart().ptr;
	*(uint64_t*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = heapStart;

	// Entry 1 - primary ray miss
	memcpy(pData + mShaderTableEntrySize * (ray_bias_count++), pRtsoProps->GetShaderIdentifier(kMissShader), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// Entry 2 - shadow-ray miss
	memcpy(pData + mShaderTableEntrySize * (ray_bias_count++), pRtsoProps->GetShaderIdentifier(kShadowMiss), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	for (size_t i = 0; i < mMeshCount; i++)
	{
		// tri ray hit
		uint8_t* pEntry = pData + mShaderTableEntrySize * (ray_bias_count++);
		memcpy(pEntry, pRtsoProps->GetShaderIdentifier(kTriHitGroup), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		
		// shadow ray hit
		pEntry = pData + mShaderTableEntrySize * (ray_bias_count++);
		memcpy(pEntry, pRtsoProps->GetShaderIdentifier(kShadowHitGroup), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	}
	
	// Unmap
	mpShaderTable->Unmap(0, nullptr);
}

void RTRenderer::onRender(CommandList& commandList, std::vector<XMMATRIX> trans)
{
	// Refit the top-level acceleration structure
	buildTopLevelAS(commandList, trans, true);

	// Let's raytrace
	commandList.TransitionBarrier(*mpOutputTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
	//commandList.resourceBarrier( mpOutputResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	D3D12_DISPATCH_RAYS_DESC raytraceDesc = {};
	raytraceDesc.Width = m_width;
	raytraceDesc.Height = m_height;
	raytraceDesc.Depth = 0;

	// RayGen is the first entry in the shader-table
	raytraceDesc.RayGenerationShaderRecord.StartAddress = mpShaderTable->GetGPUVirtualAddress() + 0 * mShaderTableEntrySize;
	raytraceDesc.RayGenerationShaderRecord.SizeInBytes = mShaderTableEntrySize;

	// Miss is the second entry in the shader-table
	size_t missOffset = 1 * mShaderTableEntrySize;
	raytraceDesc.MissShaderTable.StartAddress = mpShaderTable->GetGPUVirtualAddress() + missOffset;
	raytraceDesc.MissShaderTable.StrideInBytes = mShaderTableEntrySize;
	raytraceDesc.MissShaderTable.SizeInBytes = mShaderTableEntrySize * 2;   // 2 miss-entries

	// Hit is the fourth entry in the shader-table
	size_t hitOffset = 3 * mShaderTableEntrySize;
	raytraceDesc.HitGroupTable.StartAddress = mpShaderTable->GetGPUVirtualAddress() + hitOffset;
	raytraceDesc.HitGroupTable.StrideInBytes = mShaderTableEntrySize;
	raytraceDesc.HitGroupTable.SizeInBytes = mShaderTableEntrySize * trans.size() * 2;    //  model*2 hit table

		// Dispatch
	commandList.SetComputeRootSignature(mpEmptyRootSig);
	// Bind the empty root signature
	commandList.SetPipelineState1(mpPipelineState.GetInterfacePtr());

	commandList.DispatchRays(&raytraceDesc);

	// Copy the results to the back-buffer
	//commandList.resourceBarrier(mpOutputResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	//commandList.TransitionBarrier(*mpOutputTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);

}

RTRenderer::AccelerationStructureBuffers RTRenderer::createBottomLevelAS(CommandList& commandList, Mesh* mesh)
{
	auto pDevice = Application::Get().GetDevice();
	
	const uint32_t geometryCount =1;

	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geomDesc;
	geomDesc.resize(geometryCount);

	for (uint32_t i = 0; i < geometryCount; i++)
	{
		// vertex buffer
		geomDesc[i].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geomDesc[i].Triangles.VertexBuffer.StartAddress = mesh->m_VertexBuffer.GetD3D12Resource()->GetGPUVirtualAddress();
		geomDesc[i].Triangles.VertexBuffer.StrideInBytes = mesh->m_VertexBuffer.GetVertexStride();
		geomDesc[i].Triangles.VertexCount = mesh->m_VertexBuffer.GetNumVertices();
		geomDesc[i].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		// indices buffer
		geomDesc[i].Triangles.IndexBuffer = mesh->m_IndexBuffer.GetD3D12Resource()->GetGPUVirtualAddress();
		geomDesc[i].Triangles.IndexCount = mesh->m_IndexCount;
		geomDesc[i].Triangles.IndexFormat = mesh->m_IndexBuffer.GetIndexFormat();
		geomDesc[i].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	}

	// Get the size requirements for the scratch and AS buffers
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.NumDescs = geometryCount;
	inputs.pGeometryDescs = geomDesc.data();
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	// Create the buffers. They need to support UAV, and since we are going to immediately use them, we create them with an unordered-access state
	AccelerationStructureBuffers buffers;
	buffers.pScratch = CommandList::createBuffer(info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, kDefaultHeapProps);
	buffers.pResult = CommandList::createBuffer(info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, kDefaultHeapProps);

	// Create the bottom-level AS
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
	asDesc.DestAccelerationStructureData = buffers.pResult->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = buffers.pScratch->GetGPUVirtualAddress();

	commandList.BuildRaytracingAccelerationStructure(&asDesc);

	// We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = buffers.pResult;
	commandList.UAVBarrier(buffers.pResult);

	return buffers;
}

void RTRenderer::buildTopLevelAS(CommandList& commandList, std::vector<XMMATRIX> trans, bool update)
{
	auto pDevice = Application::Get().GetDevice();

	mMeshCount = trans.size();

	// First, get the size of the TLAS buffers and create them
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	inputs.NumDescs = trans.size();
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	if (update)
	{
		// If this a request for an update, then the TLAS was already used in a DispatchRay() call. We need a UAV barrier to make sure the read operation ends before updating the buffer
		commandList.UAVBarrier(mTopLevelBuffers.pResult);
	}
	else
	{
		// If this is not an update operation then we need to create the buffers, otherwise we will refit in-place
		mTopLevelBuffers.pScratch = commandList.createBuffer(info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kDefaultHeapProps);
		mTopLevelBuffers.pResult = commandList.createBuffer(info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, kDefaultHeapProps);
		mTopLevelBuffers.pInstanceDesc = commandList.createBuffer(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * 3, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
		mTopLevelBuffers.pResult->SetName(std::wstring(L"mTopLevelBuffers.pBuffer").c_str());
		mTlasSize = info.ResultDataMaxSizeInBytes;
	}

	// Map the instance desc buffer
	D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs;
	mTopLevelBuffers.pInstanceDesc->Map(0, nullptr, (void**)& instanceDescs);
	ZeroMemory(instanceDescs, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * trans.size());

	// The transformation matrices for the instances

	// The InstanceContributionToHitGroupIndex is set based on the shader-table layout specified in createShaderTable()
	for (uint32_t i = 0; i < trans.size(); i++)
	{
		instanceDescs[i].InstanceID = i; // This value will be exposed to the shader via InstanceID()
		instanceDescs[i].InstanceContributionToHitGroupIndex = 2 * i; 
		instanceDescs[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		memcpy(instanceDescs[i].Transform, trans[i].r, sizeof(instanceDescs[i].Transform));
		instanceDescs[i].AccelerationStructure = mpBottomLevelAs[i]->GetGPUVirtualAddress();
		instanceDescs[i].InstanceMask = 0xFF;
	}

	// Unmap
	mTopLevelBuffers.pInstanceDesc->Unmap(0, nullptr);

	// Create the TLAS
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
	asDesc.Inputs.InstanceDescs = mTopLevelBuffers.pInstanceDesc->GetGPUVirtualAddress();
	asDesc.DestAccelerationStructureData = mTopLevelBuffers.pResult->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = mTopLevelBuffers.pScratch->GetGPUVirtualAddress();

	// If this is an update operation, set the source buffer and the perform_update flag
	if (update)
	{
		asDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
		asDesc.SourceAccelerationStructureData = mTopLevelBuffers.pResult->GetGPUVirtualAddress();
	}

	commandList.BuildRaytracingAccelerationStructure(&asDesc);

	// We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
	commandList.UAVBarrier(mTopLevelBuffers.pResult);
}