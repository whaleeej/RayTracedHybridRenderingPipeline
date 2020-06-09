#pragma once
#include <DX12LibPCH.h>
#include <Camera.h>
#include <Game.h>
#include <IndexBuffer.h>
#include <Light.h>
#include <Window.h>
#include <Mesh.h>
#include <RootSignature.h>
#include <Texture.h>
#include <VertexBuffer.h>
#include <RTHelper.h>
#include <DirectXMath.h>
#include <Material.h>
#include <random>
#include <RenderTarget.h>


#include "Transform.h"
#include "PBRMaterial.h"
#include "TexturePool.h"
#include "MeshPool.h"
#include "Scene.h"
#include "SkyboxRenderer.h"


class HybridPipeline : public Game
{
public:
    using super = Game;

    HybridPipeline(const std::wstring& name, int width, int height, bool vSync = false);
    virtual ~HybridPipeline();
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
	void loadResource();
	void loadGameObject();
	void transformGameObject();

private:
	std::shared_ptr<Scene> m_Scene;
	std::vector<std::shared_ptr<Renderer>> m_Renderers;
	RenderResourceMap m_RenderResourceMap;
	
	int m_Width;
	int m_Height;

	/////////////////////////////////////////////// Camera controller
    float m_Forward;
    float m_Backward;
    float m_Left;
    float m_Right;
    float m_Up;
    float m_Down;
    float m_Pitch;
    float m_Yaw;
	bool m_AnimateCamera;
    bool m_AnimateLights;
};