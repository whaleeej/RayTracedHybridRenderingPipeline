#include "SkyboxRenderer.h"

SkyboxRenderer::SkyboxRenderer(int w, int h):
	Renderer(w, h),
	m_ScissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX)),
	m_Viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height)))
{

}

SkyboxRenderer::~SkyboxRenderer()
{
}

void SkyboxRenderer::LoadResource(std::shared_ptr<Scene> scene, RenderResourceMap& resources)
{
	Renderer::LoadResource(scene, resources);

	m_Albedo  = createTex2D_RenderTarget(L"gAlbedoMetallic", m_Width, m_Height);
	m_Albedo_prev = createTex2D_RenderTarget(L"gAlbedoMetallic_prev", m_Width, m_Height);
	m_DepthStencil = createTex2D_DepthStencil(L"Depth Render Target", m_Width, m_Height);
	m_SkyboxRT.AttachTexture(AttachmentPoint::Color0, m_Albedo);
	m_SkyboxRT.AttachTexture(AttachmentPoint::DepthStencil, m_DepthStencil);

	
	m_Resources->emplace(RRD_ALBEDO_MATALLIC, std::make_shared<Texture>(m_Albedo));
	m_Resources->emplace(RRD_ALBEDO_MATALLIC_PREV, std::make_shared<Texture>(m_Albedo_prev));
	m_Resources->emplace(RRD_DEPTH_STENCIL, std::make_shared<Texture>(m_DepthStencil));
}

void SkyboxRenderer::LoadPipeline()
{
	auto device = Application::Get().GetDevice();
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Load the Skybox shaders.
	ComPtr<ID3DBlob> vs;
	ComPtr<ID3DBlob> ps;
	ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/HybridRenderingPipeline/Skybox_VS.cso", &vs));
	ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/HybridRenderingPipeline/Skybox_PS.cso", &ps));

	// Setup the input layout for the skybox vertex shader.
	D3D12_INPUT_ELEMENT_DESC inputLayout[1] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	// Allow input layout and deny unnecessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER1 rootParameters[2];
	rootParameters[0].InitAsConstants(sizeof(DirectX::XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[1].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_STATIC_SAMPLER_DESC linearClampSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	rootSignatureDescription.Init_1_1(arraysize(rootParameters), rootParameters, 1, &linearClampSampler, rootSignatureFlags);

	m_SkyboxSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC des; // comparti for device1 interface
	ZeroMemory(&des, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	des.pRootSignature = m_SkyboxSignature.GetRootSignature().Get();
	des.VS = CD3DX12_SHADER_BYTECODE(vs.Get());
	des.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
	CD3DX12_BLEND_DESC blendDes(D3D12_DEFAULT);
	des.BlendState = blendDes;
	des.SampleMask = 0xffffffff;
	CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
	des.RasterizerState = rasterizerDesc;
	CD3DX12_DEPTH_STENCIL_DESC dsDesc(D3D12_DEFAULT);
	dsDesc.DepthEnable = false;
	des.DepthStencilState = dsDesc;
	des.InputLayout = { inputLayout, 1 };
	des.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	des.NumRenderTargets = m_SkyboxRT.GetRenderTargetFormats().NumRenderTargets;
	for (int fi = 0; fi < 8; fi++) {
		if (fi < m_SkyboxRT.GetRenderTargetFormats().NumRenderTargets)
			des.RTVFormats[fi] = m_SkyboxRT.GetRenderTargetFormats().RTFormats[fi];
		else
			des.RTVFormats[fi] = DXGI_FORMAT_UNKNOWN;
	}
	des.DSVFormat = m_SkyboxRT.GetDepthStencilFormat();
	DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };
	des.SampleDesc = sampleDesc;
	des.NodeMask = 0;
	des.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(device->CreateGraphicsPipelineState(&des, IID_PPV_ARGS(&m_SkyboxPipelineState)));
}

void SkyboxRenderer::Update(UpdateEventArgs& e, std::shared_ptr<Scene> scene)
{
	auto viewMatrix = XMMatrixTranspose(XMMatrixRotationQuaternion(scene->m_Camera.get_Rotation()));
	auto projMatrix = scene->m_Camera.get_ProjectionMatrix();
	viewProjMatrix = viewMatrix * projMatrix;
}

void SkyboxRenderer::Render(RenderEventArgs& e, std::shared_ptr<Scene> scene, std::shared_ptr<CommandList> commandList)
{
	FLOAT clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	commandList->ClearTexture(m_Albedo, clearColor);
	commandList->ClearDepthStencilTexture(m_DepthStencil, D3D12_CLEAR_FLAG_DEPTH);

	commandList->SetViewport(m_Viewport);
	commandList->SetScissorRect(m_ScissorRect);
	commandList->SetRenderTarget(m_SkyboxRT);
	commandList->SetPipelineState(m_SkyboxPipelineState);
	commandList->SetGraphicsRootSignature(m_SkyboxSignature);

	// The view matrix should only consider the camera's rotation, but not the translation.
	commandList->SetGraphics32BitConstants(0, viewProjMatrix);

	// skybox rendering
	auto& skybox_texture = TexturePool::Get().getTexture(scene->cubemap_Index);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = skybox_texture->GetD3D12ResourceDesc().Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MipLevels = (UINT)-1; 
	// TODO: Need a better way to bind a cubemap.
	commandList->SetShaderResourceView(1, 0, *skybox_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 0, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, &srvDesc);

	scene->m_SkyboxMesh->Draw(*commandList);
}

void SkyboxRenderer::Resize(int w, int h)
{
	Renderer::Resize(w, h);

	m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f,
		static_cast<float>(m_Width), static_cast<float>(m_Height));
}

void SkyboxRenderer::PreRender(RenderResourceMap& resources)
{
	Renderer::PreRender(resources);
	auto swapCurrPrevTexture = [](Texture& t1, Texture& t2) {
		Texture tmp = t2;
		t2 = t1;
		t1 = tmp;
	};
	swapCurrPrevTexture(m_Albedo, m_Albedo_prev);
	m_SkyboxRT.AttachTexture(AttachmentPoint::Color0, m_Albedo);

}

RenderResourceMap* SkyboxRenderer::PostRender()
{
	(*m_Resources)[RRD_ALBEDO_MATALLIC] = std::make_shared<Texture>(m_Albedo);
	(*m_Resources)[RRD_ALBEDO_MATALLIC_PREV] = std::make_shared<Texture>(m_Albedo_prev);
	return Renderer::PostRender();
}
