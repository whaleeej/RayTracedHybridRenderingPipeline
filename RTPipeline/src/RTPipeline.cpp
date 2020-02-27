#include <RTPipeline.h>

#include <Application.h>
#include <CommandQueue.h>
#include <CommandList.h>
#include <Helpers.h>
#include <Light.h>

#include <Window.h>

#include <wrl.h>
using namespace Microsoft::WRL;

#include <d3dx12.h>
#include <d3dcompiler.h>
#include <DirectXColors.h>
#include <ResourceStateTracker.h>
#include <ASBuffer.h>
#include <Helpers.h>
#include <DirectXMath.h>

using namespace DirectX;

#include <algorithm> // For std::min and std::max.
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif


// An enum for root signature parameters.
// I'm not using scoped enums to avoid the explicit cast that would be required
// to use these as root indices in the root signature.
enum RootParameters
{
    MatricesCB,         // ConstantBuffer<Mat> MatCB : register(b0);
    MaterialCB,         // ConstantBuffer<Material> MaterialCB : register( b0, space1 );
	PBRMaterialCB,  // ConstantBuffer<PBRMaterial> PBRMaterialCB: register(b1, space1);
    Textures,            // Texture2D DiffuseTextures : register( t0-tN );
    NumRootParameters
};

// Builds a look-at (world) matrix from a point, up and direction vectors.

static bool g_AllowFullscreenToggle = true;

XMMATRIX XM_CALLCONV LookAtMatrix(FXMVECTOR Position, FXMVECTOR Direction, FXMVECTOR Up)
{
    assert(!XMVector3Equal(Direction, XMVectorZero()));
    assert(!XMVector3IsInfinite(Direction));
    assert(!XMVector3Equal(Up, XMVectorZero()));
    assert(!XMVector3IsInfinite(Up));

    XMVECTOR R2 = XMVector3Normalize(Direction);

    XMVECTOR R0 = XMVector3Cross(Up, R2);
    R0 = XMVector3Normalize(R0);

    XMVECTOR R1 = XMVector3Cross(R2, R0);

    XMMATRIX M(R0, R1, R2, Position);

    return M;
}



HybridPipeline::HybridPipeline(const std::wstring& name, int width, int height, bool vSync)
    : super(name, width, height, vSync)
    , m_ScissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX))
    , m_Viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)))
    , m_Forward(0)
    , m_Backward(0)
    , m_Left(0)
    , m_Right(0)
    , m_Up(0)
    , m_Down(0)
    , m_Pitch(0)
    , m_Yaw(0)
    , m_AnimateLights(false)
    , m_Shift(false)
    , m_Width(0)
    , m_Height(0)
{

    XMVECTOR cameraPos = XMVectorSet(0, 10, -35 , 1);
    XMVECTOR cameraTarget = XMVectorSet(0, 5, 0, 1);
    XMVECTOR cameraUp = XMVectorSet(0, 1, 0, 0);

    m_Camera.set_LookAt(cameraPos, cameraTarget, cameraUp);

    m_pAlignedCameraData = (CameraData*)_aligned_malloc(sizeof(CameraData), 16);
    m_pAlignedCameraData->m_InitialCamPos = m_Camera.get_Translation();
    m_pAlignedCameraData->m_InitialCamRot = m_Camera.get_Rotation();
}

HybridPipeline::~HybridPipeline()
{
    _aligned_free(m_pAlignedCameraData);
}

bool HybridPipeline::LoadContent()
{
	auto device = Application::Get().GetDevice();
	{ // Resource Loading
		auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
		auto commandList = commandQueue->GetCommandList();

		// Create a Cube mesh
		meshPool.emplace("cube", Mesh::CreateCube(*commandList));
		meshPool.emplace("sphere", Mesh::CreateSphere(*commandList));
		meshPool.emplace("cone", Mesh::CreateCone(*commandList));
		meshPool.emplace("torus", Mesh::CreateTorus(*commandList));
		meshPool.emplace("plane", Mesh::CreatePlane(*commandList));

		// load PBR textures
		texturePool.emplace("default_albedo", Texture());
		commandList->LoadTextureFromFile(texturePool["default_albedo"], L"Assets/Textures/pbr/default/albedo.bmp", TextureUsage::Albedo);
		texturePool.emplace("default_metallic", Texture());
		commandList->LoadTextureFromFile(texturePool["default_metallic"], L"Assets/Textures/pbr/default/metallic.bmp", TextureUsage::MetallicMap);
		texturePool.emplace("default_normal", Texture());
		commandList->LoadTextureFromFile(texturePool["default_normal"], L"Assets/Textures/pbr/default/normal.bmp", TextureUsage::Normalmap);
		texturePool.emplace("default_roughness", Texture());
		commandList->LoadTextureFromFile(texturePool["default_roughness"], L"Assets/Textures/pbr/default/roughness.bmp", TextureUsage::RoughnessMap);
		//// rusted iron
		texturePool.emplace("rusted_iron_albedo", Texture());
		commandList->LoadTextureFromFile(texturePool["rusted_iron_albedo"], L"Assets/Textures/pbr/rusted_iron/albedo.png", TextureUsage::Albedo);
		texturePool.emplace("rusted_iron_metallic", Texture());
		commandList->LoadTextureFromFile(texturePool["rusted_iron_metallic"], L"Assets/Textures/pbr/rusted_iron/metallic.png", TextureUsage::MetallicMap);
		texturePool.emplace("rusted_iron_normal", Texture());
		commandList->LoadTextureFromFile(texturePool["rusted_iron_normal"], L"Assets/Textures/pbr/rusted_iron/normal.png", TextureUsage::Normalmap);
		texturePool.emplace("rusted_iron_roughness", Texture());
		commandList->LoadTextureFromFile(texturePool["rusted_iron_roughness"], L"Assets/Textures/pbr/rusted_iron/roughness.png", TextureUsage::RoughnessMap);
		//// grid metal
		texturePool.emplace("grid_metal_albedo", Texture());
		commandList->LoadTextureFromFile(texturePool["grid_metal_albedo"], L"Assets/Textures/pbr/grid_metal/albedo.png", TextureUsage::Albedo);
		texturePool.emplace("grid_metal_metallic", Texture());
		commandList->LoadTextureFromFile(texturePool["grid_metal_metallic"], L"Assets/Textures/pbr/grid_metal/metallic.png", TextureUsage::MetallicMap);
		texturePool.emplace("grid_metal_normal", Texture());
		commandList->LoadTextureFromFile(texturePool["grid_metal_normal"], L"Assets/Textures/pbr/grid_metal/normal.png", TextureUsage::Normalmap);
		texturePool.emplace("grid_metal_roughness", Texture());
		commandList->LoadTextureFromFile(texturePool["grid_metal_roughness"], L"Assets/Textures/pbr/grid_metal/roughness.png", TextureUsage::RoughnessMap);
		//// metal
		texturePool.emplace("metal_albedo", Texture());
		commandList->LoadTextureFromFile(texturePool["metal_albedo"], L"Assets/Textures/pbr/metal/albedo.png", TextureUsage::Albedo);
		texturePool.emplace("metal_metallic", Texture());
		commandList->LoadTextureFromFile(texturePool["metal_metallic"], L"Assets/Textures/pbr/metal/metallic.png", TextureUsage::MetallicMap);
		texturePool.emplace("metal_normal", Texture());
		commandList->LoadTextureFromFile(texturePool["metal_normal"], L"Assets/Textures/pbr/metal/normal.png", TextureUsage::Normalmap);
		texturePool.emplace("metal_roughness", Texture());
		commandList->LoadTextureFromFile(texturePool["metal_roughness"], L"Assets/Textures/pbr/metal/roughness.png", TextureUsage::RoughnessMap);

		auto fenceValue = commandQueue->ExecuteCommandList(commandList);
		commandQueue->WaitForFenceValue(fenceValue);
	}

	{ ///////////////////////// GameObject Gen
		gameObjectPool.emplace("sphere",std::make_shared<GameObject>());
		gameObjectPool["sphere"]->mesh = "sphere";
		gameObjectPool["sphere"]->material.base = Material::White;
		gameObjectPool["sphere"]->material.pbr = PBRMaterial(1.0f, 1.0f);
		gameObjectPool["sphere"]->material.tex = TextureMaterial("rusted_iron");

		gameObjectPool.emplace("cube", std::make_shared<GameObject>());
		gameObjectPool["cube"]->mesh = "cube";
		gameObjectPool["cube"]->material.base = Material::White;
		gameObjectPool["cube"]->material.pbr = PBRMaterial(1.0f, 1.0f);
		gameObjectPool["cube"]->material.tex = TextureMaterial("grid_metal");

		gameObjectPool.emplace("torus", std::make_shared<GameObject>());
		gameObjectPool["torus"]->mesh = "torus";
		gameObjectPool["torus"]->material.base = Material::White;
		gameObjectPool["torus"]->material.pbr = PBRMaterial(1.0f, 1.0f);
		gameObjectPool["torus"]->material.tex = TextureMaterial("metal");

		gameObjectPool.emplace("Floor plane", std::make_shared<GameObject>()); //1
		gameObjectPool["Floor plane"]->mesh = "plane";
		gameObjectPool["Floor plane"]->material.base = Material::Red;
		gameObjectPool["Floor plane"]->material.pbr = PBRMaterial(1.0f, 1.0f);
		gameObjectPool["Floor plane"]->material.tex = TextureMaterial("default");

		gameObjectPool.emplace("Back wall", std::make_shared<GameObject>());//2
		gameObjectPool["Back wall"]->mesh = "plane";
		gameObjectPool["Back wall"]->material.base = Material::Green;
		gameObjectPool["Back wall"]->material.pbr = PBRMaterial(1.0f, 1.0f);
		gameObjectPool["Back wall"]->material.tex = TextureMaterial("default");

		gameObjectPool.emplace("Ceiling plane", std::make_shared<GameObject>());//3
		gameObjectPool["Ceiling plane"]->mesh = "plane";
		gameObjectPool["Ceiling plane"]->material.base = Material::Blue;
		gameObjectPool["Ceiling plane"]->material.pbr = PBRMaterial(1.0f, 1.0f);
		gameObjectPool["Ceiling plane"]->material.tex = TextureMaterial("default");

		gameObjectPool.emplace("Front wall", std::make_shared<GameObject>());//4
		gameObjectPool["Front wall"]->mesh = "plane";
		gameObjectPool["Front wall"]->material.base = Material::Yellow;
		gameObjectPool["Front wall"]->material.pbr = PBRMaterial(1.0f, 1.0f);
		gameObjectPool["Front wall"]->material.tex = TextureMaterial("default");

		gameObjectPool.emplace("Left wall", std::make_shared<GameObject>());//5
		gameObjectPool["Left wall"]->mesh = "plane";
		gameObjectPool["Left wall"]->material.base = Material::Cyan;
		gameObjectPool["Left wall"]->material.pbr = PBRMaterial(1.0f, 1.0f);
		gameObjectPool["Left wall"]->material.tex = TextureMaterial("default");

		gameObjectPool.emplace("Right wall", std::make_shared<GameObject>());//6
		gameObjectPool["Right wall"]->mesh = "plane";
		gameObjectPool["Right wall"]->material.base = Material::Magenta;
		gameObjectPool["Right wall"]->material.pbr = PBRMaterial(1.0f, 1.0f);
		gameObjectPool["Right wall"]->material.tex = TextureMaterial("default");
	}
    
	{ // Initial the transform
		gameObjectPool["sphere"]->Translate(XMMatrixTranslation(4.0f, 3.0f, 2.0f));
		gameObjectPool["sphere"]->Rotate(XMMatrixIdentity());
		gameObjectPool["sphere"]->Scale(XMMatrixScaling(6.0f, 6.0f, 6.0f));

		gameObjectPool["cube"]->Translate(XMMatrixTranslation(-4.0f, 3.0f, -2.0f));
		gameObjectPool["cube"]->Rotate(XMMatrixRotationY(XMConvertToRadians(45.0f)));
		gameObjectPool["cube"]->Scale(XMMatrixScaling(6.0f, 6.0f, 6.0f));

		gameObjectPool["torus"]->Translate(XMMatrixTranslation(4.0f, 0.6f, -6.0f));
		gameObjectPool["torus"]->Rotate(XMMatrixRotationY(XMConvertToRadians(45.0f)));
		gameObjectPool["torus"]->Scale(XMMatrixScaling(4.0f, 4.0f, 4.0f));

		float scalePlane = 20.0f;
		float translateOffset = scalePlane / 2.0f;

		gameObjectPool["Floor plane"]->Translate(XMMatrixTranslation(0.0f, 0.0f, 0.0f));
		gameObjectPool["Floor plane"]->Rotate(XMMatrixIdentity());
		gameObjectPool["Floor plane"]->Scale(XMMatrixScaling(scalePlane, 1.0f, scalePlane));

		gameObjectPool["Back wall"]->Translate(XMMatrixTranslation(0.0f, translateOffset, translateOffset));
		gameObjectPool["Back wall"]->Rotate(XMMatrixRotationX(XMConvertToRadians(-90)));
		gameObjectPool["Back wall"]->Scale(XMMatrixScaling(scalePlane, 1.0f, scalePlane));

		gameObjectPool["Ceiling plane"]->Translate(XMMatrixTranslation(0.0f, translateOffset * 2.0f, 0));
		gameObjectPool["Ceiling plane"]->Rotate(XMMatrixRotationX(XMConvertToRadians(180)));
		gameObjectPool["Ceiling plane"]->Scale(XMMatrixScaling(scalePlane, 1.0f, scalePlane));

		gameObjectPool["Front wall"]->Translate(XMMatrixTranslation(0, translateOffset, -translateOffset));
		gameObjectPool["Front wall"]->Rotate(XMMatrixRotationX(XMConvertToRadians(90)));
		gameObjectPool["Front wall"]->Scale(XMMatrixScaling(scalePlane, 1.0f, scalePlane));

		gameObjectPool["Left wall"]->Translate(XMMatrixTranslation(-translateOffset, translateOffset, 0));
		gameObjectPool["Left wall"]->Rotate(XMMatrixRotationX(XMConvertToRadians(-90)) * XMMatrixRotationY(XMConvertToRadians(-90)));
		gameObjectPool["Left wall"]->Scale(XMMatrixScaling(scalePlane, 1.0f, scalePlane));

		gameObjectPool["Right wall"]->Translate(XMMatrixTranslation(translateOffset, translateOffset, 0));
		gameObjectPool["Right wall"]->Rotate(XMMatrixRotationX(XMConvertToRadians(-90)) * XMMatrixRotationY(XMConvertToRadians(90)));
		gameObjectPool["Right wall"]->Scale(XMMatrixScaling(scalePlane, 1.0f, scalePlane));
	}

	{ ///////////////////////////// BackScreen RT Creation
		DXGI_FORMAT HDRFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
		DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

		// Disable the multiple sampling for performance and simplicity
		DXGI_SAMPLE_DESC sampleDesc = { 1, 0 }/*Application::Get().GetMultisampleQualityLevels(HDRFormat, D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT)*/;

		// Create an off-screen multi render target(MRT) and a depth buffer.
		// AttachmentPoint::Color0
		auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(HDRFormat,
			m_Width, m_Height,
			1, 1,
			sampleDesc.Count, sampleDesc.Quality,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
		D3D12_CLEAR_VALUE colorClearValue;
		colorClearValue.Format = colorDesc.Format;
		colorClearValue.Color[0] = 0.0f;
		colorClearValue.Color[1] = 0.0f;
		colorClearValue.Color[2] = 0.0f;
		colorClearValue.Color[3] = 1.0f;

		Texture gPosition = Texture(colorDesc, &colorClearValue,
			TextureUsage::RenderTarget,
			L"gPosition Texture");
		Texture gAlbedo = Texture(colorDesc, &colorClearValue,
			TextureUsage::RenderTarget,
			L"gAlbedo Texture");
		Texture gMetallic = Texture(colorDesc, &colorClearValue,
			TextureUsage::RenderTarget,
			L"gMetallic Texture");
		Texture gNormal = Texture(colorDesc, &colorClearValue,
			TextureUsage::RenderTarget,
			L"gNormal Texture");
		Texture gRoughness = Texture(colorDesc, &colorClearValue,
			TextureUsage::RenderTarget,
			L"gRoughness Texture");

		// Create a depth buffer for the Deferred render target.
		auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthBufferFormat,
			m_Width, m_Height,
			1, 1,
			sampleDesc.Count, sampleDesc.Quality,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
		D3D12_CLEAR_VALUE depthClearValue;
		depthClearValue.Format = depthDesc.Format;
		depthClearValue.DepthStencil = { 1.0f, 0 };

		Texture depthTexture = Texture(depthDesc, &depthClearValue,
			TextureUsage::Depth,
			L"Depth Render Target");

		// Attach the HDR texture to the HDR render target.
		m_DeferredRenderTarget.AttachTexture(AttachmentPoint::Color0, gPosition);
		m_DeferredRenderTarget.AttachTexture(AttachmentPoint::Color1, gAlbedo);
		m_DeferredRenderTarget.AttachTexture(AttachmentPoint::Color2, gMetallic);
		m_DeferredRenderTarget.AttachTexture(AttachmentPoint::Color3, gNormal);
		m_DeferredRenderTarget.AttachTexture(AttachmentPoint::Color4, gRoughness);
		m_DeferredRenderTarget.AttachTexture(AttachmentPoint::DepthStencil, depthTexture);
	}

	{ // RT Gen
		createAccelerationStructures();
		createRtPipelineState();
		createShaderResourcesAndSrvUavheap();
		createShaderTable();
	}

	{ /////////////////////////////////// Root Signature
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}
		// Disable the multiple sampling for performance and simplicity
		DXGI_SAMPLE_DESC sampleDesc = { 1, 0 }/*Application::Get().GetMultisampleQualityLevels(HDRFormat, D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT)*/;

		// Create a root signature for the Deferred Shadings pipeline.
		{
			// Load the Deferred shaders.
			ComPtr<ID3DBlob> vs;
			ComPtr<ID3DBlob> ps;
			ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/RTPipeline/Deferred_VS.cso", &vs));
			ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/RTPipeline/Deferred_PS.cso", &ps));

			// Allow input layout and deny unnecessary access to certain pipeline stages.
			D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

			CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);

			CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameters::NumRootParameters];
			rootParameters[RootParameters::MatricesCB].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
			rootParameters[RootParameters::MaterialCB].InitAsConstantBufferView(0, 1, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
			rootParameters[RootParameters::PBRMaterialCB].InitAsConstantBufferView(1, 1, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
			rootParameters[RootParameters::Textures].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

			//CD3DX12_STATIC_SAMPLER_DESC linearRepeatSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
			CD3DX12_STATIC_SAMPLER_DESC anisotropicSampler(0, D3D12_FILTER_ANISOTROPIC);

			CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
			rootSignatureDescription.Init_1_1(RootParameters::NumRootParameters, rootParameters, 1, &anisotropicSampler, rootSignatureFlags);

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
			deferredPipelineStateStream.DSVFormat = m_DeferredRenderTarget.GetDepthStencilFormat();
			deferredPipelineStateStream.RTVFormats = m_DeferredRenderTarget.GetRenderTargetFormats();
			deferredPipelineStateStream.SampleDesc = sampleDesc;

			D3D12_PIPELINE_STATE_STREAM_DESC deferredPipelineStateStreamDesc = {
				sizeof(DeferredPipelineStateStream), &deferredPipelineStateStream
			};
			ThrowIfFailed(device->CreatePipelineState(&deferredPipelineStateStreamDesc, IID_PPV_ARGS(&m_DeferredPipelineState)));
		}

		// Create the PostProcessing Root Signature
		{
			CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

			CD3DX12_ROOT_PARAMETER1 rootParameters[1];
			rootParameters[0].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

			CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
			rootSignatureDescription.Init_1_1(1, rootParameters);

			m_PostProcessingRootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

			// Create the PostProcessing PSO
			ComPtr<ID3DBlob> vs;
			ComPtr<ID3DBlob> ps;
			ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/RTPipeline/PostProcessing_VS.cso", &vs));
			ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/RTPipeline/PostProcessing_PS.cso", &ps));

			CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
			rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

			struct PostProcessingPipelineStateStream
			{
				CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
				CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
				CD3DX12_PIPELINE_STATE_STREAM_VS VS;
				CD3DX12_PIPELINE_STATE_STREAM_PS PS;
				CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterizer;
				CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
			} postProcessingPipelineStateStream;

			postProcessingPipelineStateStream.pRootSignature = m_PostProcessingRootSignature.GetRootSignature().Get();
			postProcessingPipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			postProcessingPipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vs.Get());
			postProcessingPipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
			postProcessingPipelineStateStream.Rasterizer = rasterizerDesc;
			postProcessingPipelineStateStream.RTVFormats = m_pWindow->GetRenderTarget().GetRenderTargetFormats();

			D3D12_PIPELINE_STATE_STREAM_DESC postProcessingPipelineStateStreamDesc = {
				sizeof(PostProcessingPipelineStateStream), & postProcessingPipelineStateStream
			};
			ThrowIfFailed(device->CreatePipelineState(&postProcessingPipelineStateStreamDesc, IID_PPV_ARGS(&m_PostProcessingPipelineState)));
		}
	}

	return true;
}

void HybridPipeline::OnResize(ResizeEventArgs& e)
{
    super::OnResize(e);

    if (m_Width != e.Width || m_Height != e.Height)
    {
        m_Width = std::max(1, e.Width);
        m_Height = std::max(1, e.Height);

        float aspectRatio = m_Width / (float)m_Height;
        m_Camera.set_Projection(45.0f, aspectRatio, 0.1f, 100.0f);

        m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f,
            static_cast<float>(m_Width), static_cast<float>(m_Height));

		m_DeferredRenderTarget.Resize(m_Width, m_Height);
    }
}

void HybridPipeline::UnloadContent()
{
}

void HybridPipeline::OnUpdate(UpdateEventArgs& e)
{
    static uint64_t frameCount = 0;
	static uint64_t totalFrameCount = 0;
    static double totalTime = 0.0;

    super::OnUpdate(e);

	{// frametime
		totalTime += e.ElapsedTime;
		frameCount++;
		totalFrameCount++;

		if (totalTime > 1.0)
		{
			double fps = frameCount / totalTime;

			char buffer[512];
			sprintf_s(buffer, "FPS: %f,  FrameCount: %lld\n", fps, totalFrameCount);
			OutputDebugStringA(buffer);

			frameCount = 0;
			totalTime = 0.0;
		}
	}

	// update the structure buffer of Pointlight
	{
		FrameIndexCB fid;
		fid.FrameIndex = static_cast<uint32_t>(totalFrameCount);
		void* pData;
		mpFrameIndexCB->Map(0, nullptr, (void**)& pData);
		memcpy(pData, &fid, sizeof(FrameIndexCB));
		mpFrameIndexCB->Unmap(0, nullptr);
	}

	{// camera

		 // Update the camera.
		float speedMultipler = (m_Shift ? 16.0f : 4.0f);

		XMVECTOR cameraTranslate = XMVectorSet(m_Right - m_Left, 0.0f, m_Forward - m_Backward, 1.0f) * speedMultipler * static_cast<float>(e.ElapsedTime);
		XMVECTOR cameraPan = XMVectorSet(0.0f, m_Up - m_Down, 0.0f, 1.0f) * speedMultipler * static_cast<float>(e.ElapsedTime);
		m_Camera.Translate(cameraTranslate, Space::Local);
		m_Camera.Translate(cameraPan, Space::Local);

		XMVECTOR cameraRotation = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(m_Pitch), XMConvertToRadians(m_Yaw), 0.0f);
		m_Camera.set_Rotation(cameraRotation);

		XMMATRIX viewMatrix = m_Camera.get_ViewMatrix();

		// update the camera for rt
		{
			CameraRTCB mCameraCB;
			mCameraCB.PositionWS = m_Camera.get_Translation();
			mCameraCB.InverseViewMatrix = m_Camera.get_InverseViewMatrix();
			mCameraCB.fov = m_Camera.get_FoV();
			void* pData;
			mpRTCameraCB->Map(0, nullptr, (void**)& pData);
			memcpy(pData, &mCameraCB, sizeof(CameraRTCB));
			mpRTCameraCB->Unmap(0, nullptr);
		}
	}
   
	{ // light
		XMMATRIX viewMatrix = m_Camera.get_ViewMatrix();

		static const XMVECTORF32 LightColors[] =
		{
			Colors::White, Colors::Yellow, Colors::Red, Colors::Green, Colors::Blue, Colors::Indigo, Colors::Violet, Colors::Orange
		};

		static float lightAnimTime = 0.0f;
		if (m_AnimateLights)
		{
			lightAnimTime += static_cast<float>(e.ElapsedTime) * 0.5f * XM_PI;
		}

		const float radius = 8.0f;
		const float offset = 2.0f * XM_PI / numPointLights;
		const float offset2 = offset + (offset / 2.0f);

		// Setup the light buffers.
		m_PointLights.resize(numPointLights);
		for (int i = 0; i < numPointLights; ++i)
		{
			PointLight& l = m_PointLights[i];

			l.PositionWS = {
				static_cast<float>(std::sin(lightAnimTime + offset * i)) * radius,
				9.0f,
				static_cast<float>(std::cos(lightAnimTime + offset * i)) * radius,
				1.0f
			};
			XMVECTOR positionWS = XMLoadFloat4(&l.PositionWS);
			l.Color = XMFLOAT4(LightColors[i]);
			l.Intensity = 1.0f;
			l.Attenuation = 0.01f;
		}

		m_SpotLights.resize(numSpotLights);
		for (int i = 0; i < numSpotLights; ++i)
		{
			SpotLight& l = m_SpotLights[i];

			l.PositionWS = {
				static_cast<float>(std::sin(lightAnimTime + offset * i + offset2)) * radius,
				9.0f,
				static_cast<float>(std::cos(lightAnimTime + offset * i + offset2)) * radius,
				1.0f
			};
			XMVECTOR positionWS = XMLoadFloat4(&l.PositionWS);

			XMVECTOR directionWS = XMVector3Normalize(XMVectorSetW(XMVectorNegate(positionWS), 0));
			XMStoreFloat4(&l.DirectionWS, directionWS);

			l.Color = XMFLOAT4(LightColors[numPointLights + i]);
			l.Intensity = 1.0f;
			l.SpotAngle = XMConvertToRadians(45.0f);
			l.Attenuation = 0.01f;
		}

		{ // update the light for rt
			// update the mpRTLightPropertiesCB
			{
				LightPropertiesCB mLightPropertiesCB;
				mLightPropertiesCB.NumPointLights = numPointLights;
				mLightPropertiesCB.NumSpotLights = numSpotLights;
				void* pData;
				mpRTLightPropertiesCB->Map(0, nullptr, (void**)& pData);
				memcpy(pData, &mLightPropertiesCB, sizeof(LightPropertiesCB));
				mpRTLightPropertiesCB->Unmap(0, nullptr);
			}

			// update the structure buffer of Pointlight
			{
				void* pData;
				mpRTPointLightSB->Map(0, nullptr, (void**)& pData);
				memcpy(pData, &(m_PointLights[0]), sizeof(PointLight) * numPointLights);
				mpRTPointLightSB->Unmap(0, nullptr);
			}
			// update the structure buffer of spotlight
			{
				void* pData;
				mpRTSpotLightSB->Map(0, nullptr, (void**)& pData);
				memcpy(pData, &(m_SpotLights[0]), sizeof(SpotLight) * numSpotLights);
				mpRTSpotLightSB->Unmap(0, nullptr);
			}
		}
	}
}

void HybridPipeline::OnRender(RenderEventArgs& e)
{
    super::OnRender(e);

    auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = commandQueue->GetCommandList();
	
	{// Rasterizing Pipe, deprecated
		{
			FLOAT clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
			commandList->ClearTexture(m_DeferredRenderTarget.GetTexture(AttachmentPoint::Color0), clearColor);
			commandList->ClearTexture(m_DeferredRenderTarget.GetTexture(AttachmentPoint::Color1), clearColor);
			commandList->ClearTexture(m_DeferredRenderTarget.GetTexture(AttachmentPoint::Color2), clearColor);
			commandList->ClearTexture(m_DeferredRenderTarget.GetTexture(AttachmentPoint::Color3), clearColor);
			commandList->ClearTexture(m_DeferredRenderTarget.GetTexture(AttachmentPoint::Color4), clearColor);
			commandList->ClearDepthStencilTexture(m_DeferredRenderTarget.GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH);
		}

		// Deferred Pipeline
		{
			commandList->SetViewport(m_Viewport);
			commandList->SetScissorRect(m_ScissorRect);
			commandList->SetRenderTarget(m_DeferredRenderTarget);
			commandList->SetPipelineState(m_DeferredPipelineState);
			commandList->SetGraphicsRootSignature(m_DeferredRootSignature);

			for (auto it = gameObjectPool.begin(); it != gameObjectPool.end(); it++) {
				it->second->Draw(*commandList, m_Camera, texturePool, meshPool);
			}
		}
	}

	{ // Let's raytrace
		commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, mpSrvUavHeap.Get());

		commandList->TransitionBarrier(*mpOutputTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->TransitionBarrier(m_DeferredRenderTarget.GetTexture(AttachmentPoint::Color0), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->TransitionBarrier(m_DeferredRenderTarget.GetTexture(AttachmentPoint::Color1), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->TransitionBarrier(m_DeferredRenderTarget.GetTexture(AttachmentPoint::Color2), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->TransitionBarrier(m_DeferredRenderTarget.GetTexture(AttachmentPoint::Color3), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->TransitionBarrier(m_DeferredRenderTarget.GetTexture(AttachmentPoint::Color4), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		D3D12_DISPATCH_RAYS_DESC raytraceDesc = {};
		raytraceDesc.Width = m_Viewport.Width;
		raytraceDesc.Height = m_Viewport.Height;
		raytraceDesc.Depth = 1;

		// RayGen is the first entry in the shader-table
		raytraceDesc.RayGenerationShaderRecord.StartAddress = mpShaderTable->GetGPUVirtualAddress() + 0 * mShaderTableEntrySize;
		raytraceDesc.RayGenerationShaderRecord.SizeInBytes = mShaderTableEntrySize;

		// Miss is the second entry in the shader-table
		size_t missOffset = 1 * mShaderTableEntrySize;
		raytraceDesc.MissShaderTable.StartAddress = mpShaderTable->GetGPUVirtualAddress() + missOffset;
		raytraceDesc.MissShaderTable.StrideInBytes = mShaderTableEntrySize;
		raytraceDesc.MissShaderTable.SizeInBytes = mShaderTableEntrySize;   // Only a s single miss-entry

		// Hit is the third entry in the shader-table
		size_t hitOffset = 2 * mShaderTableEntrySize;
		raytraceDesc.HitGroupTable.StartAddress = mpShaderTable->GetGPUVirtualAddress() + hitOffset;
		raytraceDesc.HitGroupTable.StrideInBytes = mShaderTableEntrySize;
		raytraceDesc.HitGroupTable.SizeInBytes = mShaderTableEntrySize;

		// Bind the empty root signature
		commandList->SetComputeRootSignature(mpEmptyRootSig);
		// Dispatch
		commandList->SetPipelineState1(mpPipelineState);
		commandList->DispatchRays(&raytraceDesc);
	}
	
	{ // Perform off-screen texture to RT post processing
		commandList->SetViewport(m_Viewport);
		commandList->SetScissorRect(m_ScissorRect);
		commandList->SetRenderTarget(m_pWindow->GetRenderTarget());
		commandList->SetPipelineState(m_PostProcessingPipelineState);
		commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->SetGraphicsRootSignature(m_PostProcessingRootSignature);
		commandList->SetShaderResourceView(0, 0, *mpOutputTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		//commandList->SetShaderResourceView(0, 0, m_DeferredRenderTarget.GetTexture(AttachmentPoint::Color3), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->Draw(3);
	}

    commandQueue->ExecuteCommandList(commandList);
    // Present
    m_pWindow->Present();
}

void HybridPipeline::OnKeyPressed(KeyEventArgs& e)
{
    super::OnKeyPressed(e);
    switch (e.Key)
    {
    case KeyCode::Escape:
        Application::Get().Quit(0);
        break;
    case KeyCode::Enter:
        if (e.Alt)
        {
    case KeyCode::F11:
        if (g_AllowFullscreenToggle)
        {
            m_pWindow->ToggleFullscreen();
            g_AllowFullscreenToggle = false;
        }
        break;
        }
    case KeyCode::V:
        m_pWindow->ToggleVSync();
        break;
    case KeyCode::R:
        // Reset camera transform
        m_Camera.set_Translation(m_pAlignedCameraData->m_InitialCamPos);
        m_Camera.set_Rotation(m_pAlignedCameraData->m_InitialCamRot);
        m_Pitch = 0.0f;
        m_Yaw = 0.0f;
        break;
    case KeyCode::Up:
    case KeyCode::W:
        m_Forward = 2.0f;
        break;
    case KeyCode::Left:
    case KeyCode::A:
        m_Left = 2.0f;
        break;
    case KeyCode::Down:
    case KeyCode::S:
        m_Backward = 2.0f;
        break;
    case KeyCode::Right:
    case KeyCode::D:
        m_Right = 2.0f;
        break;
    case KeyCode::Q:
        m_Down =2.0f;
        break;
    case KeyCode::E:
        m_Up = 2.0f;
        break;
    case KeyCode::Space:
        m_AnimateLights = !m_AnimateLights;
        break;
    case KeyCode::ShiftKey:
        //m_Shift = true;
		// disable the shift key
        break;
    }
    
}

void HybridPipeline::OnKeyReleased(KeyEventArgs& e)
{
    super::OnKeyReleased(e);

    switch (e.Key)
    {
    case KeyCode::Enter:
        if (e.Alt)
        {
    case KeyCode::F11:
        g_AllowFullscreenToggle = true;
        }
        break;
    case KeyCode::Up:
    case KeyCode::W:
        m_Forward = 0.0f;
        break;
    case KeyCode::Left:
    case KeyCode::A:
        m_Left = 0.0f;
        break;
    case KeyCode::Down:
    case KeyCode::S:
        m_Backward = 0.0f;
        break;
    case KeyCode::Right:
    case KeyCode::D:
        m_Right = 0.0f;
        break;
    case KeyCode::Q:
        m_Down = 0.0f;
        break;
    case KeyCode::E:
        m_Up = 0.0f;
        break;
    case KeyCode::ShiftKey:
        m_Shift = false;
        break;
    }
    
}

void HybridPipeline::OnMouseMoved(MouseMotionEventArgs& e)
{
    super::OnMouseMoved(e);

    const float mouseSpeed = 0.1f;
    if (e.LeftButton)
    {
        m_Pitch -= e.RelY * mouseSpeed;

        m_Pitch = Math::clamp(m_Pitch, -90.0f, 90.0f);

        m_Yaw -= e.RelX * mouseSpeed;
    }
    
}

void HybridPipeline::OnMouseWheel(MouseWheelEventArgs& e)
{
    auto fov = m_Camera.get_FoV();

    fov -= e.WheelDelta;
    fov = Math::clamp(fov, 12.0f, 90.0f);

    m_Camera.set_FoV(fov);

    char buffer[256];
    sprintf_s(buffer, "FoV: %f\n", fov);
    OutputDebugStringA(buffer);
}


////////////////////////////////////////////////////////////////////// RT Object 
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

void HybridPipeline::createAccelerationStructures()
{
	auto pDevice = Application::Get().GetDevice();
	auto commandQueue = Application::Get().GetCommandQueue();
	auto commandList = commandQueue->GetCommandList();

	//{ // mpVertexBuffer // 临时生成的三角形
	//	float vertices[][3] =
	//	{
	//		{0, 1.73205f,  0},
	//		{1.0f,  0, 0},
	//		{-1.0f, 0, 0},
	//	};
	//	// For simplicity, we create the vertex buffer on the upload heap, but that's not required
	//	mpVertexBuffer = createBuffer(pDevice, sizeof(vertices), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
	//	uint8_t* pData;
	//	mpVertexBuffer->Map(0, nullptr, (void**)& pData);
	//	memcpy(pData, vertices, sizeof(vertices));
	//	mpVertexBuffer->Unmap(0, nullptr);
	//	// 成员变量持有，全局记录持久资源的状态
	//	ResourceStateTracker::AddGlobalResourceState(mpVertexBuffer.Get(), D3D12_RESOURCE_STATE_GENERIC_READ);
	//	NAME_D3D12_OBJECT(mpVertexBuffer);
	//}
	
	std::vector<AccelerationStructureBuffers> bottomLevelBuffers;
	mpBottomLevelASes.resize(meshPool.size()); 
	bottomLevelBuffers.resize(meshPool.size());//Before the commandQueue finishes its execution of AS creation, these resources' lifecycle have to be held
	std::map<MeshIndex, int> MeshIndex2blasSeqCache; // Record MeshIndex to the sequence of mesh in mpBottomLevelASes

	int blasSeqCount=0;
	for (auto it = meshPool.begin(); it != meshPool.end(); it++) { //mpBottomLevelASes

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
		bottomLevelBuffers[blasSeqCount].pScratch = createBuffer(pDevice, info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kDefaultHeapProps);
		bottomLevelBuffers[blasSeqCount].pResult = createBuffer(pDevice, info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, kDefaultHeapProps);
		// 成员变量持有，全局记录持久资源的状态
		ASBuffer asBuffer;
		mpBottomLevelASes[blasSeqCount] = bottomLevelBuffers[blasSeqCount].pResult;
		asBuffer.SetD3D12Resource(mpBottomLevelASes[blasSeqCount]);
		asBuffer.SetName(L"BottomLevelAS"+string_2_wstring(it->first));
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
		int gameObjectCount = gameObjectPool.size();
		// First, get the size of the TLAS buffers and create them
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
		inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
		inputs.NumDescs = gameObjectCount;
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
		topLevelBuffers.pInstanceDesc = createBuffer(pDevice, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * gameObjectCount, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
		D3D12_RAYTRACING_INSTANCE_DESC* pInstanceDesc;
		topLevelBuffers.pInstanceDesc->Map(0, nullptr, (void**)& pInstanceDesc);
		ZeroMemory(pInstanceDesc, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * gameObjectCount);

		int tlasGoCount=0;
		for (auto it = gameObjectPool.begin(); it != gameObjectPool.end(); it++)
		{
			// Initialize the instance desc. We only have a single instance
			pInstanceDesc[tlasGoCount].InstanceID = tlasGoCount;                            // This value will be exposed to the shader via InstanceID()
			pInstanceDesc[tlasGoCount].InstanceContributionToHitGroupIndex = 0;   // This is the offset inside the shader-table. We only have a single geometry, so the offset 0
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

void HybridPipeline::createRtPipelineState()
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

	std::array<D3D12_STATE_SUBOBJECT, 10> subobjects;
	uint32_t index = 0;

	// Compile the shader
	auto pDxilLib = compileLibrary(L"RTPipeline/shaders/RayTracing.hlsl", L"lib_6_3");
	const WCHAR* entryPoints[] = { kRayGenShader, kMissShader ,kClosestHitShader };
	DxilLibrary dxilLib = DxilLibrary(pDxilLib, entryPoints, arraysize(entryPoints));
	subobjects[index++] = dxilLib.stateSubobject; // 0 Library

	//HitProgram hitProgram(kAnyHitShader, nullptr, kHitGroup);
	HitProgram hitProgram(0, kClosestHitShader, kHitGroup);
	subobjects[index++] = hitProgram.subObject; // 1 Hit Group

	// Create the ray-gen root-signature and association
	LocalRootSignature rgsRootSignature(mpDevice, createLocalRootDesc(1,8,3,0).desc);
	subobjects[index] = rgsRootSignature.subobject; // 2 RayGen Root Sig

	uint32_t rgsRootIndex = index++; // 2
	ExportAssociation rgsRootAssociation(&kRayGenShader, 1, &(subobjects[rgsRootIndex]));
	subobjects[index++] = rgsRootAssociation.subobject; // 3 Associate Root Sig to RGS

	// Create the miss- and hit-programs root-signature and association
	D3D12_ROOT_SIGNATURE_DESC emptyDesc = {};
	emptyDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	LocalRootSignature hitMissRootSignature(mpDevice, emptyDesc);
	subobjects[index] = hitMissRootSignature.subobject; // 4 Root Sig to be shared between Miss and CHS

	uint32_t hitMissRootIndex = index++; // 4
	const WCHAR* missHitExportName[] = { kMissShader, kClosestHitShader };
	ExportAssociation missHitRootAssociation(missHitExportName, arraysize(missHitExportName), &(subobjects[hitMissRootIndex]));
	subobjects[index++] = missHitRootAssociation.subobject; // 5 Associate Root Sig to Miss and CHS

	// Bind the payload size to the programs
	ShaderConfig shaderConfig(sizeof(float) * 2, sizeof(float) * 3);
	subobjects[index] = shaderConfig.subobject; // 6 Shader Config

	uint32_t shaderConfigIndex = index++; // 6
	const WCHAR* shaderExports[] = { kMissShader, kClosestHitShader, kRayGenShader };
	ExportAssociation configAssociation(shaderExports, arraysize(shaderExports), &(subobjects[shaderConfigIndex]));
	subobjects[index++] = configAssociation.subobject; // 7 Associate Shader Config to Miss, CHS, RGS

	// Create the pipeline config
	PipelineConfig config(1);
	subobjects[index++] = config.subobject; // 8

	// Create the global root signature and store the empty signature
	GlobalRootSignature root(mpDevice, {});
	mpEmptyRootSig = root.pRootSig;
	subobjects[index++] = root.subobject; // 9

	// Create the state
	D3D12_STATE_OBJECT_DESC desc;
	desc.NumSubobjects = index; // 10
	desc.pSubobjects = subobjects.data();
	desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

	ThrowIfFailed(mpDevice->CreateStateObject(&desc, IID_PPV_ARGS(&mpPipelineState)));
}

RootSignatureDesc HybridPipeline::createLocalRootDesc(int uav_num, int srv_num, int cbv_num, int c32_num)
{
	// Create the root-signature 
	//******Layout*********/
	// Param[0] = DescriptorTable
	/////////////// |---------------range for UAV---------------| |-------------------range for SRV-------------------|
	// Param[1-n] = 
	RootSignatureDesc desc;
	desc.range.resize(2);
	// UAV: gOutput 
	desc.range[0].BaseShaderRegister = 0;
	desc.range[0].NumDescriptors = uav_num;
	desc.range[0].RegisterSpace = 0;
	desc.range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	desc.range[0].OffsetInDescriptorsFromTableStart = 0;

	// SRV: gRtScene GBuffer(1-5)
	desc.range[1].BaseShaderRegister = 0;
	desc.range[1].NumDescriptors = srv_num;
	desc.range[1].RegisterSpace = 0;
	desc.range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	desc.range[1].OffsetInDescriptorsFromTableStart = uav_num;

	desc.rootParams.resize(1+ cbv_num + c32_num);

	desc.rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	desc.rootParams[0].DescriptorTable.NumDescriptorRanges = 2;
	desc.rootParams[0].DescriptorTable.pDescriptorRanges = desc.range.data();

	for (int i = 1; i <= cbv_num; i++) {
		desc.rootParams[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		desc.rootParams[i].Descriptor.RegisterSpace = 0;
		desc.rootParams[i].Descriptor.ShaderRegister = i-1;
	}

	for (int i = cbv_num+1; i <= cbv_num + c32_num; i++) {
		desc.rootParams[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		desc.rootParams[i].Constants.Num32BitValues = 1;
		desc.rootParams[i].Constants.RegisterSpace = 0;
		desc.rootParams[i].Constants.ShaderRegister = i - 1;
	}

	// Create the desc
	desc.desc.NumParameters = 1 + cbv_num+ c32_num;
	desc.desc.pParameters = desc.rootParams.data();
	desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	return desc;
}

void HybridPipeline::createShaderResourcesAndSrvUavheap()
{
	auto pDevice = Application::Get().GetDevice();
	//****************************UAV Resource
	// gOutput
	mpOutputTexture = std::make_shared<Texture>(CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R32G32B32A32_FLOAT, m_Viewport.Width, m_Viewport.Height,1,0,1,0, 
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_TEXTURE_LAYOUT_UNKNOWN,0
	), nullptr, TextureUsage::RenderTarget, L"mpOutputTexture");

	//****************************SRV Resource
	//gRtScene / GPosition / GAlbedo / GMetallic / GNormal / GRoughness
	//PointLights // SpotLight
	mpRTPointLightSB = createBuffer(pDevice, numPointLights * sizeof(PointLight), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
	mpRTSpotLightSB = createBuffer(pDevice, numSpotLights * sizeof(SpotLight), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);

	//****************************CBV Resource
	//create the constant buffer resource for cameraCB
	mpRTCameraCB = createBuffer(pDevice, sizeof(CameraRTCB), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ,kUploadHeapProps);
	mpRTLightPropertiesCB = createBuffer(pDevice, sizeof(LightPropertiesCB), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
	mpFrameIndexCB = createBuffer(pDevice, sizeof(FrameIndexCB), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);

	// Create the uavSrvHeap and its handle
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = 9;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mpSrvUavHeap)));
	D3D12_CPU_DESCRIPTOR_HANDLE uavSrvHandle = mpSrvUavHeap->GetCPUDescriptorHandleForHeapStart();

	// Create the UAV for GOutput
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavGOutputDesc = {};
	uavGOutputDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	pDevice->CreateUnorderedAccessView(mpOutputTexture->GetD3D12Resource().Get(), nullptr, &uavGOutputDesc, uavSrvHandle);
	uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Create srv for TLAS
	D3D12_SHADER_RESOURCE_VIEW_DESC srvTLASDesc = {};
	srvTLASDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvTLASDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvTLASDesc.RaytracingAccelerationStructure.Location = mpTopLevelAS->GetGPUVirtualAddress();
	pDevice->CreateShaderResourceView(nullptr, &srvTLASDesc, uavSrvHandle);
	uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//GBuffer
	for (int i = 0; i < 5; i++) {
		UINT pDestDescriptorRangeSizes[] = { 1 };
		pDevice->CopyDescriptors(1, &uavSrvHandle, pDestDescriptorRangeSizes,
			1, &m_DeferredRenderTarget.GetTexture((AttachmentPoint)i).GetShaderResourceView(), pDestDescriptorRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvPointLight = {};
	srvPointLight.Format = DXGI_FORMAT_UNKNOWN;
	srvPointLight.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;	
	srvPointLight.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvPointLight.Buffer.FirstElement = 0;
	srvPointLight.Buffer.NumElements = numPointLights;
	srvPointLight.Buffer.StructureByteStride = sizeof(PointLight);
	pDevice->CreateShaderResourceView(mpRTPointLightSB.Get(), &srvPointLight, uavSrvHandle);
	uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvSpotLight = {};
	srvSpotLight.Format = DXGI_FORMAT_UNKNOWN;
	srvSpotLight.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvSpotLight.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvSpotLight.Buffer.FirstElement = 0;
	srvSpotLight.Buffer.NumElements = numSpotLights;
	srvSpotLight.Buffer.StructureByteStride = sizeof(SpotLight);
	pDevice->CreateShaderResourceView(mpRTSpotLightSB.Get(), &srvSpotLight, uavSrvHandle);
	uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void HybridPipeline::createShaderTable()
{
	auto mpDevice = Application::Get().GetDevice();

	// Calculate the size and create the buffer
	mShaderTableEntrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	mShaderTableEntrySize += 24; // The ray-gen's descriptor table
	mShaderTableEntrySize = Math::AlignUp(mShaderTableEntrySize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
	uint32_t shaderTableSize = mShaderTableEntrySize * 3;

	// For simplicity, we create the shader-table on the upload heap. You can also create it on the default heap
	mpShaderTable = createBuffer(mpDevice, shaderTableSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
	NAME_D3D12_OBJECT(mpShaderTable);
	ResourceStateTracker::AddGlobalResourceState(mpShaderTable.Get(), D3D12_RESOURCE_STATE_GENERIC_READ);

	// Map the buffer
	uint8_t* pData;
	ThrowIfFailed(mpShaderTable->Map(0, nullptr, (void**)& pData));

	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> pRtsoProps;
	mpPipelineState->QueryInterface(IID_PPV_ARGS(&pRtsoProps));

	// Entry 0 - ray-gen program ID and descriptor data
	memcpy(pData, pRtsoProps->GetShaderIdentifier(kRayGenShader), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	uint64_t heapStart = mpSrvUavHeap->GetGPUDescriptorHandleForHeapStart().ptr;
	*(uint64_t*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = heapStart;
	*(D3D12_GPU_VIRTUAL_ADDRESS*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8) = mpRTCameraCB->GetGPUVirtualAddress();
	*(D3D12_GPU_VIRTUAL_ADDRESS*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8 +8) = mpRTLightPropertiesCB->GetGPUVirtualAddress();
	*(D3D12_GPU_VIRTUAL_ADDRESS*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8 + 8 + 8) = mpFrameIndexCB->GetGPUVirtualAddress();
	// Entry 1 - miss program
	memcpy(pData + mShaderTableEntrySize, pRtsoProps->GetShaderIdentifier(kMissShader), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// Entry 2 - hit program
	uint8_t* pHitEntry = pData + mShaderTableEntrySize * 2; // +2 skips the ray-gen and miss entries
	memcpy(pHitEntry, pRtsoProps->GetShaderIdentifier(kHitGroup), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// Unmap
	mpShaderTable->Unmap(0, nullptr);
}

void HybridPipeline::GameObject::Draw(CommandList& commandList, Camera& camera, std::map<TextureIndex, Texture>& texturePool, std::map<MeshIndex, std::shared_ptr<Mesh>>& meshPool)
{
	commandList.SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, transform.ComputeMatCB(camera.get_ViewMatrix(), camera.get_ProjectionMatrix()));
	commandList.SetGraphicsDynamicConstantBuffer(RootParameters::MaterialCB, material.base);
	commandList.SetGraphicsDynamicConstantBuffer(RootParameters::PBRMaterialCB, material.pbr);
	commandList.SetShaderResourceView(RootParameters::Textures, 0, (texturePool.find(material.tex.AlbedoTexture)!=texturePool.end())?texturePool[material.tex.AlbedoTexture]: texturePool["default_albedo"], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList.SetShaderResourceView(RootParameters::Textures, 1, (texturePool.find(material.tex.MetallicTexture) != texturePool.end()) ? texturePool[material.tex.MetallicTexture] : texturePool["default_metallic"], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList.SetShaderResourceView(RootParameters::Textures, 2, (texturePool.find(material.tex.NormalTexture) != texturePool.end()) ? texturePool[material.tex.NormalTexture] : texturePool["default_normal"], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList.SetShaderResourceView(RootParameters::Textures, 3, (texturePool.find(material.tex.RoughnessTexture) != texturePool.end()) ? texturePool[material.tex.RoughnessTexture] : texturePool["default_roughness"], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	if (meshPool.find(mesh) != meshPool.end()) {
		meshPool[mesh]->Draw(commandList);
	}
}
