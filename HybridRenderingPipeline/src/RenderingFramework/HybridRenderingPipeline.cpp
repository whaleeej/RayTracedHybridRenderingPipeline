#include <HybridRenderingPipeline.h>
#include <Window.h>
#include <Application.h>
#include <CommandQueue.h>
#include <CommandList.h>
#include <Texture.h>

#include "SkyboxRenderer.h"
#include "GBufferRenderer.h"
#include "RayTracingRenderer.h"
#include "ShadowFilteringRenderer.h"
#include "GIFilteringRenderer.h"
#include "PostProcessingRenderer.h"

#include <RenderdocBoost.h>

static bool g_AllowFullscreenToggle = true;
static uint64_t frameCount = 0;
static double totalTime = 0.0;

HybridRenderingPipeline::HybridRenderingPipeline(const std::wstring& name, int width, int height, bool vSync)
    : super(name, width, height, vSync)
{
	
	m_Scene = std::make_shared<Scene>();
	m_Renderers.push_back(std::make_shared<SkyboxRenderer>(m_Width, m_Height));
	m_Renderers.push_back(std::make_shared<GBufferRenderer>(m_Width, m_Height));
	if (Application::Get().isDXRSupport()) {
		m_Renderers.push_back(std::make_shared<RayTracingRenderer>(m_Width, m_Height));
		m_Renderers.push_back(std::make_shared<ShadowFilteringRenderer>(m_Width, m_Height));
		m_Renderers.push_back(std::make_shared<GIFilteringRenderer>(m_Width, m_Height));
	}
	m_Renderers.push_back(std::make_shared<PostProcessingRenderer>(m_Width, m_Height));
}

HybridRenderingPipeline::~HybridRenderingPipeline()
{
    
}

bool HybridRenderingPipeline::LoadContent()
{
	m_RenderResourceMap.emplace(RRD_RENDERTARGET_PRESENT,std::make_shared<Texture>(m_pWindow->GetRenderTarget().GetTexture(AttachmentPoint::Color0)));
	
	m_Scene->LoadContent();

	for (auto& pRenderer : m_Renderers) {
		pRenderer->LoadResource(m_Scene, m_RenderResourceMap);
		pRenderer->LoadPipeline();
	}
	return true;
}

void HybridRenderingPipeline::OnResize(ResizeEventArgs& e)
{
    super::OnResize(e);
    if (m_Width != e.Width || m_Height != e.Height)
    {
        m_Width = std::max(1, e.Width);
        m_Height = std::max(1, e.Height);

		m_Scene->OnResize(m_Width, m_Height);

		for (auto& pRenderer : m_Renderers) {
			pRenderer->Resize(m_Width, m_Height);
		}
    }
}

void HybridRenderingPipeline::UnloadContent()
{
	m_Scene->UnLoadContent();
}

void HybridRenderingPipeline::OnUpdate(UpdateEventArgs& e)
{
    super::OnUpdate(e);

	// show the frametime
	totalTime += e.ElapsedTime; frameCount++;

	if (totalTime > 1.0){
		double fps = frameCount / totalTime;
		char buffer[512];
		sprintf_s(buffer, "FPS: %f,  FrameCount: %lld\n", fps, e.FrameNumber);
		OutputDebugStringA(buffer);

		frameCount = 0;totalTime = 0.0;
	}

    m_Scene->OnUpdate(e);

	for (auto& pRenderer : m_Renderers) {
		pRenderer->Update(e, m_Scene);
	}
}

void HybridRenderingPipeline::OnRender(RenderEventArgs& e)
{
    super::OnRender(e);

    auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = commandQueue->GetCommandList();
	
	m_RenderResourceMap[RRD_RENDERTARGET_PRESENT]=std::make_shared<Texture>(m_pWindow->GetRenderTarget().GetTexture(AttachmentPoint::Color0));

	for (auto& pRenderer : m_Renderers) {
		// main render skeleton: Pre Rendering
		pRenderer->PreRender(m_RenderResourceMap);

		// main render skeleton: Render it!
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
	m_pWindow->Present();
}

void HybridRenderingPipeline::OnKeyPressed(KeyEventArgs& e)
{
    super::OnKeyPressed(e);

    m_Scene->OnKeyPressed(e);

	for (auto& pRenderer : m_Renderers) {
		pRenderer->PressKey(e);
	}

    switch (e.Key)
    {
    case KeyCode::Escape:
        Application::Get().Quit(0);
        break;
    case KeyCode::Enter:
        if (e.Alt)
        {
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

	case KeyCode::C:
		if (Application::Get().IsRenderDocEnabled()) {
			rdcboost::D3D12EnableRenderDoc(Application::Get().GetDevice().Get(), 1);
			void* m_pRdcAPI = rdcboost::GetRenderdocAPI();
			RENDERDOC_API_1_0_1* pAPI = static_cast<RENDERDOC_API_1_0_1*>(m_pRdcAPI);
			if (pAPI != NULL)
			{
				std::string pathTemplate = pAPI->GetLogFilePathTemplate();
				size_t lastSep = pathTemplate.find_last_of("\\/");
				if (lastSep != std::string::npos)
					pAPI->SetLogFilePathTemplate(pathTemplate.substr(lastSep + 1).c_str());
			}
		}
		break;
    }
}

void HybridRenderingPipeline::OnKeyReleased(KeyEventArgs& e)
{
    super::OnKeyReleased(e);

    m_Scene->OnKeyReleased(e);

	for (auto& pRenderer : m_Renderers) {
		pRenderer->ReleaseKey(e);
	}

    switch (e.Key)
    {
    case KeyCode::Enter:
        if (e.Alt)
        {
			g_AllowFullscreenToggle = true;
        }
        break;
    }
}

void HybridRenderingPipeline::OnMouseMoved(MouseMotionEventArgs& e)
{
    super::OnMouseMoved(e);

    m_Scene->OnMouseMoved(e);
}

void HybridRenderingPipeline::OnMouseWheel(MouseWheelEventArgs& e)
{
    super::OnMouseWheel(e);

    m_Scene->OnMouseWheel(e);
}



