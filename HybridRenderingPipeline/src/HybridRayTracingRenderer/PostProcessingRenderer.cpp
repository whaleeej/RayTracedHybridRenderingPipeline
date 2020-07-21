#include "PostProcessingRenderer.h"

PostProcessingRenderer::PostProcessingRenderer(int w, int h) :
	Renderer(w, h),
	m_ScissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX)),
	m_Viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height)))
{

}

PostProcessingRenderer::~PostProcessingRenderer()
{
}


void PostProcessingRenderer::LoadResource(std::shared_ptr<Scene> scene, RenderResourceMap& resources)
{
	Renderer::LoadResource(scene, resources);
	gPosition = *resources[RRD_POSITION];
	gAlbedoMetallic = *resources[RRD_ALBEDO_MATALLIC];
	gNormalRoughness = *resources[RRD_NORMAL_ROUGHNESS];
	gExtra = *resources[RRD_EXTRA];
	if (Application::Get().isDXRSupport()) {
		mRtShadowOutputTexture = *resources[RRD_RT_SHADOW_SAMPLE];
		mRtReflectOutputTexture = *resources[RRD_RT_INDIRECT_SAMPLE];
		col_acc = *resources[RRD_SHADOW_ACC];
		filtered_curr = *resources[RRD_INDIRECT_FILTERED];
	}
	else {
		mRtShadowOutputTexture = createTex2D_ReadOnly(L"mRtShadowOutputTexture", m_Width, m_Height);
		mRtReflectOutputTexture = createTex2D_ReadOnly(L"mRtReflectOutputTexture", m_Width, m_Height);
		col_acc = createTex2D_ReadOnly(L"col_acc", m_Width, m_Height);
		filtered_curr = createTex2D_ReadOnly(L"filtered_curr", m_Width, m_Height);
	}

	std::shared_ptr<Texture> presentTexture = resources[RRD_RENDERTARGET_PRESENT];
	presentRenderTarget.AttachTexture(AttachmentPoint::Color0, *presentTexture);
}

void PostProcessingRenderer::LoadPipeline()
{
	auto device = Application::Get().GetDevice();
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}
	DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };

	CD3DX12_DESCRIPTOR_RANGE1 descriptorRange[1] = {
				CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 8, 0)
	};

	CD3DX12_ROOT_PARAMETER1 rootParameters[4];
	rootParameters[0].InitAsDescriptorTable(arraysize(descriptorRange), descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[1].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[2].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[3].InitAsConstantBufferView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);


	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	rootSignatureDescription.Init_1_1(arraysize(rootParameters), rootParameters);

	m_PostLightingRootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

	ComPtr<ID3DBlob> vs;
	ComPtr<ID3DBlob> ps;
	ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/HybridRenderingPipeline/PostProcessing_VS.cso", &vs));
	ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/HybridRenderingPipeline/PostLighting_PS.cso", &ps));

	CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

	struct PostLightingStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterizer;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
	} postLightingPipelineStateStream;

	postLightingPipelineStateStream.pRootSignature = m_PostLightingRootSignature.GetRootSignature().Get();
	postLightingPipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	postLightingPipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vs.Get());
	postLightingPipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
	postLightingPipelineStateStream.Rasterizer = rasterizerDesc;
	postLightingPipelineStateStream.RTVFormats = presentRenderTarget.GetRenderTargetFormats();

	D3D12_PIPELINE_STATE_STREAM_DESC postLightingPipelineStateStreamDesc = {
		sizeof(postLightingPipelineStateStream), &postLightingPipelineStateStream
	};
	ThrowIfFailed(device->CreatePipelineState(&postLightingPipelineStateStreamDesc, IID_PPV_ARGS(&m_PostLightingPipelineState)));
}

void PostProcessingRenderer::Update(UpdateEventArgs& e, std::shared_ptr<Scene> scene)
{
	mCameraCB.PositionWS = scene->m_Camera.get_Translation();
	mCameraCB.InverseViewMatrix = scene->m_Camera.get_InverseViewMatrix();
	mCameraCB.fov = scene->m_Camera.get_FoV();
}

void PostProcessingRenderer::Render(RenderEventArgs& e, std::shared_ptr<Scene> scene, std::shared_ptr<CommandList> commandList)
{
	commandList->SetViewport(m_Viewport);
	commandList->SetScissorRect(m_ScissorRect);
	commandList->SetRenderTarget(presentRenderTarget);
	commandList->SetPipelineState(m_PostLightingPipelineState);
	commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->SetGraphicsRootSignature(m_PostLightingRootSignature);
	uint32_t ppSrvUavOffset = 0;
	commandList->SetShaderResourceView(0, ppSrvUavOffset++, gPosition, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList->SetShaderResourceView(0, ppSrvUavOffset++, gAlbedoMetallic, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList->SetShaderResourceView(0, ppSrvUavOffset++, gNormalRoughness, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList->SetShaderResourceView(0, ppSrvUavOffset++, gExtra, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList->SetShaderResourceView(0, ppSrvUavOffset++, mRtShadowOutputTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList->SetShaderResourceView(0, ppSrvUavOffset++, mRtReflectOutputTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList->SetShaderResourceView(0, ppSrvUavOffset++, col_acc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList->SetShaderResourceView(0, ppSrvUavOffset++, filtered_curr, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList->SetGraphicsDynamicConstantBuffer(1, scene->m_PointLight);
	commandList->SetGraphicsDynamicConstantBuffer(2, mCameraCB);
	commandList->SetGraphicsDynamicConstantBuffer(3, postTestingCB);
	commandList->Draw(3);
}

void PostProcessingRenderer::Resize(int w, int h)
{
	Renderer::Resize(w, h);

	m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f,
		static_cast<float>(m_Width), static_cast<float>(m_Height));
}

void PostProcessingRenderer::PressKey(KeyEventArgs& e)
{
	switch (e.Key)
	{
	case KeyCode::D4:
		postTestingCB.inc();
		break;
	}
}

void PostProcessingRenderer::PreRender(RenderResourceMap& resources)
{
	Renderer::PreRender(resources);

	gPosition = *resources[RRD_POSITION];
	gAlbedoMetallic = *resources[RRD_ALBEDO_MATALLIC];
	gNormalRoughness = *resources[RRD_NORMAL_ROUGHNESS];
	gExtra = *resources[RRD_EXTRA];
	if (Application::Get().isDXRSupport()) {
		mRtShadowOutputTexture = *resources[RRD_RT_SHADOW_SAMPLE];
		mRtReflectOutputTexture = *resources[RRD_RT_INDIRECT_SAMPLE];

		col_acc = *resources[RRD_SHADOW_ACC];
		filtered_curr = *resources[RRD_INDIRECT_FILTERED];
	}
	

	std::shared_ptr<Texture> presentTexture = resources[RRD_RENDERTARGET_PRESENT];
	presentRenderTarget.AttachTexture(AttachmentPoint::Color0, *presentTexture);
}

RenderResourceMap* PostProcessingRenderer::PostRender()
{
	return Renderer::PostRender();
}

