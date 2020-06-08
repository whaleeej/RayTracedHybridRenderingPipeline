#pragma once

#include <DX12LibPCH.h>
#include <Events.h>
#include "Scene.h"

using RenderResourceIndex = std::string;
using RenderResourceMap = std::map<RenderResourceIndex, std::shared_ptr<Texture>>;

class Renderer {
public:
	virtual void Resize(int w, int h);


	virtual void LoadResource(std::shared_ptr<Scene> scene)=0;
	virtual void LoadPipeline()=0;
	virtual void Update(UpdateEventArgs& e, std::shared_ptr<Scene> scene) = 0;
	virtual void PreRender(RenderResourceMap& resources);
	virtual void Render(RenderEventArgs& e, std::shared_ptr<Scene> scene) = 0;
	virtual RenderResourceMap& PostRender();

protected:
	int m_Width;
	int m_Height;
	RenderResourceMap& m_Resources;
};