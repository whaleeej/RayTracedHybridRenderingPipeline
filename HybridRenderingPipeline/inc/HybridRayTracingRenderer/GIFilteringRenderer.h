#pragma once

#include "Renderer.h"

#define LOCAL_WIDTH 8
#define LOCAL_HEIGHT 8
#define BLOCK_EDGE_LENGTH 32
#define BLOCK_PIXELS BLOCK_EDGE_LENGTH*BLOCK_EDGE_LENGTH
#define BLOCK_EDGE_HALF (BLOCK_EDGE_LENGTH / 2)
#define WORKSET_WIDTH (BLOCK_EDGE_LENGTH * ((m_Width + BLOCK_EDGE_LENGTH - 1) / BLOCK_EDGE_LENGTH))
#define WORKSET_HEIGHT (BLOCK_EDGE_LENGTH *  ((m_Height + BLOCK_EDGE_LENGTH - 1) / BLOCK_EDGE_LENGTH))
#define WORKSET_WITH_MARGINS_WIDTH (WORKSET_WIDTH + BLOCK_EDGE_LENGTH)
#define WORKSET_WITH_MARGINS_HEIGHT (WORKSET_HEIGHT + BLOCK_EDGE_LENGTH)
#define OUTPUT_SIZE (WORKSET_WIDTH * WORKSET_HEIGHT)
#define LOCAL_SIZE 256
#define FITTER_GLOBAL (LOCAL_SIZE * ((WORKSET_WITH_MARGINS_WIDTH / BLOCK_EDGE_LENGTH) * (WORKSET_WITH_MARGINS_HEIGHT / BLOCK_EDGE_LENGTH)))
// feature buffer define
#define BUFFER_COUNT 15
#define FEATURES_NOT_SCALED 6
#define FEATURES_SCALED 6
#define NOT_SCALED_FEATURE_BUFFERS \
1.f,\
normal.x,\
normal.y,\
normal.z,\
metallic.x,\
roughness.x,
#define SCALED_FEATURE_BUFFERS \
world_position.x,\
world_position.y,\
world_position.z,\
world_position.x*world_position.x,\
world_position.y*world_position.y,\
world_position.z*world_position.z



class GIFilteringRenderer : public Renderer {


public:
	GIFilteringRenderer(int w, int h);
	virtual ~GIFilteringRenderer();

public://override
	virtual void LoadResource(std::shared_ptr<Scene> scene, RenderResourceMap& resources);
	virtual void LoadPipeline();
	virtual void Update(UpdateEventArgs& e, std::shared_ptr<Scene> scene);
	virtual void Render(RenderEventArgs& e, std::shared_ptr<Scene> scene, std::shared_ptr<CommandList> commandList);

public:
	virtual void PreRender(RenderResourceMap& resources);
	virtual RenderResourceMap* PostRender();

private:
	Texture gPosition; //srv
	Texture gAlbedoMetallic; //srv
	Texture gNormalRoughness; //srv
	Texture gExtra; //srv
	Texture gPosition_prev; //srv
	Texture gAlbedoMetallic_prev; //srv
	Texture gNormalRoughness_prev; //srv
	Texture gExtra_prev; //srv

	Texture mRtReflectOutputTexture;

	// BMFR_1_TemporalNoisy
	Texture noisy_curr;
	Texture noisy_prev;
	Texture spp_curr;
	Texture spp_prev;
	Texture pixel_reproject;
	Texture pixel_accept;
	Texture A_LSQ_matrix;

	// BMFR_2_QRFactorization
	Texture lsq_weights;
	Texture feature_scale_minmax;

	// BMFR_3_WeightedSUm
	Texture weighted_sum;

	// BMFR_4_TemporalFilterd
	Texture filtered_curr;
	Texture filtered_prev;

	RootSignature m_PostBMFRTemporalNoisyRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PostBMFRTemporalNoisyPipelineState;

	RootSignature m_PostBMFRQRFactorizationRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PostBMFRQRFactorizationPipelineState;

	RootSignature m_PostBMFRWeightedSumRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PostBMFRWeightedSumPipelineState;

	RootSignature m_PostBMFRTemporalFilteredRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PostBMFRTemporalFilteredPipelineState;

	XMMATRIX viewProjectMatrix;
	XMMATRIX viewProjectMatrix_prev;

	int local_width = 8;
	int local_height = 8;
};