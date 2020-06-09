#pragma once

#include "Renderer.h"

class ShadowFilteringRenderer : public Renderer {
public:
	ShadowFilteringRenderer(int w, int h);
	virtual ~ShadowFilteringRenderer();

public://override
	virtual void LoadResource(std::shared_ptr<Scene> scene, RenderResourceMap& resources);
	virtual void LoadPipeline();
	virtual void Update(UpdateEventArgs& e, std::shared_ptr<Scene> scene);
	virtual void Render(RenderEventArgs& e, std::shared_ptr<Scene> scene, std::shared_ptr<CommandList> commandList);

private:
	Texture gPosition; //srv
	Texture gAlbedoMetallic; //srv
	Texture gNormalRoughness; //srv
	Texture gExtra; //srv
	Texture gPosition_prev; //srv
	Texture gAlbedoMetallic_prev; //srv
	Texture gNormalRoughness_prev; //srv
	Texture gExtra_prev; //srv

	// SVGF Post Temporal
	Texture col_acc; //uav
	Texture moment_acc; //uav
	Texture his_length; //uav
	Texture col_acc_prev; //srv
	Texture moment_acc_prev; //srv
	Texture his_length_prev; //srv

	// SVGF PostATrous
	Texture color_inout[2];
	Texture variance_inout[2];

	Texture mRtShadowOutputTexture;

	XMMATRIX viewProjectMatrix;
	XMMATRIX viewProjectMatrix_prev;

	RootSignature m_PostSVGFTemporalRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PostSVGFTemporalPipelineState;

	RootSignature m_PostSVGFATrousRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PostSVGFATrousPipelineState;

	uint32_t ATrous_Level_Max = 3;
	int local_width = 8;
	int local_height = 8;
};