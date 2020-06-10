#include "GBufferRenderer.h"

GBufferRenderer::GBufferRenderer(int w, int h) :
	Renderer(w, h),
	m_ScissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX)),
	m_Viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height)))
{

}

GBufferRenderer::~GBufferRenderer()
{
}

void GBufferRenderer::LoadResource(std::shared_ptr<Scene> scene, RenderResourceMap& resources)
{
	Renderer::LoadResource(scene, resources);

	gPosition = createTex2D_RenderTarget(L"gPosition", m_Width, m_Height);
	gAlbedoMetallic = *resources[RRD_ALBEDO_MATALLIC];
	gNormalRoughness = createTex2D_RenderTarget(L"gNormalRoughness", m_Width, m_Height);
	gExtra = createTex2D_RenderTarget(L"gExtra", m_Width, m_Height);

	gPosition_prev = createTex2D_RenderTarget(L"gPosition_prev", m_Width, m_Height);
	gAlbedoMetallic_prev = *resources[RRD_ALBEDO_MATALLIC_PREV];
	gNormalRoughness_prev = createTex2D_RenderTarget(L"gNormalRoughness_prev", m_Width, m_Height);
	gExtra_prev = createTex2D_RenderTarget(L"gExtra_prev", m_Width, m_Height);

	m_Resources->emplace(RRD_POSITION, std::make_shared<Texture>(gPosition));
	m_Resources->emplace(RRD_POSITION_PREV, std::make_shared<Texture>(gPosition_prev));
	m_Resources->emplace(RRD_NORMAL_ROUGHNESS, std::make_shared<Texture>(gNormalRoughness));
	m_Resources->emplace(RRD_NORMAL_ROUGHNESS_PREV, std::make_shared<Texture>(gNormalRoughness_prev));
	m_Resources->emplace(RRD_EXTRA, std::make_shared<Texture>(gExtra));
	m_Resources->emplace(RRD_EXTRA_PREV, std::make_shared<Texture>(gExtra_prev));


	m_GBuffer.AttachTexture(AttachmentPoint::Color0, gPosition);
	m_GBuffer.AttachTexture(AttachmentPoint::Color1, gAlbedoMetallic);
	m_GBuffer.AttachTexture(AttachmentPoint::Color2, gNormalRoughness);
	m_GBuffer.AttachTexture(AttachmentPoint::Color3, gExtra);
	m_GBuffer.AttachTexture(AttachmentPoint::DepthStencil, *resources[RRD_DEPTH_STENCIL]);
}

void GBufferRenderer::LoadPipeline()
{
	auto device = Application::Get().GetDevice();
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}
	DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };

	// Load the Deferred shaders.
	ComPtr<ID3DBlob> vs;
	ComPtr<ID3DBlob> ps;
	ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/HybridRenderingPipeline/Deferred_VS.cso", &vs));
	ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/HybridRenderingPipeline/Deferred_PS.cso", &ps));

	// Allow input layout and deny unnecessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	CD3DX12_DESCRIPTOR_RANGE1 descriptorRange[1] = {
		CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0) };

	CD3DX12_ROOT_PARAMETER1 rootParameters[5];
	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[1].InitAsConstantBufferView(0, 1, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[2].InitAsConstantBufferView(1, 1, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[3].InitAsConstants(1, 2, 1, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[4].InitAsDescriptorTable(arraysize(descriptorRange), descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

	//CD3DX12_STATIC_SAMPLER_DESC linearRepeatSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
	CD3DX12_STATIC_SAMPLER_DESC anisotropicSampler(0, D3D12_FILTER_ANISOTROPIC);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	rootSignatureDescription.Init_1_1(arraysize(rootParameters), rootParameters, 1, &anisotropicSampler, rootSignatureFlags);

	m_DeferredRootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

	// Setup the Deferred pipeline state.
	struct DeferredPipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
		CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC SampleDesc;
	} deferredPipelineStateStream;

	deferredPipelineStateStream.pRootSignature = m_DeferredRootSignature.GetRootSignature().Get();
	deferredPipelineStateStream.InputLayout = { VertexPositionNormalTexture::InputElements, VertexPositionNormalTexture::InputElementCount };
	deferredPipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	deferredPipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vs.Get());
	deferredPipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
	deferredPipelineStateStream.DSVFormat = m_GBuffer.GetDepthStencilFormat();
	deferredPipelineStateStream.RTVFormats = m_GBuffer.GetRenderTargetFormats();
	deferredPipelineStateStream.SampleDesc = sampleDesc;

	D3D12_PIPELINE_STATE_STREAM_DESC deferredPipelineStateStreamDesc = {
		sizeof(DeferredPipelineStateStream), &deferredPipelineStateStream
	};
	ThrowIfFailed(device->CreatePipelineState(&deferredPipelineStateStreamDesc, IID_PPV_ARGS(&m_DeferredPipelineState)));
}

void GBufferRenderer::Update(UpdateEventArgs& e, std::shared_ptr<Scene> scene)
{
}

void GBufferRenderer::Render(RenderEventArgs& e, std::shared_ptr<Scene> scene, std::shared_ptr<CommandList> commandList)
{
	FLOAT clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	commandList->ClearTexture(m_GBuffer.GetTexture(AttachmentPoint::Color0), clearColor);
	commandList->ClearTexture(m_GBuffer.GetTexture(AttachmentPoint::Color2), clearColor);
	commandList->ClearTexture(m_GBuffer.GetTexture(AttachmentPoint::Color3), clearColor);

	commandList->SetViewport(m_Viewport);
	commandList->SetScissorRect(m_ScissorRect);
	commandList->SetRenderTarget(m_GBuffer);
	commandList->SetPipelineState(m_DeferredPipelineState);
	commandList->SetGraphicsRootSignature(m_DeferredRootSignature);

	for (auto it = scene->gameObjectPool.begin(); it != scene->gameObjectPool.end(); it++) {
		{
			// comment the light for pos visualization
			std::string objIndex = it->first;
			if (objIndex.find("light") != std::string::npos) {
				continue;
			}
		}
		it->second->Draw(*commandList, scene->m_Camera);
	}
}

void GBufferRenderer::Resize(int w, int h)
{
	Renderer::Resize(w, h);

	m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f,
		static_cast<float>(m_Width), static_cast<float>(m_Height));
}

void GBufferRenderer::PreRender(RenderResourceMap& resources)
{
	Renderer::PreRender(resources);
	auto swapCurrPrevTexture = [](Texture& t1, Texture& t2) {
		Texture tmp = t2;
		t2 = t1;
		t1 = tmp;
	};
	gAlbedoMetallic = *resources[RRD_ALBEDO_MATALLIC];
	gAlbedoMetallic_prev = *resources[RRD_ALBEDO_MATALLIC_PREV];

	swapCurrPrevTexture(gPosition, gPosition_prev);
	swapCurrPrevTexture(gNormalRoughness, gNormalRoughness_prev);
	swapCurrPrevTexture(gExtra, gExtra_prev);

	m_GBuffer.AttachTexture(AttachmentPoint::Color0, gPosition);
	m_GBuffer.AttachTexture(AttachmentPoint::Color1, gAlbedoMetallic);
	m_GBuffer.AttachTexture(AttachmentPoint::Color2, gNormalRoughness);
	m_GBuffer.AttachTexture(AttachmentPoint::Color3, gExtra);
	m_GBuffer.AttachTexture(AttachmentPoint::DepthStencil, *resources[RRD_DEPTH_STENCIL]);
}

RenderResourceMap* GBufferRenderer::PostRender()
{
	(*m_Resources)[RRD_POSITION] = std::make_shared<Texture>(gPosition);
	(*m_Resources)[RRD_POSITION_PREV] = std::make_shared<Texture>(gPosition_prev);
	(*m_Resources)[RRD_NORMAL_ROUGHNESS] = std::make_shared<Texture>(gNormalRoughness);
	(*m_Resources)[RRD_NORMAL_ROUGHNESS_PREV] = std::make_shared<Texture>(gNormalRoughness_prev);
	(*m_Resources)[RRD_EXTRA] = std::make_shared<Texture>(gExtra);
	(*m_Resources)[RRD_EXTRA_PREV] = std::make_shared<Texture>(gExtra_prev);
	return Renderer::PostRender();
}
