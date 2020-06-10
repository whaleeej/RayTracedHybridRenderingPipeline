#pragma once

#include "Renderer.h"

class PostProcessingRenderer : public Renderer{
public:
	PostProcessingRenderer(int w, int h);
	virtual ~PostProcessingRenderer();

public://override
	virtual void LoadResource(std::shared_ptr<Scene> scene, RenderResourceMap& resources);
	virtual void LoadPipeline();
	virtual void Update(UpdateEventArgs& e, std::shared_ptr<Scene> scene);
	virtual void Render(RenderEventArgs& e, std::shared_ptr<Scene> scene, std::shared_ptr<CommandList> commandList);
	virtual void Resize(int w, int h);
	virtual void PressKey(KeyEventArgs& e);

public:
	virtual void PreRender(RenderResourceMap& resources);
	virtual RenderResourceMap* PostRender();

private:
	struct CameraCB
	{
		XMVECTOR PositionWS;
		XMMATRIX InverseViewMatrix;
		float fov;
		float padding[3];
	};
	struct PassTestingCB {
		int index = 0;
		float padding[3];
		void inc() {
			index = (index + 1) % 8;
		}
	};

private:
	RenderTarget presentRenderTarget;

	Texture gPosition; //srv
	Texture gAlbedoMetallic; //srv
	Texture gNormalRoughness; //srv
	Texture gExtra; //srv

	Texture mRtShadowOutputTexture;
	Texture mRtReflectOutputTexture;

	Texture col_acc;
	Texture filtered_curr;

	CameraCB mCameraCB;
	PassTestingCB postTestingCB;

	RootSignature m_PostLightingRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PostLightingPipelineState;

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;
};