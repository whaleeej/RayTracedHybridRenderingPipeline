#include "ShadowFilteringRenderer.h"

ShadowFilteringRenderer::ShadowFilteringRenderer(int w, int h) :
	Renderer(w, h)
{

}

ShadowFilteringRenderer::~ShadowFilteringRenderer()
{
}

void ShadowFilteringRenderer::LoadResource(std::shared_ptr<Scene> scene, RenderResourceMap& resources)
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

	mRtShadowOutputTexture = *resources[RRD_RT_SHADOW_SAMPLE];

	col_acc = createTex2D_ReadWrite(L"col_acc", m_Width, m_Height);
	moment_acc = createTex2D_ReadWrite(L"moment_acc", m_Width, m_Height);
	his_length = createTex2D_ReadWrite(L"his_length", m_Width, m_Height);
	col_acc_prev = createTex2D_ReadWrite(L"col_acc_prev", m_Width, m_Height);
	moment_acc_prev = createTex2D_ReadWrite(L"moment_acc_prev", m_Width, m_Height);
	his_length_prev = createTex2D_ReadWrite(L"his_length_prev", m_Width, m_Height);

	color_inout[0] = createTex2D_ReadWrite(L"color_inout[0]", m_Width, m_Height);
	color_inout[1] = createTex2D_ReadWrite(L"color_inout[1]", m_Width, m_Height);
	variance_inout[0] = createTex2D_ReadWrite(L"variance_inout[0]", m_Width, m_Height);
	variance_inout[1] = createTex2D_ReadWrite(L"variance_inout[1]", m_Width, m_Height);

	resources.emplace(RRD_SHADOW_ACC, std::make_shared<Texture>(col_acc));
}

void ShadowFilteringRenderer::LoadPipeline()
{
	auto device = Application::Get().GetDevice();
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}
	DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };

	// Create the PostSVGFTempral_CS Root Signature
	{
		CD3DX12_DESCRIPTOR_RANGE1 descriptorRange[2] = {
			CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 12, 0),
			CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 4, 0) };

		CD3DX12_ROOT_PARAMETER1 rootParameters[2];
		rootParameters[0].InitAsDescriptorTable(arraysize(descriptorRange), descriptorRange);
		rootParameters[1].InitAsConstantBufferView(0, 0);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(arraysize(rootParameters), rootParameters);

		m_PostSVGFTemporalRootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

		// Create the PostProcessing PSO
		ComPtr<ID3DBlob> cs;
		ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/HybridRenderingPipeline/PostSVGFTemporal_CS.cso", &cs));

		struct PostProcessingPipelineStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_CS CS;
		} postProcessingPipelineStateStream;

		postProcessingPipelineStateStream.pRootSignature = m_PostSVGFTemporalRootSignature.GetRootSignature().Get();
		postProcessingPipelineStateStream.CS = CD3DX12_SHADER_BYTECODE(cs.Get());

		D3D12_PIPELINE_STATE_STREAM_DESC postProcessingPipelineStateStreamDesc = {
			sizeof(PostProcessingPipelineStateStream), &postProcessingPipelineStateStream
		};
		ThrowIfFailed(device->CreatePipelineState(&postProcessingPipelineStateStreamDesc, IID_PPV_ARGS(&m_PostSVGFTemporalPipelineState)));
	}

	// Create the PostATrous_CS Root Signature
	{
		CD3DX12_DESCRIPTOR_RANGE1 descriptorRange[2] = {
			CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 0),
			CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, 0)
		};

		CD3DX12_ROOT_PARAMETER1 rootParameters[2];
		rootParameters[0].InitAsDescriptorTable(arraysize(descriptorRange), descriptorRange);
		rootParameters[1].InitAsConstants(4, 0);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(arraysize(rootParameters), rootParameters);

		m_PostSVGFATrousRootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

		ComPtr<ID3DBlob> cs;
		ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/HybridRenderingPipeline/PostSVGFATrous_CS.cso", &cs));

		struct PostATrousPipelineStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_CS CS;
		} postATrousPipelineStateStream;

		postATrousPipelineStateStream.pRootSignature = m_PostSVGFATrousRootSignature.GetRootSignature().Get();
		postATrousPipelineStateStream.CS = CD3DX12_SHADER_BYTECODE(cs.Get());

		D3D12_PIPELINE_STATE_STREAM_DESC postAtrousPipelineStateStreamDesc = {
			sizeof(PostATrousPipelineStateStream), &postATrousPipelineStateStream
		};
		ThrowIfFailed(device->CreatePipelineState(&postAtrousPipelineStateStreamDesc, IID_PPV_ARGS(&m_PostSVGFATrousPipelineState)));
	}
}

void ShadowFilteringRenderer::Update(UpdateEventArgs& e, std::shared_ptr<Scene> scene)
{
	viewProjectMatrix_prev = viewProjectMatrix;
	viewProjectMatrix = scene->m_Camera.get_ViewMatrix() * scene->m_Camera.get_ProjectionMatrix();
}

void ShadowFilteringRenderer::Render(RenderEventArgs& e, std::shared_ptr<Scene> scene, std::shared_ptr<CommandList> commandList)
{
	// Perform Temporal Accumulation and Variance Estimation
	{
		commandList->SetPipelineState(m_PostSVGFTemporalPipelineState);
		commandList->SetComputeRootSignature(m_PostSVGFTemporalRootSignature);
		uint32_t ppSrvUavOffset = 0;
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gPosition, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gAlbedoMetallic, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gNormalRoughness, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gExtra, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gPosition_prev, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gAlbedoMetallic_prev, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gNormalRoughness_prev, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gExtra_prev, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, col_acc_prev, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, moment_acc_prev, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, his_length_prev, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, mRtShadowOutputTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetUnorderedAccessView(0, ppSrvUavOffset++, col_acc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->SetUnorderedAccessView(0, ppSrvUavOffset++, moment_acc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->SetUnorderedAccessView(0, ppSrvUavOffset++, his_length, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->SetUnorderedAccessView(0, ppSrvUavOffset++, variance_inout[1], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->SetComputeDynamicConstantBuffer(1, viewProjectMatrix_prev);
		commandList->Dispatch(m_Width / local_width, m_Height / local_height);
	}

	// Perform ATrous Iteration
	{
		commandList->SetPipelineState(m_PostSVGFATrousPipelineState);
		commandList->SetComputeRootSignature(m_PostSVGFATrousRootSignature);
		uint32_t ppSrvUavOffset = 0;
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gPosition, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gAlbedoMetallic, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gNormalRoughness, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gExtra, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		// pingpang interation
		for (uint32_t level = 1; level <= ATrous_Level_Max; level++) {
			assert(ATrous_Level_Max > 1);
			uint32_t pingPangSrvUavOffset = ppSrvUavOffset;
			Texture color_in = (level == 1) ? col_acc : color_inout[level % 2];
			Texture color_out = (level == ATrous_Level_Max) ? col_acc : color_inout[(level + 1) % 2];
			Texture variance_in = variance_inout[level % 2]; // 这里预先设定过temporal pipeline的rt就是variance_inout[1]
			Texture variance_out = variance_inout[(level + 1) % 2];
			commandList->SetShaderResourceView(0, pingPangSrvUavOffset++, color_in, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			commandList->SetShaderResourceView(0, pingPangSrvUavOffset++, variance_in, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			commandList->SetUnorderedAccessView(0, pingPangSrvUavOffset++, color_out, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			commandList->SetUnorderedAccessView(0, pingPangSrvUavOffset++, variance_out, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			commandList->SetCompute32BitConstants(1, level);
			commandList->Dispatch(m_Width / local_width, m_Height / local_height);
		}
	}
}

void ShadowFilteringRenderer::PreRender(RenderResourceMap& resources)
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

	mRtShadowOutputTexture = *resources[RRD_RT_SHADOW_SAMPLE];

	auto swapCurrPrevTexture = [](Texture& t1, Texture& t2) {
		Texture tmp = t2;
		t2 = t1;
		t1 = tmp;
	};

	swapCurrPrevTexture(col_acc_prev, col_acc);
	swapCurrPrevTexture(moment_acc_prev, moment_acc);
	swapCurrPrevTexture(his_length_prev, his_length);

	
}

RenderResourceMap* ShadowFilteringRenderer::PostRender()
{
	(*m_Resources)[RRD_SHADOW_ACC] = std::make_shared<Texture>(col_acc);
	return Renderer::PostRender();
}
