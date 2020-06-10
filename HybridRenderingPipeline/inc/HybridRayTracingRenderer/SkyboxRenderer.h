#pragma once

#include "Renderer.h"

class SkyboxRenderer: public Renderer {
public:
	SkyboxRenderer(int w, int h);
	virtual ~SkyboxRenderer();

public://override
	virtual void LoadResource(std::shared_ptr<Scene> scene, RenderResourceMap& resources);
	virtual void LoadPipeline();
	virtual void Update(UpdateEventArgs& e, std::shared_ptr<Scene> scene);
	virtual void Render(RenderEventArgs& e, std::shared_ptr<Scene> scene, std::shared_ptr<CommandList> commandList);
	virtual void Resize(int w, int h);

public:
	virtual void PreRender(RenderResourceMap& resources);
	virtual RenderResourceMap* PostRender();

private:
	RootSignature m_SkyboxSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_SkyboxPipelineState;

	RenderTarget m_SkyboxRT;

	Texture m_Albedo;
	Texture m_Albedo_prev;
	Texture m_DepthStencil;

	DirectX::XMMATRIX viewProjMatrix;

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;
};
