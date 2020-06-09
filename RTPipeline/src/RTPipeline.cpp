#include <RTPipeline.h>
#include <Application.h>
#include <CommandQueue.h>
#include <CommandList.h>
#include <Helpers.h>
#include <Light.h>

#include <Window.h>

#include <wrl.h>

#include <io.h>
using namespace Microsoft::WRL;

#include <d3dx12.h>
#include <d3dcompiler.h>
#include <DirectXColors.h>
#include <ResourceStateTracker.h>
#include <ASBuffer.h>
#include <Helpers.h>
#include <DirectXMath.h>

using namespace DirectX;

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

#define SCENE1 1
#define GLOBAL_BENCHMARK_LIMIT 1*120.0
static bool g_AllowFullscreenToggle = true;
static uint64_t frameCount = 0;
static uint64_t globalFrameCount = 0;
static double totalTime = 0.0;
static double globalTime = 0.0;
static float cameraAnimTime = 0.0f;

////////////////////////////////////////////////////////////////////// framework

HybridPipeline::HybridPipeline(const std::wstring& name, int width, int height, bool vSync)
    : super(name, width, height, vSync)
    , m_Forward(0)
    , m_Backward(0)
    , m_Left(0)
    , m_Right(0)
    , m_Up(0)
    , m_Down(0)
    , m_Pitch(0)
    , m_Yaw(0)
	, m_AnimateCamera(false)
    , m_AnimateLights(false)
	, m_Width(0)
	, m_Height(0)
{
	m_Scene = std::make_shared<Scene>();
	m_Renderers.push_back(std::make_shared<SkyboxRenderer>(m_Width, m_Height));
}

HybridPipeline::~HybridPipeline()
{
    
}

bool HybridPipeline::LoadContent()
{
	// build Scene
	loadResource();
	loadGameObject();
	transformGameObject();

	m_RenderResourceMap.emplace(RRD_RENDERTARGET_PRESENT,std::make_shared<Texture>(m_pWindow->GetRenderTarget().GetTexture(AttachmentPoint::Color0)));
	
	for (auto& pRenderer : m_Renderers) {
		pRenderer->LoadResource(m_Scene, m_RenderResourceMap);
		pRenderer->LoadPipeline();
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
        m_Scene->m_Camera.set_Projection(45.0f, aspectRatio, 0.1f, 100.0f);

		for (auto& pRenderer : m_Renderers) {
			pRenderer->Resize(m_Width, m_Height);
		}
    }
}

void HybridPipeline::UnloadContent()
{
}

void HybridPipeline::OnUpdate(UpdateEventArgs& e)
{
    super::OnUpdate(e);

	// frametime
	{
		globalTime+= e.ElapsedTime;
		globalFrameCount++;

		totalTime += e.ElapsedTime;
		frameCount++;

		if (totalTime > 1.0)
		{
			double fps = frameCount / totalTime;
			char buffer[512];
			sprintf_s(buffer, "FPS: %f,  FrameCount: %lld\n", fps, globalFrameCount);
			OutputDebugStringA(buffer);

			frameCount = 0;
			totalTime = 0.0;
		}
	}

	if (globalTime >= GLOBAL_BENCHMARK_LIMIT) {
		Application::Get().Quit(0);
	}

	// camera
	{
		if (m_AnimateCamera)
		{
			cameraAnimTime += static_cast<float>(e.ElapsedTime) * 0.5f * XM_PI/10.0f;
			// Setup the lights.
			const float radius = 35.0f;
			XMVECTOR cameraPos = XMVectorSet(static_cast<float>(std::sin(cameraAnimTime))* radius,
																			10,
																			static_cast<float>(std::cos(3.141562653 + cameraAnimTime))* radius,
																			1.0);
			XMVECTOR cameraTarget = XMVectorSet(0, 10, 0, 1);
			XMVECTOR cameraUp = XMVectorSet(0, 1, 0, 0);

			m_Scene->m_Camera.set_LookAt(cameraPos, cameraTarget, cameraUp);
		}
		else {
			float speedMultipler = 4.0f;

			XMVECTOR cameraTranslate = XMVectorSet(m_Right - m_Left, 0.0f, m_Forward - m_Backward, 1.0f) * speedMultipler * static_cast<float>(e.ElapsedTime);
			XMVECTOR cameraPan = XMVectorSet(0.0f, m_Up - m_Down, 0.0f, 1.0f) * speedMultipler * static_cast<float>(e.ElapsedTime);
			m_Scene->m_Camera.Translate(cameraTranslate, Space::Local);
			m_Scene->m_Camera.Translate(cameraPan, Space::Local);

			XMVECTOR cameraRotation = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(m_Pitch), XMConvertToRadians(m_Yaw), 0.0f);
			m_Scene->m_Camera.set_Rotation(cameraRotation);

			XMMATRIX viewMatrix = m_Scene->m_Camera.get_ViewMatrix();
		}
	}

	// light
	{ 
		static float lightAnimTime = 1.0f * XM_PI;
		if (m_AnimateLights)
		{
			lightAnimTime += static_cast<float>(e.ElapsedTime) * 0.5f * XM_PI;
		}

		// Setup the lights.
		const float radius = 6.0f;
		PointLight& l = m_Scene->m_PointLight;
		l.PositionWS = {
			static_cast<float>(std::sin(lightAnimTime )) * radius,
			9,
			static_cast<float>(std::cos(lightAnimTime)) * radius,
			1.0f
		};
		l.Color = XMFLOAT4(Colors::White);
		l.Intensity = 2.0f;
		l.Attenuation = 0.01f;
		l.Radius = 2.0f;

		// Update the pointlight gameobject
		{
			m_Scene->gameObjectPool[m_Scene->lightObjectIndex]->Translate(XMMatrixTranslation(l.PositionWS.x, l.PositionWS.y, l.PositionWS.z));
			m_Scene->gameObjectPool[m_Scene->lightObjectIndex]->Scale(XMMatrixScaling(l.Radius, l.Radius, l.Radius));
		}
	}

	for (auto& pRenderer : m_Renderers) {
		pRenderer->Update(e, m_Scene);
	}
}

void HybridPipeline::OnRender(RenderEventArgs& e)
{
    super::OnRender(e);

    auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = commandQueue->GetCommandList();
	
	m_RenderResourceMap[RRD_RENDERTARGET_PRESENT]=std::make_shared<Texture>(m_pWindow->GetRenderTarget().GetTexture(AttachmentPoint::Color0));

	for (auto& pRenderer : m_Renderers) {
		// main render skeleton: Pre Rendering
		pRenderer->PreRender(m_RenderResourceMap);

		// main render skeleton: Render it!
		pRenderer->PreRender(m_RenderResourceMap);
		pRenderer->Render(e, m_Scene, commandList);

		// main render skeleton: After Rendering
		auto pRetResourceMap = pRenderer->PostRender();

		//对比是否是同一个Map
		if (pRetResourceMap != &m_RenderResourceMap) {//如果不是
			for (auto pRes = pRetResourceMap->begin(); pRes != pRetResourceMap->end(); pRes++) {
				if (m_RenderResourceMap.find(pRes->first) == m_RenderResourceMap.end()) {//则在Map中新增缺的内容
					m_RenderResourceMap.emplace(pRes->first, pRes->second);
				}
			}
		}
	}

	commandQueue->ExecuteCommandList(commandList);
	// Present
	m_pWindow->Present();

	//// prev buffer cache
	//{
	//	auto swapCurrPrevTexture = [](Texture& t1, Texture& t2) {
	//		Texture tmp = t2;
	//		t2 = t1;
	//		t1 = tmp;
	//	};
	//	viewProjectMatrix_prev = m_Scene->m_Camera.get_ViewMatrix() * m_Scene->m_Camera.get_ProjectionMatrix();
	//	swapCurrPrevTexture(gPosition_prev, gPosition);
	//	swapCurrPrevTexture(gAlbedoMetallic_prev, gAlbedoMetallic);
	//	swapCurrPrevTexture(gNormalRoughness_prev, gNormalRoughness);
	//	swapCurrPrevTexture(gExtra_prev, gExtra);
	//	swapCurrPrevTexture(col_acc_prev, col_acc);
	//	swapCurrPrevTexture(moment_acc_prev, moment_acc);
	//	swapCurrPrevTexture(his_length_prev, his_length);
	//	swapCurrPrevTexture(noisy_prev, noisy_curr);
	//	swapCurrPrevTexture(spp_prev, spp_curr);
	//	swapCurrPrevTexture(filtered_prev, filtered_curr);

	//	// G Buffer re-orgnizating
	//	m_GBuffer.AttachTexture(AttachmentPoint::Color0, gPosition);
	//	m_GBuffer.AttachTexture(AttachmentPoint::Color1, gAlbedoMetallic);
	//	m_GBuffer.AttachTexture(AttachmentPoint::Color2, gNormalRoughness);
	//	m_GBuffer.AttachTexture(AttachmentPoint::Color3, gExtra);
	//}


   
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
		m_Scene->m_Camera.set_Translation(m_Scene->m_pAlignedCameraData->m_InitialCamPos);
		m_Scene->m_Camera.set_Rotation(m_Scene->m_pAlignedCameraData->m_InitialCamRot);
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
    case KeyCode::L:
        m_AnimateLights = !m_AnimateLights;
        break;
	case KeyCode::C:
		m_AnimateCamera = !m_AnimateCamera;
		cameraAnimTime = 0.0f;
		if (!m_AnimateCamera) {
			m_Scene->m_Camera.set_Translation(m_Scene->m_pAlignedCameraData->m_InitialCamPos);
			m_Scene->m_Camera.set_Rotation(m_Scene->m_pAlignedCameraData->m_InitialCamRot);
			m_Pitch = 0.0f;
			m_Yaw = 0.0f;
		}
		break;
	case KeyCode::X:
		//postTestingCB.inc();
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
    auto fov = m_Scene->m_Camera.get_FoV();

    fov -= e.WheelDelta;
    fov = Math::clamp(fov, 12.0f, 90.0f);

	m_Scene->m_Camera.set_FoV(fov);

    char buffer[256];
    sprintf_s(buffer, "FoV: %f\n", fov);
    OutputDebugStringA(buffer);
}

////////////////////////////////////////////////////////////////////// internal update
void HybridPipeline::loadResource() {
	auto device = Application::Get().GetDevice();
	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue->GetCommandList();
	auto& meshPool = MeshPool::Get(); auto& texturePool = TexturePool::Get();

	// Create a Cube mesh
	meshPool.addMesh("cube", Mesh::CreateCube(*commandList));
	meshPool.addMesh("sphere", Mesh::CreateSphere(*commandList));
	meshPool.addMesh("cone", Mesh::CreateCone(*commandList));
	meshPool.addMesh("torus", Mesh::CreateTorus(*commandList));
	meshPool.addMesh("plane", Mesh::CreatePlane(*commandList));

	// Create a skybox mesh
	m_Scene->setSkybox("skybox_cubemap", Mesh::CreateCube(*commandList, 1.0f, true));
	// load cubemap
#ifdef SCENE1
	texturePool.loadTexture("skybox_pano", "Assets/HDR/skybox_default.hdr", TextureUsage::Albedo, *commandList);
#elif SCENE2
	texturePool.loadTexture("skybox_pano", "Assets/HDR/Milkyway_BG.hdr", TextureUsage::Albedo, *commandList);
#else
	texturePool.loadTexture("skybox_pano", "Assets/HDR/Ice_Lake_HiRes_TMap_2.hdr", TextureUsage::Albedo, *commandList);
#endif
	texturePool.loadCubemap(1024, "skybox_pano", "skybox_cubemap", *commandList);
	// load PBR textures
	
	texturePool.loadPBRSeriesTexture(DEFAULT_PBR_HEADER, "Assets/Textures/pbr/default/albedo.bmp", *commandList);
	texturePool.loadPBRSeriesTexture("rusted_iron", "Assets/Textures/pbr/rusted_iron/albedo.png", *commandList);
	texturePool.loadPBRSeriesTexture("grid_metal", "Assets/Textures/pbr/grid_metal/albedo.png", *commandList);
	texturePool.loadPBRSeriesTexture("metal", "Assets/Textures/pbr/metal/albedo.png", *commandList);

	// run ocmmandlist
	auto fenceValue = commandQueue->ExecuteCommandList(commandList);
	commandQueue->WaitForFenceValue(fenceValue);
}
void HybridPipeline::loadGameObject() {
	
	auto device = Application::Get().GetDevice();
	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue->GetCommandList();


	m_Scene->addSingleGameObject("plane", { 0.5f, 0.4f, Material::White }, "Floor plane");

#ifdef SCENE1
	m_Scene->addSingleGameObject( "plane", { 0.5f, 0.5f, Material::White }, "Ceiling plane");
	m_Scene->addSingleGameObject( "plane", { 0.4f, 0.1f, Material::Pearl }, "Back wall");
	m_Scene->addSingleGameObject( "plane", { 0.4f, 0.4f, Material::Copper }, "Front wall");
	m_Scene->addSingleGameObject( "plane", { 0.4f, 0.3f, Material::Jade }, "Left wall");
	m_Scene->addSingleGameObject( "plane", { 0.4f, 0.3f, Material::Ruby }, "Right wall");
	m_Scene->addSingleGameObject( "sphere", { "rusted_iron" }, "sphere");
	m_Scene->addSingleGameObject("cube", { "grid_metal" }, "cube");
	m_Scene->addSingleGameObject( "torus", { "metal" }, "torus");
#endif
#ifdef SCENE2
	//load external object
	m_Scene->importModel(commandList, "Assets/Cerberus/Cerberus_LP.FBX", "A.tga", "M.tga", "N.tga", "R.tga");
	m_Scene->importModel(commandList, "Assets/Unreal-actor/model.dae");
	m_Scene->importModel(commandList, "Assets/Sci-fi-Biolab/source/SciFiBiolab.fbx", "COLOR","METALLIC","NORMAL","ROUGHNESS");
#endif
#ifdef SCENE3
	m_Scene->importModel(commandList, "Assets/SM_Chair/SM_Chair.FBX");
	m_Scene->importModel(commandList, "Assets/SM_TableRound/SM_TableRound.FBX");
	m_Scene->importModel(commandList, "Assets/SM_Couch/SM_Couch.FBX");
	m_Scene->importModel(commandList, "Assets/SM_Lamp_Ceiling/SM_Lamp_Ceiling.FBX");
	m_Scene->importModel(commandList, "Assets/SM_MatPreviewMesh/SM_MatPreviewMesh.FBX");
	m_Scene->copyGameObjectAssembling("Assets/SM_Chair", "Assets/SM_Chair_copy");
#endif
	// light object
	m_Scene->addSingleLight("sphere", Material::EmissiveWhite, "sphere light");

	auto fenceValue = commandQueue->ExecuteCommandList(commandList);
	commandQueue->WaitForFenceValue(fenceValue);
}
void HybridPipeline::transformGameObject() {

	auto device = Application::Get().GetDevice();
	
	{
		m_Scene->transformSingleAssembling("Assets/SM_TableRound",
			XMMatrixTranslation(0, 0.0, 0)
			, XMMatrixRotationX(-std::_Pi / 2.0f)
			, XMMatrixScaling(0.08f, 0.08f, 0.08f));

		m_Scene->transformSingleAssembling("Assets/SM_Chair",
			XMMatrixTranslation(-10, 0.0, -6)
			, XMMatrixRotationZ(-std::_Pi / 4.0f) * XMMatrixRotationX(-std::_Pi / 2.0f)
			, XMMatrixScaling(0.08f, 0.08f, 0.08f));

		m_Scene->transformSingleAssembling("Assets/SM_Chair_copy",
			XMMatrixTranslation(-4, 0.0, 6)
			, XMMatrixRotationZ(std::_Pi / 3.0f)*XMMatrixRotationX(-std::_Pi / 2.0f)
			, XMMatrixScaling(0.08f, 0.08f, 0.08f));

		m_Scene->transformSingleAssembling("Assets/SM_Couch",
			XMMatrixTranslation(8, 0.0, 0)
			, XMMatrixRotationZ(-std::_Pi ) *XMMatrixRotationX(-std::_Pi / 2.0f)
			, XMMatrixScaling(0.08f, 0.08f, 0.08f));

		m_Scene->transformSingleAssembling("Assets/SM_Lamp_Ceiling",
			XMMatrixTranslation(2, 0.0, 14)
			,  XMMatrixRotationX(std::_Pi / 2.0f)
			, XMMatrixScaling(0.12f, 0.12f, 0.12f));

		m_Scene->transformSingleAssembling("Assets/SM_MatPreviewMesh",
			XMMatrixTranslation(0, 5.5,0)
			, XMMatrixRotationX(-std::_Pi / 2.0f)
			, XMMatrixScaling(0.01f, 0.01f, 0.01f));

		m_Scene->transformSingleAssembling("Assets/Cerberus",
			XMMatrixTranslation(-1.2f, 10.0f, -1.6f)
			, XMMatrixRotationX(-std::_Pi/3.0f*2.0f)
			, XMMatrixScaling(0.02f, 0.02f, 0.02f));

		m_Scene->transformSingleAssembling("Assets/Unreal-actor",
			XMMatrixTranslation(0.0f, 10.0f, 0.0f)
			, XMMatrixRotationY( std::_Pi)
			, XMMatrixScaling(6.f, 6.f, 6.0f));

		m_Scene->transformSingleAssembling("Assets/Sci-fi-Biolab/source",
			XMMatrixTranslation(0.0f, 0.0f, 0.0f)
			, XMMatrixRotationX(-std::_Pi/2.0)
			, XMMatrixScaling(8.f, 8.f, 8.0f));

		m_Scene->transformSingleAssembling("sphere"
			, XMMatrixTranslation(4.0f, 3.0f, 2.0f)
			, XMMatrixIdentity()
			, XMMatrixScaling(6.0f, 6.0f, 6.0f));

		m_Scene->transformSingleAssembling("cube"
			, XMMatrixTranslation(-4.0f, 3.0f, -2.0f)
			, XMMatrixRotationY(XMConvertToRadians(45.0f))
			, XMMatrixScaling(6.0f, 6.0f, 6.0f));

		m_Scene->transformSingleAssembling("torus"
			, XMMatrixTranslation(4.0f, 0.6f, -6.0f)
			, XMMatrixRotationY(XMConvertToRadians(45.0f))
			, XMMatrixScaling(4.0f, 4.0f, 4.0f));

		float scalePlane = 20.0f;
		float translateOffset = scalePlane / 2.0f;

		m_Scene->transformSingleAssembling("Floor plane"
			, XMMatrixTranslation(0.0f, 0.0f, 0.0f)
			, XMMatrixIdentity()
			, XMMatrixScaling(scalePlane*10, 1.0f, scalePlane*10));

		m_Scene->transformSingleAssembling("Back wall"
			, XMMatrixTranslation(0.0f, translateOffset, translateOffset)
			, XMMatrixRotationX(XMConvertToRadians(-90))
			, XMMatrixScaling(scalePlane, 1.0f, scalePlane));

		m_Scene->transformSingleAssembling("Ceiling plane"
			, XMMatrixTranslation(0.0f, translateOffset * 2.0f, 0)
			, XMMatrixRotationX(XMConvertToRadians(180))
			, XMMatrixScaling(scalePlane, 1.0f, scalePlane));

		m_Scene->transformSingleAssembling("Front wall"
			, XMMatrixTranslation(0, translateOffset, -translateOffset)
			, XMMatrixRotationX(XMConvertToRadians(90))
			, XMMatrixScaling(scalePlane, 1.0f, scalePlane));

		m_Scene->transformSingleAssembling("Left wall"
			, XMMatrixTranslation(-translateOffset, translateOffset, 0)
			, XMMatrixRotationX(XMConvertToRadians(-90)) * XMMatrixRotationY(XMConvertToRadians(-90))
			, XMMatrixScaling(scalePlane, 1.0f, scalePlane));

		m_Scene->transformSingleAssembling("Right wall"
			, XMMatrixTranslation(translateOffset, translateOffset, 0)
			, XMMatrixRotationX(XMConvertToRadians(-90)) * XMMatrixRotationY(XMConvertToRadians(90))
			, XMMatrixScaling(scalePlane, 1.0f, scalePlane));
	}
}


