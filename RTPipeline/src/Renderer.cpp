#include "Renderer.h"

void Renderer::Resize(int w, int h)
{
	m_Width = w;
	m_Height = h;
}

void Renderer::PreRender(RenderResourceMap& resources)
{
	m_Resources = resources;
}

RenderResourceMap& Renderer::PostRender()
{
	return m_Resources;
}
