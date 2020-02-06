#include <HybridPipeline.h>

#include <Application.h>
#include <CommandQueue.h>
#include <CommandList.h>
#include <Helpers.h>
#include <Light.h>
#include <Material.h>
#include <Window.h>

#include <wrl.h>
using namespace Microsoft::WRL;

#include <d3dx12.h>
#include <d3dcompiler.h>
#include <DirectXColors.h>
#include <DirectXMath.h>

using namespace DirectX;

#include <algorithm> // For std::min and std::max.
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

struct Mat
{
    XMMATRIX ModelMatrix;
    XMMATRIX InverseTransposeModelMatrix;
    XMMATRIX ModelViewProjectionMatrix;
};

struct LightProperties
{
    uint32_t NumPointLights;
    uint32_t NumSpotLights;
};

// An enum for root signature parameters.
// I'm not using scoped enums to avoid the explicit cast that would be required
// to use these as root indices in the root signature.
enum RootParameters
{
    MatricesCB,         // ConstantBuffer<Mat> MatCB : register(b0);
    MaterialCB,         // ConstantBuffer<Material> MaterialCB : register( b0, space1 );
    Textures,           // Texture2D DiffuseTexture : register( t0 );
    NumRootParameters
};

// Clamp a value between a min and max range.
template<typename T>
constexpr const T& clamp(const T& val, const T& min = T(0), const T& max = T(1))
{
    return val < min ? min : val > max ? max : val;
}

// Builds a look-at (world) matrix from a point, up and direction vectors.
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

    XMVECTOR cameraPos = XMVectorSet(0, 5, -20, 1);
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
    auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    auto commandList = commandQueue->GetCommandList();

    // Create a Cube mesh
    m_CubeMesh = Mesh::CreateCube(*commandList);
    m_SphereMesh = Mesh::CreateSphere(*commandList);
    m_ConeMesh = Mesh::CreateCone(*commandList);
    m_TorusMesh = Mesh::CreateTorus(*commandList);
    m_PlaneMesh = Mesh::CreatePlane(*commandList);
    // Create an inverted (reverse winding order) cube so the insides are not clipped.
    m_SkyboxMesh = Mesh::CreateCube(*commandList, 1.0f, true);

    // Load some textures
    commandList->LoadTextureFromFile(m_DefaultTexture, L"Assets/Textures/DefaultWhite.bmp", TextureUsage::Albedo);
    commandList->LoadTextureFromFile(m_EarthTexture, L"Assets/Textures/earth.dds", TextureUsage::Albedo);
    commandList->LoadTextureFromFile(m_MonaLisaTexture, L"Assets/Textures/Mona_Lisa.jpg", TextureUsage::Albedo);
    commandList->LoadTextureFromFile(m_GraceCathedralTexture, L"Assets/Textures/grace-new.hdr", TextureUsage::Albedo);

	// load PBR textures
	commandList->LoadTextureFromFile(m_default_albedo_texture, L"Assets/Textures/pbr/default/albedo.bmp", TextureUsage::Albedo);
	commandList->LoadTextureFromFile(m_default_metallic_texture, L"Assets/Textures/pbr/default/metallic.bmp", TextureUsage::MetallicMap);
	commandList->LoadTextureFromFile(m_default_normal_texture, L"Assets/Textures/pbr/default/normal.bmp", TextureUsage::Normalmap);
	commandList->LoadTextureFromFile(m_default_roughness_texture, L"Assets/Textures/pbr/default/roughness.bmp", TextureUsage::RoughnessMap);
	commandList->LoadTextureFromFile(m_rustedIron_albedo_texture, L"Assets/Textures/pbr/rusted_iron/albedo.png", TextureUsage::Albedo);
	commandList->LoadTextureFromFile(m_rustedIron_metallic_texture, L"Assets/Textures/pbr/rusted_iron/metallic.png", TextureUsage::MetallicMap);
	commandList->LoadTextureFromFile(m_rustedIron_normal_texture, L"Assets/Textures/pbr/rusted_iron/normal.png", TextureUsage::Normalmap);
	commandList->LoadTextureFromFile(m_rustedIron_roughness_texture, L"Assets/Textures/pbr/rusted_iron/roughness.png", TextureUsage::RoughnessMap);

    // Create a cubemap for the HDR panorama.
    auto cubemapDesc = m_GraceCathedralTexture.GetD3D12ResourceDesc();
    cubemapDesc.Width = cubemapDesc.Height = 1024;
    cubemapDesc.DepthOrArraySize = 6;
    cubemapDesc.MipLevels = 0;

    m_GraceCathedralCubemap = Texture(cubemapDesc, nullptr, TextureUsage::Albedo, L"Grace Cathedral Cubemap");
    // Convert the 2D panorama to a 3D cubemap.
    commandList->PanoToCubemap(m_GraceCathedralCubemap, m_GraceCathedralTexture);

    // Create an HDR intermediate render target.
    DXGI_FORMAT HDRFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

    // Disable the multiple sampling for performance and simplicity
	DXGI_SAMPLE_DESC sampleDesc = {1, 0}/*Application::Get().GetMultisampleQualityLevels(HDRFormat, D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT)*/;

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

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    // Create a root signature for the Deferred Shadings pipeline.
    {
        // Load the Deferred shaders.
        ComPtr<ID3DBlob> vs;
        ComPtr<ID3DBlob> ps;
        ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/HybridPipeline/Deferred_VS.cso", &vs));
        ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/HybridPipeline/Deferred_PS.cso", &ps));

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

    // Create the SDR Root Signature
    {
        CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

        CD3DX12_ROOT_PARAMETER1 rootParameters[1];
        rootParameters[0].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
        rootSignatureDescription.Init_1_1(1, rootParameters);

        m_SDRRootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

        // Create the SDR PSO
        ComPtr<ID3DBlob> vs;
        ComPtr<ID3DBlob> ps;
        ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/HybridPipeline/PostProcessing_VS.cso", &vs));
        ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/HybridPipeline/PostProcessing_PS.cso", &ps));

        CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
        rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

        struct SDRPipelineStateStream
        {
            CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
            CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
            CD3DX12_PIPELINE_STATE_STREAM_VS VS;
            CD3DX12_PIPELINE_STATE_STREAM_PS PS;
            CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterizer;
            CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
        } sdrPipelineStateStream;

        sdrPipelineStateStream.pRootSignature = m_SDRRootSignature.GetRootSignature().Get();
        sdrPipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        sdrPipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vs.Get());
        sdrPipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
        sdrPipelineStateStream.Rasterizer = rasterizerDesc;
        sdrPipelineStateStream.RTVFormats = m_pWindow->GetRenderTarget().GetRenderTargetFormats();

        D3D12_PIPELINE_STATE_STREAM_DESC sdrPipelineStateStreamDesc = {
            sizeof(SDRPipelineStateStream), &sdrPipelineStateStream
        };
        ThrowIfFailed(device->CreatePipelineState(&sdrPipelineStateStreamDesc, IID_PPV_ARGS(&m_SDRPipelineState)));
    }

    auto fenceValue = commandQueue->ExecuteCommandList(commandList);
    commandQueue->WaitForFenceValue(fenceValue);

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
    static double totalTime = 0.0;

    super::OnUpdate(e);

    totalTime += e.ElapsedTime;
    frameCount++;

    if (totalTime > 1.0)
    {
        double fps = frameCount / totalTime;

        char buffer[512];
        sprintf_s(buffer, "FPS: %f\n", fps);
        OutputDebugStringA(buffer);

        frameCount = 0;
        totalTime = 0.0;
    }

    // Update the camera.
    float speedMultipler = (m_Shift ? 16.0f : 4.0f);

    XMVECTOR cameraTranslate = XMVectorSet(m_Right - m_Left, 0.0f, m_Forward - m_Backward, 1.0f) * speedMultipler * static_cast<float>(e.ElapsedTime);
    XMVECTOR cameraPan = XMVectorSet(0.0f, m_Up - m_Down, 0.0f, 1.0f) * speedMultipler * static_cast<float>(e.ElapsedTime);
    m_Camera.Translate(cameraTranslate, Space::Local);
    m_Camera.Translate(cameraPan, Space::Local);

    XMVECTOR cameraRotation = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(m_Pitch), XMConvertToRadians(m_Yaw), 0.0f);
    m_Camera.set_Rotation(cameraRotation);

    XMMATRIX viewMatrix = m_Camera.get_ViewMatrix();

    const int numPointLights = 4;
    const int numSpotLights = 4;

    static const XMVECTORF32 LightColors[] =
    {
        Colors::White, Colors::Orange, Colors::Yellow, Colors::Green, Colors::Blue, Colors::Indigo, Colors::Violet, Colors::White
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
        XMVECTOR positionVS = XMVector3TransformCoord(positionWS, viewMatrix);
        XMStoreFloat4(&l.PositionVS, positionVS);

        l.Color = XMFLOAT4(LightColors[i]);
        l.Intensity = 1.0f;
        l.Attenuation = 0.0f;
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
        XMVECTOR positionVS = XMVector3TransformCoord(positionWS, viewMatrix);
        XMStoreFloat4(&l.PositionVS, positionVS);

        XMVECTOR directionWS = XMVector3Normalize(XMVectorSetW(XMVectorNegate(positionWS), 0));
        XMVECTOR directionVS = XMVector3Normalize(XMVector3TransformNormal(directionWS, viewMatrix));
        XMStoreFloat4(&l.DirectionWS, directionWS);
        XMStoreFloat4(&l.DirectionVS, directionVS);

        l.Color = XMFLOAT4(LightColors[numPointLights + i]);
        l.Intensity = 1.0f;
        l.SpotAngle = XMConvertToRadians(45.0f);
        l.Attenuation = 0.0f;
    }
}

void XM_CALLCONV ComputeMatrices(FXMMATRIX model, CXMMATRIX view, CXMMATRIX viewProjection, Mat& mat)
{
    mat.ModelMatrix = model;
    mat.InverseTransposeModelMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, mat.ModelMatrix));
    mat.ModelViewProjectionMatrix = model * viewProjection;
}

void HybridPipeline::OnRender(RenderEventArgs& e)
{
    super::OnRender(e);

    auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = commandQueue->GetCommandList();

    // Clear the render targets.
    {
        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

        commandList->ClearTexture(m_DeferredRenderTarget.GetTexture(AttachmentPoint::Color0), clearColor);
		commandList->ClearTexture(m_DeferredRenderTarget.GetTexture(AttachmentPoint::Color1), clearColor);
		commandList->ClearTexture(m_DeferredRenderTarget.GetTexture(AttachmentPoint::Color2), clearColor);
		commandList->ClearTexture(m_DeferredRenderTarget.GetTexture(AttachmentPoint::Color3), clearColor);
		commandList->ClearTexture(m_DeferredRenderTarget.GetTexture(AttachmentPoint::Color4), clearColor);
        commandList->ClearDepthStencilTexture(m_DeferredRenderTarget.GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH);
    }

    commandList->SetViewport(m_Viewport);
    commandList->SetScissorRect(m_ScissorRect);
    commandList->SetRenderTarget(m_DeferredRenderTarget);

	// Deferred Pipeline
	{
		commandList->SetPipelineState(m_DeferredPipelineState);
		commandList->SetGraphicsRootSignature(m_DeferredRootSignature);

		// Draw the earth sphere
		XMMATRIX translationMatrix = XMMatrixTranslation(-4.0f, 2.0f, -4.0f);
		XMMATRIX rotationMatrix = XMMatrixIdentity();
		XMMATRIX scaleMatrix = XMMatrixScaling(4.0f, 4.0f, 4.0f);
		XMMATRIX worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;
		XMMATRIX viewMatrix = m_Camera.get_ViewMatrix();
		XMMATRIX viewProjectionMatrix = viewMatrix * m_Camera.get_ProjectionMatrix();

		Mat matrices;
		ComputeMatrices(worldMatrix, viewMatrix, viewProjectionMatrix, matrices);

		commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, matrices);
		commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MaterialCB, Material::White);
		commandList->SetShaderResourceView(RootParameters::Textures, 0, m_rustedIron_albedo_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(RootParameters::Textures, 1, m_rustedIron_metallic_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(RootParameters::Textures, 2, m_rustedIron_normal_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(RootParameters::Textures, 3, m_rustedIron_roughness_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		m_SphereMesh->Draw(*commandList);

		// Draw a cube
		translationMatrix = XMMatrixTranslation(4.0f, 2.0f, 4.0f);
		rotationMatrix = XMMatrixRotationY(XMConvertToRadians(45.0f));
		scaleMatrix = XMMatrixScaling(4.0f, 4.0f, 4.0f);
		worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;

		ComputeMatrices(worldMatrix, viewMatrix, viewProjectionMatrix, matrices);

		commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, matrices);
		commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MaterialCB, Material::White);
		commandList->SetShaderResourceView(RootParameters::Textures, 0, m_rustedIron_albedo_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(RootParameters::Textures, 1, m_rustedIron_metallic_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(RootParameters::Textures, 2, m_rustedIron_normal_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(RootParameters::Textures, 3, m_rustedIron_roughness_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		m_CubeMesh->Draw(*commandList);

		//// Draw a torus
		translationMatrix = XMMatrixTranslation(4.0f, 0.6f, -4.0f);
		rotationMatrix = XMMatrixRotationY(XMConvertToRadians(45.0f));
		scaleMatrix = XMMatrixScaling(4.0f, 4.0f, 4.0f);
		worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;

		ComputeMatrices(worldMatrix, viewMatrix, viewProjectionMatrix, matrices);

		commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, matrices);
		commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MaterialCB, Material::White);
		commandList->SetShaderResourceView(RootParameters::Textures, 0, m_rustedIron_albedo_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(RootParameters::Textures, 1, m_rustedIron_metallic_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(RootParameters::Textures, 2, m_rustedIron_normal_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(RootParameters::Textures, 3, m_rustedIron_roughness_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		m_TorusMesh->Draw(*commandList);

		// Floor plane.
		float scalePlane = 20.0f;
		float translateOffset = scalePlane / 2.0f;

		translationMatrix = XMMatrixTranslation(0.0f, 0.0f, 0.0f);
		rotationMatrix = XMMatrixIdentity();
		scaleMatrix = XMMatrixScaling(scalePlane, 1.0f, scalePlane);
		worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;

		ComputeMatrices(worldMatrix, viewMatrix, viewProjectionMatrix, matrices);

		commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, matrices);
		commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MaterialCB, Material::Red);
		commandList->SetShaderResourceView(RootParameters::Textures, 0, m_default_albedo_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(RootParameters::Textures, 1, m_default_metallic_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(RootParameters::Textures, 2, m_default_normal_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(RootParameters::Textures, 3, m_default_roughness_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		m_PlaneMesh->Draw(*commandList);

		// Back wall
		translationMatrix = XMMatrixTranslation(0, translateOffset, translateOffset);
		rotationMatrix = XMMatrixRotationX(XMConvertToRadians(-90));
		worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;

		ComputeMatrices(worldMatrix, viewMatrix, viewProjectionMatrix, matrices);

		commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, matrices);
		commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MaterialCB, Material::Green);
		commandList->SetShaderResourceView(RootParameters::Textures, 0, m_default_albedo_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(RootParameters::Textures, 1, m_default_metallic_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(RootParameters::Textures, 2, m_default_normal_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(RootParameters::Textures, 3, m_default_roughness_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		m_PlaneMesh->Draw(*commandList);

		// Ceiling plane
		translationMatrix = XMMatrixTranslation(0, translateOffset * 2.0f, 0);
		rotationMatrix = XMMatrixRotationX(XMConvertToRadians(180));
		worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;

		ComputeMatrices(worldMatrix, viewMatrix, viewProjectionMatrix, matrices);

		commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, matrices);
		commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MaterialCB, Material::Blue);
		commandList->SetShaderResourceView(RootParameters::Textures, 0, m_default_albedo_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(RootParameters::Textures, 1, m_default_metallic_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(RootParameters::Textures, 2, m_default_normal_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(RootParameters::Textures, 3, m_default_roughness_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		m_PlaneMesh->Draw(*commandList);

		// Front wall
		translationMatrix = XMMatrixTranslation(0, translateOffset, -translateOffset);
		rotationMatrix = XMMatrixRotationX(XMConvertToRadians(90));
		worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;

		ComputeMatrices(worldMatrix, viewMatrix, viewProjectionMatrix, matrices);

		commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, matrices);
		commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MaterialCB, Material::Yellow);
		commandList->SetShaderResourceView(RootParameters::Textures, 0, m_default_albedo_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(RootParameters::Textures, 1, m_default_metallic_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(RootParameters::Textures, 2, m_default_normal_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(RootParameters::Textures, 3, m_default_roughness_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		m_PlaneMesh->Draw(*commandList);

		// Left wall
		translationMatrix = XMMatrixTranslation(-translateOffset, translateOffset, 0);
		rotationMatrix = XMMatrixRotationX(XMConvertToRadians(-90)) * XMMatrixRotationY(XMConvertToRadians(-90));
		worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;

		ComputeMatrices(worldMatrix, viewMatrix, viewProjectionMatrix, matrices);

		commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, matrices);
		commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MaterialCB, Material::Cyan);
		commandList->SetShaderResourceView(RootParameters::Textures, 0, m_default_albedo_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(RootParameters::Textures, 1, m_default_metallic_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(RootParameters::Textures, 2, m_default_normal_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(RootParameters::Textures, 3, m_default_roughness_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		m_PlaneMesh->Draw(*commandList);

		// Right wall
		translationMatrix = XMMatrixTranslation(translateOffset, translateOffset, 0);
		rotationMatrix = XMMatrixRotationX(XMConvertToRadians(-90)) * XMMatrixRotationY(XMConvertToRadians(90));
		worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;

		ComputeMatrices(worldMatrix, viewMatrix, viewProjectionMatrix, matrices);

		commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, matrices);
		commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MaterialCB, Material::Magenta);
		commandList->SetShaderResourceView(RootParameters::Textures, 0, m_default_albedo_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(RootParameters::Textures, 1, m_default_metallic_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(RootParameters::Textures, 2, m_default_normal_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(RootParameters::Textures, 3, m_default_roughness_texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		m_PlaneMesh->Draw(*commandList);
	}
    

    // Draw shapes to visualize the position of the lights in the scene.
	//{
	//	Material lightMaterial;
	//	// No specular
	//	lightMaterial.Specular = { 0, 0, 0, 1 };

	//	// MVP matrices
	//	XMMATRIX worldMatrix;
	//	XMMATRIX rotationMatrix;
	//	XMMATRIX viewMatrix = m_Camera.get_ViewMatrix();
	//	XMMATRIX viewProjectionMatrix = viewMatrix * m_Camera.get_ProjectionMatrix();
	//	Mat matrices;

	//	for (const auto& l : m_PointLights)
	//	{
	//		lightMaterial.Emissive = l.Color;
	//		XMVECTOR lightPos = XMLoadFloat4(&l.PositionWS);
	//		worldMatrix = XMMatrixTranslationFromVector(lightPos);
	//		ComputeMatrices(worldMatrix, viewMatrix, viewProjectionMatrix, matrices);

	//		commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, matrices);
	//		commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MaterialCB, lightMaterial);

	//		m_SphereMesh->Draw(*commandList);
	//	}

	//	for (const auto& l : m_SpotLights)
	//	{
	//		lightMaterial.Emissive = l.Color;
	//		XMVECTOR lightPos = XMLoadFloat4(&l.PositionWS);
	//		XMVECTOR lightDir = XMLoadFloat4(&l.DirectionWS);
	//		XMVECTOR up = XMVectorSet(0, 1, 0, 0);

	//		// Rotate the cone so it is facing the Z axis.
	//		rotationMatrix = XMMatrixRotationX(XMConvertToRadians(-90.0f));
	//		worldMatrix = rotationMatrix * LookAtMatrix(lightPos, lightDir, up);

	//		ComputeMatrices(worldMatrix, viewMatrix, viewProjectionMatrix, matrices);

	//		commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MatricesCB, matrices);
	//		commandList->SetGraphicsDynamicConstantBuffer(RootParameters::MaterialCB, lightMaterial);

	//		m_ConeMesh->Draw(*commandList);
	//	}

	//}

	// Perform off-screen texture to RT post processing
	{
		commandList->SetRenderTarget(m_pWindow->GetRenderTarget());
		commandList->SetPipelineState(m_SDRPipelineState);
		commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->SetGraphicsRootSignature(m_SDRRootSignature);
		commandList->SetShaderResourceView(0, 0, m_DeferredRenderTarget.GetTexture(Color3), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		commandList->Draw(3);
	}

    commandQueue->ExecuteCommandList(commandList);
    // Present
    m_pWindow->Present();
}

static bool g_AllowFullscreenToggle = true;

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
        m_Shift = true;
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

        m_Pitch = clamp(m_Pitch, -90.0f, 90.0f);

        m_Yaw -= e.RelX * mouseSpeed;
    }
    
}


void HybridPipeline::OnMouseWheel(MouseWheelEventArgs& e)
{
    auto fov = m_Camera.get_FoV();

    fov -= e.WheelDelta;
    fov = clamp(fov, 12.0f, 90.0f);

    m_Camera.set_FoV(fov);

    char buffer[256];
    sprintf_s(buffer, "FoV: %f\n", fov);
    OutputDebugStringA(buffer);
}
