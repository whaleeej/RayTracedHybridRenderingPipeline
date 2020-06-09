#include "Renderer.h"



Renderer::Renderer(int w, int h):
	m_Width(w), m_Height(h)
{

}

Renderer::~Renderer()
{
	m_Resources = nullptr;
}

void Renderer::Resize(int w, int h)
{
	m_Width = w;
	m_Height = h;
}

void Renderer::LoadResource(std::shared_ptr<Scene> scene, RenderResourceMap& resources) {
	m_Resources = &resources;
}

void Renderer::PreRender(RenderResourceMap& resources)
{
	m_Resources = &resources;
}

RenderResourceMap* Renderer::PostRender()
{
	return m_Resources;
}
