#pragma once

#include "Renderer.h"

class GBufferRenderer : public Renderer {
public:
	GBufferRenderer(int w, int h);
	virtual ~GBufferRenderer();

public://override
	virtual void LoadResource(std::shared_ptr<Scene> scene, RenderResourceMap& resources);
	virtual void LoadPipeline();
	virtual void Update(UpdateEventArgs& e, std::shared_ptr<Scene> scene);
	virtual void Render(RenderEventArgs& e, std::shared_ptr<Scene> scene, std::shared_ptr<CommandList> commandList);
	virtual void Resize(int w, int h);

private:
	RootSignature m_DeferredRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_DeferredPipelineState;

	RenderTarget m_GBuffer;
	Texture gPosition; //srv
	Texture gAlbedoMetallic; //srv
	Texture gNormalRoughness; //srv
	Texture gExtra; //srv
	Texture gPosition_prev; //srv
	Texture gAlbedoMetallic_prev; //srv
	Texture gNormalRoughness_prev; //srv
	Texture gExtra_prev; //srv


	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;
};
