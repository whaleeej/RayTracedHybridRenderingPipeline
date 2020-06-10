#pragma once
#include <DX12LibPCH.h>
#include <Game.h>

#include "Renderer.h"
#include "Scene.h"


class HybridRenderingPipeline : public Game
{
public:
    using super = Game;

    HybridRenderingPipeline(const std::wstring& name, int width, int height, bool vSync = false);
    virtual ~HybridRenderingPipeline();
    virtual bool LoadContent() override;
    virtual void UnloadContent() override;

protected:
    virtual void OnUpdate(UpdateEventArgs& e) override;
    virtual void OnRender(RenderEventArgs& e) override;
    virtual void OnKeyPressed(KeyEventArgs& e) override;
    virtual void OnKeyReleased(KeyEventArgs& e);
    virtual void OnMouseMoved(MouseMotionEventArgs& e);
    virtual void OnMouseWheel(MouseWheelEventArgs& e) override;
    virtual void OnResize(ResizeEventArgs& e) override; 

private:
	std::shared_ptr<Scene> m_Scene;
	std::vector<std::shared_ptr<Renderer>> m_Renderers;
	RenderResourceMap m_RenderResourceMap;
};