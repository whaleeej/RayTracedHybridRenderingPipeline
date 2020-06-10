#include "..\inc\GIFilteringRenderer.h"

GIFilteringRenderer::GIFilteringRenderer(int w, int h) :
	Renderer(w, h)
{
}

GIFilteringRenderer::~GIFilteringRenderer()
{
}

void GIFilteringRenderer::LoadResource(std::shared_ptr<Scene> scene, RenderResourceMap& resources)
{
	Renderer::LoadResource(scene, resources);

	gPosition = *resources[RRD_POSITION];
	gPosition_prev = *resources[RRD_POSITION_PREV];
	gAlbedoMetallic = *resources[RRD_ALBEDO_MATALLIC];
	gAlbedoMetallic_prev = *resources[RRD_ALBEDO_MATALLIC_PREV];
	gNormalRoughness = *resources[RRD_NORMAL_ROUGHNESS];
	gNormalRoughness_prev = *resources[RRD_NORMAL_ROUGHNESS_PREV];
	gExtra = *resources[RRD_EXTRA];
	gExtra_prev = *resources[RRD_EXTRA_PREV];

	mRtReflectOutputTexture = *resources[RRD_RT_INDIRECT_SAMPLE];

	noisy_curr = createTex2D_ReadWrite(L"noisy_curr", m_Width, m_Height);
	noisy_prev = createTex2D_ReadWrite(L"noisy_prev", m_Width, m_Height);
	spp_curr = createTex2D_ReadWrite(L"spp_curr", m_Width, m_Height, DXGI_FORMAT_R32_UINT);
	spp_prev = createTex2D_ReadWrite(L"spp_prev", m_Width, m_Height, DXGI_FORMAT_R32_UINT);
	pixel_reproject = createTex2D_ReadWrite(L"pixel_reproject", m_Width, m_Height, DXGI_FORMAT_R32G32_FLOAT);
	pixel_accept = createTex2D_ReadWrite(L"pixel_accept", m_Width, m_Height, DXGI_FORMAT_R32_UINT);
	A_LSQ_matrix = createTex3D_ReadWrite(L"A_LSQ_matrix", WORKSET_WITH_MARGINS_WIDTH, WORKSET_WITH_MARGINS_HEIGHT, BUFFER_COUNT, DXGI_FORMAT_R32_FLOAT);
	lsq_weights = createTex3D_ReadWrite(L"lsq_weights", WORKSET_WITH_MARGINS_WIDTH / BLOCK_EDGE_LENGTH, WORKSET_WITH_MARGINS_HEIGHT / BLOCK_EDGE_LENGTH, BUFFER_COUNT - 3, DXGI_FORMAT_R32G32B32A32_FLOAT);
	feature_scale_minmax = createTex3D_ReadWrite(L"feature_scale_minmax", WORKSET_WITH_MARGINS_WIDTH / BLOCK_EDGE_LENGTH, WORKSET_WITH_MARGINS_HEIGHT / BLOCK_EDGE_LENGTH, 12, DXGI_FORMAT_R32G32B32A32_FLOAT);

	weighted_sum = createTex2D_ReadWrite(L"weighted_output", m_Width, m_Height);

	filtered_curr = createTex2D_ReadWrite(L"filtered_curr", m_Width, m_Height);
	filtered_prev = createTex2D_ReadWrite(L"filtered_prev", m_Width, m_Height);

	resources.emplace(RRD_INDIRECT_FILTERED, std::make_shared<Texture>(filtered_curr));
}

void GIFilteringRenderer::LoadPipeline()
{
	auto device = Application::Get().GetDevice();
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}
	// Disable the multiple sampling for performance and simplicity
	DXGI_SAMPLE_DESC sampleDesc = { 1, 0 }/*Application::Get().GetMultisampleQualityLevels(HDRFormat, D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT)*/;

	// Create the PostBMFRTemporalNoisy_CS Root Signature
	{
		CD3DX12_DESCRIPTOR_RANGE1 descriptorRange[2] = {
			CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 0),
			CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 5, 0)
		};

		CD3DX12_ROOT_PARAMETER1 rootParameters[3];
		rootParameters[0].InitAsDescriptorTable(arraysize(descriptorRange), descriptorRange);
		rootParameters[1].InitAsConstantBufferView(0);
		rootParameters[2].InitAsConstants(4, 1);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(arraysize(rootParameters), rootParameters);

		m_PostBMFRTemporalNoisyRootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

		ComPtr<ID3DBlob> cs;
		ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/HybridRenderingPipeline/PostBMFR_1_TemporalNoisy_CS.cso", &cs));

		struct PostBMFRTemporalNoisyPipelineStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_CS CS;
		} postBMFRTemporalNoisyPipelineStateStream;

		postBMFRTemporalNoisyPipelineStateStream.pRootSignature = m_PostBMFRTemporalNoisyRootSignature.GetRootSignature().Get();
		postBMFRTemporalNoisyPipelineStateStream.CS = CD3DX12_SHADER_BYTECODE(cs.Get());

		D3D12_PIPELINE_STATE_STREAM_DESC postBMFRTemporalNoisyPipelineStateStreamDesc = {
			sizeof(PostBMFRTemporalNoisyPipelineStateStream), &postBMFRTemporalNoisyPipelineStateStream
		};
		ThrowIfFailed(device->CreatePipelineState(&postBMFRTemporalNoisyPipelineStateStreamDesc, IID_PPV_ARGS(&m_PostBMFRTemporalNoisyPipelineState)));
	}

	// Create the PostBMFRQRFactorization_CS Root Signature
	{
		CD3DX12_DESCRIPTOR_RANGE1 descriptorRange[1] = {
			CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 3, 0)
		};

		CD3DX12_ROOT_PARAMETER1 rootParameters[2];
		rootParameters[0].InitAsDescriptorTable(arraysize(descriptorRange), descriptorRange);
		rootParameters[1].InitAsConstants(4, 0);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(arraysize(rootParameters), rootParameters);

		m_PostBMFRQRFactorizationRootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

		ComPtr<ID3DBlob> cs;
		ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/HybridRenderingPipeline/PostBMFR_2_QRFactorization_CS.cso", &cs));

		struct PostBMFRQRFactorizationPipelineStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_CS CS;
		} postBMFRQRFactorizationPipelineStateStream;

		postBMFRQRFactorizationPipelineStateStream.pRootSignature = m_PostBMFRQRFactorizationRootSignature.GetRootSignature().Get();
		postBMFRQRFactorizationPipelineStateStream.CS = CD3DX12_SHADER_BYTECODE(cs.Get());

		D3D12_PIPELINE_STATE_STREAM_DESC postBMFRQRFactorizationPipelineStateStreamDesc = {
			sizeof(PostBMFRQRFactorizationPipelineStateStream), &postBMFRQRFactorizationPipelineStateStream
		};
		ThrowIfFailed(device->CreatePipelineState(&postBMFRQRFactorizationPipelineStateStreamDesc, IID_PPV_ARGS(&m_PostBMFRQRFactorizationPipelineState)));
	}

	// Create the PostBMFRWeightedSum_CS Root Signature
	{
		CD3DX12_DESCRIPTOR_RANGE1 descriptorRange[2] = {
			CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 0),
			CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0)
		};

		CD3DX12_ROOT_PARAMETER1 rootParameters[2];
		rootParameters[0].InitAsDescriptorTable(arraysize(descriptorRange), descriptorRange);
		rootParameters[1].InitAsConstants(4, 0);


		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(arraysize(rootParameters), rootParameters);

		m_PostBMFRWeightedSumRootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

		ComPtr<ID3DBlob> cs;
		ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/HybridRenderingPipeline/PostBMFR_3_WeightedSum_CS.cso", &cs));

		struct PostBMFRWeightedSumStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_CS CS;
		} postBMFRWeightedSumStateStream;

		postBMFRWeightedSumStateStream.pRootSignature = m_PostBMFRWeightedSumRootSignature.GetRootSignature().Get();
		postBMFRWeightedSumStateStream.CS = CD3DX12_SHADER_BYTECODE(cs.Get());

		D3D12_PIPELINE_STATE_STREAM_DESC postBMFRWeightedSumStateStreamDesc = {
			sizeof(postBMFRWeightedSumStateStream), &postBMFRWeightedSumStateStream
		};
		ThrowIfFailed(device->CreatePipelineState(&postBMFRWeightedSumStateStreamDesc, IID_PPV_ARGS(&m_PostBMFRWeightedSumPipelineState)));
	}

	// Create the PostBMFRTemporalFiltered_CS Root Signature
	{
		CD3DX12_DESCRIPTOR_RANGE1 descriptorRange[2] = {
			CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 0),
			CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0)
		};

		CD3DX12_ROOT_PARAMETER1 rootParameters[2];
		rootParameters[0].InitAsDescriptorTable(arraysize(descriptorRange), descriptorRange);
		rootParameters[1].InitAsConstants(4, 0);


		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(arraysize(rootParameters), rootParameters);

		m_PostBMFRTemporalFilteredRootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

		ComPtr<ID3DBlob> cs;
		ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/HybridRenderingPipeline/PostBMFR_4_TemporalFiltered_CS.cso", &cs));

		struct PostBMFRTemporalFilteredStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_CS CS;
		} postBMFRTemporalFilteredStateStream;

		postBMFRTemporalFilteredStateStream.pRootSignature = m_PostBMFRTemporalFilteredRootSignature.GetRootSignature().Get();
		postBMFRTemporalFilteredStateStream.CS = CD3DX12_SHADER_BYTECODE(cs.Get());

		D3D12_PIPELINE_STATE_STREAM_DESC postBMFRTemporalFilteredStateStreamDesc = {
			sizeof(postBMFRTemporalFilteredStateStream), &postBMFRTemporalFilteredStateStream
		};
		ThrowIfFailed(device->CreatePipelineState(&postBMFRTemporalFilteredStateStreamDesc, IID_PPV_ARGS(&m_PostBMFRTemporalFilteredPipelineState)));
	}
}

void GIFilteringRenderer::Update(UpdateEventArgs& e, std::shared_ptr<Scene> scene)
{
	viewProjectMatrix_prev = viewProjectMatrix;
	viewProjectMatrix = scene->m_Camera.get_ViewMatrix() * scene->m_Camera.get_ProjectionMatrix();
}

void GIFilteringRenderer::Render(RenderEventArgs& e, std::shared_ptr<Scene> scene, std::shared_ptr<CommandList> commandList)
{
	// Perform BMFR_1_TemporalNoisy
	{
		commandList->SetPipelineState(m_PostBMFRTemporalNoisyPipelineState);
		commandList->SetComputeRootSignature(m_PostBMFRTemporalNoisyRootSignature);
		uint32_t ppSrvUavOffset = 0;
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gNormalRoughness, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gNormalRoughness_prev, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gPosition, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gPosition_prev, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gExtra, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gExtra_prev, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, noisy_prev, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, spp_prev, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, mRtReflectOutputTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gAlbedoMetallic, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetUnorderedAccessView(0, ppSrvUavOffset++, noisy_curr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->SetUnorderedAccessView(0, ppSrvUavOffset++, spp_curr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->SetUnorderedAccessView(0, ppSrvUavOffset++, pixel_reproject, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->SetUnorderedAccessView(0, ppSrvUavOffset++, pixel_accept, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->SetUnorderedAccessView(0, ppSrvUavOffset++, A_LSQ_matrix, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->SetComputeDynamicConstantBuffer(1, viewProjectMatrix_prev);
		commandList->SetCompute32BitConstants(2, e.FrameNumber);
		commandList->Dispatch(WORKSET_WITH_MARGINS_WIDTH / local_width, WORKSET_WITH_MARGINS_HEIGHT / local_height);
	}

	// Perform BMFR_2_QR
	{
		commandList->SetPipelineState(m_PostBMFRQRFactorizationPipelineState);
		commandList->SetComputeRootSignature(m_PostBMFRQRFactorizationRootSignature);
		uint32_t ppSrvUavOffset = 0;
		commandList->SetUnorderedAccessView(0, ppSrvUavOffset++, lsq_weights, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->SetUnorderedAccessView(0, ppSrvUavOffset++, feature_scale_minmax, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->SetUnorderedAccessView(0, ppSrvUavOffset++, A_LSQ_matrix, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->SetCompute32BitConstants(1, e.FrameNumber);
		commandList->Dispatch(FITTER_GLOBAL / LOCAL_SIZE);
	}

	// Perform BMFR_3_WeightedSum
	{
		commandList->SetPipelineState(m_PostBMFRWeightedSumPipelineState);
		commandList->SetComputeRootSignature(m_PostBMFRWeightedSumRootSignature);
		uint32_t ppSrvUavOffset = 0;
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, lsq_weights, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, feature_scale_minmax, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gNormalRoughness, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gPosition, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, noisy_curr, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gAlbedoMetallic, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetUnorderedAccessView(0, ppSrvUavOffset++, weighted_sum, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->SetCompute32BitConstants(1, e.FrameNumber);
		commandList->Dispatch(WORKSET_WIDTH / local_width, WORKSET_HEIGHT / local_height);
	}

	// Perform BMFR_4_
	{
		commandList->SetPipelineState(m_PostBMFRTemporalFilteredPipelineState);
		commandList->SetComputeRootSignature(m_PostBMFRTemporalFilteredRootSignature);
		uint32_t ppSrvUavOffset = 0;
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, weighted_sum, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, pixel_reproject, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, pixel_accept, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, spp_curr, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, filtered_prev, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetUnorderedAccessView(0, ppSrvUavOffset++, filtered_curr, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->SetCompute32BitConstants(1, e.FrameNumber);
		commandList->Dispatch(WORKSET_WIDTH / local_width, WORKSET_HEIGHT / local_height);
	}
}

void GIFilteringRenderer::PreRender(RenderResourceMap& resources)
{
	Renderer::PreRender(resources);
	gPosition = *resources[RRD_POSITION];
	gPosition_prev = *resources[RRD_POSITION_PREV];
	gAlbedoMetallic = *resources[RRD_ALBEDO_MATALLIC];
	gAlbedoMetallic_prev = *resources[RRD_ALBEDO_MATALLIC_PREV];
	gNormalRoughness = *resources[RRD_NORMAL_ROUGHNESS];
	gNormalRoughness_prev = *resources[RRD_NORMAL_ROUGHNESS_PREV];
	gExtra = *resources[RRD_EXTRA];
	gExtra_prev = *resources[RRD_EXTRA_PREV];

	mRtReflectOutputTexture = *resources[RRD_RT_INDIRECT_SAMPLE];

	auto swapCurrPrevTexture = [](Texture& t1, Texture& t2) {
		Texture tmp = t2;
		t2 = t1;
		t1 = tmp;
	};
	swapCurrPrevTexture(noisy_prev, noisy_curr);
	swapCurrPrevTexture(spp_prev, spp_curr);
	swapCurrPrevTexture(filtered_prev, filtered_curr);
}

RenderResourceMap* GIFilteringRenderer::PostRender()
{
	(*m_Resources)[RRD_INDIRECT_FILTERED] = std::make_shared<Texture>(filtered_curr);
	return Renderer::PostRender();
}
